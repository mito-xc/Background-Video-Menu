#include "VideoSprite.hpp"

using namespace geode::prelude;

VideoSprite* VideoSprite::create(const std::filesystem::path& filepath) {
    auto ret = new VideoSprite();
    if (ret && ret->init(filepath)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool VideoSprite::init(const std::filesystem::path& filepath) {
    if (!CCSprite::init()) {
        return false;
    }

    m_filePath = filepath;
    m_decoder = std::make_unique<VideoDecoder>();

    if (!m_decoder->open(filepath)) {
        log::error("No se pudo abrir el archivo de video: {}", filepath.string());
        return false;
    }

    m_texWidth = m_decoder->getWidth();
    m_texHeight = m_decoder->getHeight();

    if (m_texWidth == 0 || m_texHeight == 0) {
        m_texWidth = 1920;
        m_texHeight = 1080;
    }

    // Crear textura inicial en RGBA8888
    std::vector<uint8_t> blank(m_texWidth * m_texHeight * 4, 0);
    auto pTexture = new CCTexture2D();
    if (pTexture->initWithData(blank.data(), kCCTexture2DPixelFormat_RGBA8888, m_texWidth, m_texHeight, CCSizeMake(static_cast<float>(m_texWidth), static_cast<float>(m_texHeight)))) {
        this->initWithTexture(pTexture);
        this->setTextureRect(CCRectMake(0, 0, static_cast<float>(m_texWidth), static_cast<float>(m_texHeight)));
        pTexture->release();
    } else {
        pTexture->release();
        return false;
    }

    this->setAnchorPoint({0.5f, 0.5f});
    m_targetSize = CCDirector::sharedDirector()->getWinSize();
    applyFitMode();

    this->scheduleUpdate();
    return true;
}

void VideoSprite::onEnter() {
    CCSprite::onEnter();
    if (m_decoder && !m_decoder->isPlaying()) {
        m_decoder->play();
    }
}

void VideoSprite::onExit() {
    if (m_decoder && m_decoder->isPlaying()) {
        m_decoder->pause();
    }
    CCSprite::onExit();
}

void VideoSprite::update(float dt) {
    CCSprite::update(dt);

    if (!m_decoder) return;

    uint32_t w = 0, h = 0;
    if (m_decoder->getNextFrame(m_frameBuffer, w, h)) {
        if (w > 0 && h > 0 && !m_frameBuffer.empty()) {
            updateTexture(m_frameBuffer, w, h);
        }
    }
}

void VideoSprite::updateTexture(const std::vector<uint8_t>& data, uint32_t width, uint32_t height) {
    auto texture = this->getTexture();
    if (!texture) return;

    if (width != m_texWidth || height != m_texHeight) {
        // Redimensionar textura si cambió de resolución
        m_texWidth = width;
        m_texHeight = height;

        auto newTexture = new CCTexture2D();
        if (newTexture->initWithData(data.data(), kCCTexture2DPixelFormat_RGBA8888, width, height, CCSizeMake(static_cast<float>(width), static_cast<float>(height)))) {
            this->setTexture(newTexture);
            this->setTextureRect(CCRectMake(0, 0, static_cast<float>(width), static_cast<float>(height)));
            newTexture->release();
            applyFitMode();
        } else {
            newTexture->release();
        }
        return;
    }

    // Actualización directa en la GPU
    glBindTexture(GL_TEXTURE_2D, texture->getName());
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        width,
        height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data.data()
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VideoSprite::setFitMode(FitMode mode) {
    m_fitMode = mode;
    applyFitMode();
}

void VideoSprite::setTargetSize(const CCSize& size) {
    m_targetSize = size;
    applyFitMode();
}

void VideoSprite::applyFitMode() {
    if (m_texWidth == 0 || m_texHeight == 0) return;

    CCSize target = m_targetSize;
    if (target.width <= 0 || target.height <= 0) {
        target = CCDirector::sharedDirector()->getWinSize();
    }

    float scaleX = target.width / static_cast<float>(m_texWidth);
    float scaleY = target.height / static_cast<float>(m_texHeight);

    switch (m_fitMode) {
        case FitMode::Cover: {
            // Llenar el 100% de la pantalla sin deformar
            float scale = std::max(scaleX, scaleY);
            this->setScaleX(scale);
            this->setScaleY(scale);
            break;
        }
        case FitMode::Contain: {
            // Ajustar dentro de la pantalla
            float scale = std::min(scaleX, scaleY);
            this->setScaleX(scale);
            this->setScaleY(scale);
            break;
        }
        case FitMode::Stretch: {
            // Estirar exactamente a los bordes
            this->setScaleX(scaleX);
            this->setScaleY(scaleY);
            break;
        }
        case FitMode::Center: {
            this->setScaleX(1.0f);
            this->setScaleY(1.0f);
            break;
        }
    }
}

void VideoSprite::setLoop(bool loop) {
    if (m_decoder) m_decoder->setLoop(loop);
}

bool VideoSprite::isLoop() const {
    return m_decoder ? m_decoder->isLoop() : true;
}

void VideoSprite::setVolume(float volume) {
    if (m_decoder) m_decoder->setVolume(volume);
}

float VideoSprite::getVolume() const {
    return m_decoder ? m_decoder->getVolume() : 1.0f;
}

void VideoSprite::setMute(bool mute) {
    if (m_decoder) m_decoder->setMute(mute);
}

bool VideoSprite::isMute() const {
    return m_decoder ? m_decoder->isMute() : false;
}

void VideoSprite::play() {
    if (m_decoder) m_decoder->play();
}

void VideoSprite::pause() {
    if (m_decoder) m_decoder->pause();
}

void VideoSprite::stop() {
    if (m_decoder) m_decoder->stop();
}
