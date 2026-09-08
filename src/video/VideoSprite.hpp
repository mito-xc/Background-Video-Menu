#pragma once

#include <Geode/Geode.hpp>
#include "VideoDecoder.hpp"
#include <filesystem>
#include <memory>

enum class FitMode {
    Cover,      // Fills area preserving aspect ratio (clips excess)
    Contain,    // Fits inside area preserving aspect ratio (letterboxing)
    Stretch,    // Fills entire area stretching aspect ratio
    Center      // Displays original size centered
};

class VideoSprite : public cocos2d::CCSprite {
public:
    static VideoSprite* create(const std::filesystem::path& filepath);

    bool init(const std::filesystem::path& filepath);
    virtual void update(float dt) override;
    virtual void onEnter() override;
    virtual void onExit() override;

    void setFitMode(FitMode mode);
    FitMode getFitMode() const { return m_fitMode; }

    void setTargetSize(const cocos2d::CCSize& size);
    cocos2d::CCSize getTargetSize() const { return m_targetSize; }

    void setLoop(bool loop);
    bool isLoop() const;

    void play();
    void pause();
    void stop();

    VideoDecoder* getDecoder() { return m_decoder.get(); }

private:
    void updateTexture(const std::vector<uint8_t>& data, uint32_t width, uint32_t height);
    void applyFitMode();

    std::unique_ptr<VideoDecoder> m_decoder;
    std::filesystem::path m_filePath;
    FitMode m_fitMode{FitMode::Cover};
    cocos2d::CCSize m_targetSize{cocos2d::CCSizeZero};
    GLuint m_textureId{0};
    uint32_t m_texWidth{0};
    uint32_t m_texHeight{0};
    std::vector<uint8_t> m_frameBuffer;
};
