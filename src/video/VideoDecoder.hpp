#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#endif

struct VideoFrame {
    std::vector<uint8_t> data; // RGBA32
    int64_t timestampUs = 0;   // In microseconds
    uint32_t width = 0;
    uint32_t height = 0;
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();

    bool open(const std::filesystem::path& filepath);
    void close();

    void play();
    void pause();
    void stop();
    void setLoop(bool loop) { m_loop.store(loop); }
    bool isLoop() const { return m_loop.load(); }
    bool isPlaying() const { return m_isPlaying.load(); }
    bool isLoaded() const { return m_isLoaded.load(); }

    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    double getFPS() const { return m_fps; }
    int64_t getDurationUs() const { return m_durationUs; }

    // Retrieve latest decoded frame for the current playback time
    bool getNextFrame(std::vector<uint8_t>& outBuffer, uint32_t& outWidth, uint32_t& outHeight);

private:
    void decodeLoop();
    bool readFrameInternal(VideoFrame& frame);
    void seekToStart();

    std::filesystem::path m_filePath;
    std::atomic<bool> m_isLoaded{false};
    std::atomic<bool> m_isPlaying{false};
    std::atomic<bool> m_loop{true};
    std::atomic<bool> m_stopRequested{false};

    uint32_t m_width{0};
    uint32_t m_height{0};
    double m_fps{30.0};
    int64_t m_durationUs{0};

    std::chrono::high_resolution_clock::time_point m_startTime;
    int64_t m_pausedTimeUs{0};

    std::thread m_decodeThread;
    std::mutex m_frameMutex;

    // Double buffering for smooth GPU texture upload
    VideoFrame m_currentFrame;
    VideoFrame m_pendingFrame;
    std::atomic<bool> m_hasNewFrame{false};

#ifdef _WIN32
    IMFSourceReader* m_pReader{nullptr};
    bool m_mfInitialized{false};
#endif
};
