#include "VideoDecoder.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <mferror.h>
#include <propvarutil.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "winmm.lib")
#endif

VideoDecoder::VideoDecoder() {
#ifdef _WIN32
    HRESULT hr = MFStartup(MF_VERSION);
    m_mfInitialized = SUCCEEDED(hr);
#endif
}

VideoDecoder::~VideoDecoder() {
    close();
#ifdef _WIN32
    if (m_mfInitialized) {
        MFShutdown();
    }
#endif
}

bool VideoDecoder::open(const std::filesystem::path& filepath) {
    close();

    m_filePath = filepath;
    if (!std::filesystem::exists(filepath)) {
        return false;
    }

#ifdef _WIN32
    if (!m_mfInitialized) return false;

    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 2);
    if (FAILED(hr)) return false;

    // Enable hardware acceleration / DXVA video processing when available
    pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

    std::wstring wpath = filepath.wstring();
    hr = MFCreateSourceReaderFromURL(wpath.c_str(), pAttributes, &m_pReader);
    pAttributes->Release();

    if (FAILED(hr) || !m_pReader) {
        return false;
    }

    // Configure the video decoder to output uncompressed RGB32
    IMFMediaType* pVideoType = nullptr;
    hr = MFCreateMediaType(&pVideoType);
    if (SUCCEEDED(hr)) {
        pVideoType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pVideoType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pVideoType);
        pVideoType->Release();
    }

    if (FAILED(hr)) {
        close();
        return false;
    }

    // Get output video dimensions and framerate
    IMFMediaType* pCurrentType = nullptr;
    hr = m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
    if (SUCCEEDED(hr) && pCurrentType) {
        UINT32 w = 0, h = 0;
        if (SUCCEEDED(MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &w, &h))) {
            m_width = w;
            m_height = h;
        }

        UINT32 num = 0, den = 1;
        if (SUCCEEDED(MFGetAttributeRatio(pCurrentType, MF_MT_FRAME_RATE, &num, &den)) && den > 0) {
            m_fps = static_cast<double>(num) / static_cast<double>(den);
        }

        pCurrentType->Release();
    }

    // Configure audio stream for uncompressed 44.1kHz Stereo 16-bit PCM
    m_pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    IMFMediaType* pAudioType = nullptr;
    hr = MFCreateMediaType(&pAudioType);
    if (SUCCEEDED(hr)) {
        pAudioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        pAudioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        pAudioType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        pAudioType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
        pAudioType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        pAudioType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
        pAudioType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 4);

        HRESULT hrAudio = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pAudioType);
        pAudioType->Release();

        if (SUCCEEDED(hrAudio)) {
            m_hasAudio.store(true);
            WAVEFORMATEX wfx = {};
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 2;
            wfx.nSamplesPerSec = 44100;
            wfx.wBitsPerSample = 16;
            wfx.nBlockAlign = 4;
            wfx.nAvgBytesPerSec = 44100 * 4;

            if (waveOutOpen(&m_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR) {
                m_waveHeaders.resize(WAVE_BUFFER_COUNT);
                m_waveBuffers.resize(WAVE_BUFFER_COUNT, std::vector<uint8_t>(WAVE_BUFFER_SIZE, 0));
                for (size_t i = 0; i < WAVE_BUFFER_COUNT; ++i) {
                    ZeroMemory(&m_waveHeaders[i], sizeof(WAVEHDR));
                    m_waveHeaders[i].lpData = reinterpret_cast<LPSTR>(m_waveBuffers[i].data());
                    m_waveHeaders[i].dwBufferLength = static_cast<DWORD>(WAVE_BUFFER_SIZE);
                    m_waveHeaders[i].dwFlags = WHDR_DONE;
                }
                m_currentWaveHdr = 0;
            }
        }
    }

    // Query presentation duration
    PROPVARIANT var;
    PropVariantInit(&var);
    hr = m_pReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (SUCCEEDED(hr) && var.vt == VT_UI8) {
        // MF duration is in 100-nanosecond units (hns)
        m_durationUs = var.uhVal.QuadPart / 10;
    }
    PropVariantClear(&var);

    m_isLoaded.store(true);
    m_stopRequested.store(false);

    // Start decoding thread
    m_decodeThread = std::thread(&VideoDecoder::decodeLoop, this);
    play();

    return true;
