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

    // Configure the decoder to output uncompressed RGB32
    IMFMediaType* pMediaType = nullptr;
    hr = MFCreateMediaType(&pMediaType);
    if (FAILED(hr)) {
        close();
        return false;
    }

    pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

    hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType);
    pMediaType->Release();

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
    }
}

void VideoDecoder::pause() {
    if (m_isPlaying.load()) {
        auto now = std::chrono::high_resolution_clock::now();
        m_pausedTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count();
        m_isPlaying.store(false);
    }
}

void VideoDecoder::stop() {
    pause();
    m_pausedTimeUs = 0;
    seekToStart();
}

void VideoDecoder::seekToStart() {
#ifdef _WIN32
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

bool VideoDecoder::readFrameInternal(VideoFrame& frame) {
#ifdef _WIN32
    if (!m_pReader) return false;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG llTimestamp = 0;
    IMFSample* pSample = nullptr;

    HRESULT hr = m_pReader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &llTimestamp,
        &pSample
    );

    if (FAILED(hr)) {
        return false;
    }

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        if (pSample) pSample->Release();
        return false;
    }

    if (!pSample) {
        return true; // Null sample (e.g. stream format change or skip), try again
    }

    IMFMediaBuffer* pBuffer = nullptr;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (SUCCEEDED(hr) && pBuffer) {
        BYTE* pData = nullptr;
        DWORD maxLength = 0, currentLength = 0;

        hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
        if (SUCCEEDED(hr) && pData && m_width > 0 && m_height > 0) {
            size_t pixelCount = static_cast<size_t>(m_width) * m_height;
            frame.data.resize(pixelCount * 4);
            frame.width = m_width;
            frame.height = m_height;
            frame.timestampUs = llTimestamp / 10; // Convert 100ns to microseconds

            // Convert RGB32 (BGRA/BGR0 bottom-up or top-down) to standard RGBA32
            // Windows RGB32 is typically BGRA with lines stored top-down or bottom-up
            const uint8_t* src = pData;
            uint8_t* dst = frame.data.data();

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
    return true;
#else
    return false;
#endif
}

void VideoDecoder::decodeLoop() {
    VideoFrame localFrame;

    while (!m_stopRequested.load()) {
        if (!m_isPlaying.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        bool success = readFrameInternal(localFrame);

        if (!success) {
            // End of stream reached
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
    }
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