#else
    return false;
#endif
}

void VideoDecoder::close() {
    m_stopRequested.store(true);
    m_isPlaying.store(false);

    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }

#ifdef _WIN32
    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut);
        for (size_t i = 0; i < m_waveHeaders.size(); ++i) {
            if (m_waveHeaders[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(m_hWaveOut, &m_waveHeaders[i], sizeof(WAVEHDR));
            }
        }
        waveOutClose(m_hWaveOut);
        m_hWaveOut = nullptr;
    }
    m_waveHeaders.clear();
    m_waveBuffers.clear();

    if (m_pReader) {
        m_pReader->Release();
        m_pReader = nullptr;
    }
#endif

    m_isLoaded.store(false);
    m_width = 0;
    m_height = 0;
    m_hasNewFrame.store(false);
}

void VideoDecoder::play() {
    if (!m_isPlaying.load()) {
        m_startTime = std::chrono::high_resolution_clock::now() - std::chrono::microseconds(m_pausedTimeUs);
        m_isPlaying.store(true);
#ifdef _WIN32
        if (m_hWaveOut) {
            waveOutRestart(m_hWaveOut);
        }
#endif
    }
}

void VideoDecoder::pause() {
    if (m_isPlaying.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        m_pausedTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count();
        m_isPlaying.store(false);
#ifdef _WIN32
        if (m_hWaveOut) {
            waveOutPause(m_hWaveOut);
        }
#endif
    }
}

void VideoDecoder::stop() {
    pause();
    m_pausedTimeUs = 0;
    seekToStart();
}

void VideoDecoder::seekToStart() {
#ifdef _WIN32
    if (m_hWaveOut) {
        waveOutReset(m_hWaveOut);
    }
    if (m_pReader) {
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = 0;
        m_pReader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);
    }
#endif
}

void VideoDecoder::writeAudioPcm(const uint8_t* data, size_t length) {
#ifdef _WIN32
    if (!m_hWaveOut || length == 0) return;

    float vol = m_volume.load();
    if (m_mute.load() || vol <= 0.001f) return;

    size_t offset = 0;
    while (offset < length && !m_stopRequested.load()) {
        size_t chunk = std::min(length - offset, static_cast<size_t>(WAVE_BUFFER_SIZE));

        WAVEHDR& hdr = m_waveHeaders[m_currentWaveHdr];
        int attempts = 0;
        while (!(hdr.dwFlags & WHDR_DONE) && !m_stopRequested.load() && attempts < 25) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            attempts++;
        }

        if (hdr.dwFlags & WHDR_PREPARED) {
            waveOutUnprepareHeader(m_hWaveOut, &hdr, sizeof(WAVEHDR));
        }

        std::vector<uint8_t>& buf = m_waveBuffers[m_currentWaveHdr];
        std::memcpy(buf.data(), data + offset, chunk);

        // Apply real-time volume scaling
        if (vol < 0.999f) {
            int16_t* samples = reinterpret_cast<int16_t*>(buf.data());
            size_t sampleCount = chunk / sizeof(int16_t);
            for (size_t i = 0; i < sampleCount; ++i) {
                samples[i] = static_cast<int16_t>(std::clamp<float>(samples[i] * vol, -32768.f, 32767.f));
            }
        }

        hdr.lpData = reinterpret_cast<LPSTR>(buf.data());
        hdr.dwBufferLength = static_cast<DWORD>(chunk);
        hdr.dwFlags = 0;

        if (waveOutPrepareHeader(m_hWaveOut, &hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
            waveOutWrite(m_hWaveOut, &hdr, sizeof(WAVEHDR));
        }

        m_currentWaveHdr = (m_currentWaveHdr + 1) % WAVE_BUFFER_COUNT;
        offset += chunk;
    }
#endif
}

void VideoDecoder::decodeLoop() {
#ifdef _WIN32
    VideoFrame localFrame;

    while (!m_stopRequested.load()) {
        if (!m_isPlaying.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (!m_pReader) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG llTimestamp = 0;
        IMFSample* pSample = nullptr;

        HRESULT hr = m_pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_ANY_STREAM,
            0,
            &streamIndex,
            &flags,
            &llTimestamp,
            &pSample
        );

        if (FAILED(hr)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            if (pSample) pSample->Release();
            if (m_loop.load()) {
                seekToStart();
                m_startTime = std::chrono::high_resolution_clock::now();
                m_pausedTimeUs = 0;
                continue;
            } else {
                m_isPlaying.store(false);
                continue;
            }
        }

        if (!pSample) {
            continue;
        }

        // Process Audio Stream
        if (streamIndex == (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM) {
            if (m_hWaveOut && !m_mute.load() && m_volume.load() > 0.001f) {
                IMFMediaBuffer* pBuffer = nullptr;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&pBuffer)) && pBuffer) {
                    BYTE* pData = nullptr;
                    DWORD currentLength = 0;
                    if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &currentLength)) && pData && currentLength > 0) {
                        writeAudioPcm(pData, currentLength);
                        pBuffer->Unlock();
                    }
                    pBuffer->Release();
                }
            }
            pSample->Release();
            continue;
        }

        // Process Video Stream
        if (streamIndex == (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM) {
            IMFMediaBuffer* pBuffer = nullptr;
            hr = pSample->ConvertToContiguousBuffer(&pBuffer);
            if (SUCCEEDED(hr) && pBuffer) {
                BYTE* pData = nullptr;
                DWORD maxLength = 0, currentLength = 0;

                hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
                if (SUCCEEDED(hr) && pData && m_width > 0 && m_height > 0) {
                    size_t pixelCount = static_cast<size_t>(m_width) * m_height;
                    localFrame.data.resize(pixelCount * 4);
                    localFrame.width = m_width;
                    localFrame.height = m_height;
                    localFrame.timestampUs = llTimestamp / 10;

                    const uint8_t* src = pData;
                    uint8_t* dst = localFrame.data.data();

                    for (size_t i = 0; i < pixelCount; ++i) {
                        uint8_t b = src[i * 4 + 0];
                        uint8_t g = src[i * 4 + 1];
                        uint8_t r = src[i * 4 + 2];
                        dst[i * 4 + 0] = r;
                        dst[i * 4 + 1] = g;
                        dst[i * 4 + 2] = b;
                        dst[i * 4 + 3] = 255;
                    }

                    pBuffer->Unlock();
                }
                pBuffer->Release();
            }

            pSample->Release();

            if (localFrame.data.empty()) {
                continue;
            }

            // Frame synchronization: sleep until this frame is due for display
            auto now = std::chrono::high_resolution_clock::now();
            int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count();

            if (localFrame.timestampUs > elapsedUs) {
                int64_t sleepUs = localFrame.timestampUs - elapsedUs;
                if (sleepUs > 1000) {
                    std::this_thread::sleep_for(std::chrono::microseconds(sleepUs - 500));
                }
            }

            // Push frame to buffer
            {
                std::lock_guard<std::mutex> lock(m_frameMutex);
                m_pendingFrame = std::move(localFrame);
                m_hasNewFrame.store(true);
            }
        } else {
            pSample->Release();
        }
    }
#endif
}

bool VideoDecoder::getNextFrame(std::vector<uint8_t>& outBuffer, uint32_t& outWidth, uint32_t& outHeight) {
    if (!m_hasNewFrame.load()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (!m_hasNewFrame.load()) {
        return false;
    }

    m_currentFrame = std::move(m_pendingFrame);
    m_hasNewFrame.store(false);

    outBuffer = m_currentFrame.data;
    outWidth = m_currentFrame.width;
    outHeight = m_currentFrame.height;
    return true;
}
