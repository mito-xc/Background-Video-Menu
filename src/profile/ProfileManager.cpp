#include "ProfileManager.hpp"
#include <Geode/binding/MenuGameLayer.hpp>
#include <Geode/binding/LevelSelectLayer.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/GJShopLayer.hpp>
#include <Geode/binding/SecretRewardsLayer.hpp>
#include <Geode/binding/GJGarageLayer.hpp>
#include <fstream>
#include <matjson.hpp>

using namespace geode::prelude;

ProfileManager* ProfileManager::get() {
    static ProfileManager instance;
    return &instance;
}

ProfileManager::ProfileManager() {
    m_configPath = Mod::get()->getConfigDir() / "profiles.json";
    load();
}

bool ProfileManager::isVideoExtension(const std::filesystem::path& path) const {
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext == ".mp4" || ext == ".mov" || ext == ".wmv" || ext == ".m4v" || ext == ".avi" || ext == ".webm";
}

void ProfileManager::load() {
    if (!std::filesystem::exists(m_configPath)) {
        return;
    }

    try {
        std::ifstream file(m_configPath);
        if (!file.is_open()) return;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        auto parseRes = matjson::parse(content);
        if (!parseRes.isOk()) return;

        auto root = parseRes.unwrap();
        if (!root.isObject()) return;

        if (root.contains("language") && root["language"].isNumber()) {
            m_language = static_cast<Language>(root["language"].asInt().unwrapOr(0));
        }

        for (size_t i = 0; i < static_cast<size_t>(SceneType::COUNT); ++i) {
            std::string key = std::to_string(i);
            if (root.contains(key) && root[key].isObject()) {
                auto& pObj = root[key];
                auto& prof = m_profiles[i];

                if (pObj.contains("bgType") && pObj["bgType"].isNumber()) {
                    prof.bgType = static_cast<BgType>(pObj["bgType"].asInt().unwrapOr(0));
                }
                if (pObj.contains("filePath") && pObj["filePath"].isString()) {
                    prof.filePath = pObj["filePath"].asString().unwrapOr("");
                }
                if (pObj.contains("fitMode") && pObj["fitMode"].isNumber()) {
                    prof.fitMode = static_cast<FitMode>(pObj["fitMode"].asInt().unwrapOr(0));
                }
                if (pObj.contains("opacity") && pObj["opacity"].isNumber()) {
                    prof.opacity = static_cast<float>(pObj["opacity"].asDouble().unwrapOr(1.0));
                }
                if (pObj.contains("posX") && pObj["posX"].isNumber()) {
                    prof.posX = static_cast<float>(pObj["posX"].asDouble().unwrapOr(0.0));
                }
                if (pObj.contains("posY") && pObj["posY"].isNumber()) {
                    prof.posY = static_cast<float>(pObj["posY"].asDouble().unwrapOr(0.0));
                }
                if (pObj.contains("zoom") && pObj["zoom"].isNumber()) {
                    prof.zoom = static_cast<float>(pObj["zoom"].asDouble().unwrapOr(1.0));
                }
                if (pObj.contains("hidePlayers") && pObj["hidePlayers"].isBool()) {
                    prof.hidePlayers = pObj["hidePlayers"].asBool().unwrapOr(false);
                }
                if (pObj.contains("hideGround") && pObj["hideGround"].isBool()) {
                    prof.hideGround = pObj["hideGround"].asBool().unwrapOr(false);
                }
                if (pObj.contains("transparentLayers") && pObj["transparentLayers"].isBool()) {
                    prof.transparentLayers = pObj["transparentLayers"].asBool().unwrapOr(true);
                }
                if (pObj.contains("loop") && pObj["loop"].isBool()) {
                    prof.loop = pObj["loop"].asBool().unwrapOr(true);
                }
                if (pObj.contains("muteAudio") && pObj["muteAudio"].isBool()) {
                    prof.muteAudio = pObj["muteAudio"].asBool().unwrapOr(true);
                }
                if (pObj.contains("volume") && pObj["volume"].isNumber()) {
                    prof.volume = static_cast<float>(pObj["volume"].asDouble().unwrapOr(1.0));
                }
            }
        }
    } catch (const std::exception& e) {
        log::error("Error al cargar perfiles: {}", e.what());
    }
}

void ProfileManager::save() {
    try {
        matjson::Value root = matjson::Value::object();
        root["language"] = static_cast<int>(m_language);

        for (size_t i = 0; i < static_cast<size_t>(SceneType::COUNT); ++i) {
            const auto& prof = m_profiles[i];
            matjson::Value pObj = matjson::Value::object();

            pObj["bgType"] = static_cast<int>(prof.bgType);
            pObj["filePath"] = prof.filePath;
            pObj["fitMode"] = static_cast<int>(prof.fitMode);
            pObj["opacity"] = prof.opacity;
            pObj["posX"] = prof.posX;
            pObj["posY"] = prof.posY;
            pObj["zoom"] = prof.zoom;
            pObj["hidePlayers"] = prof.hidePlayers;
            pObj["hideGround"] = prof.hideGround;
            pObj["transparentLayers"] = prof.transparentLayers;
            pObj["loop"] = prof.loop;
            pObj["muteAudio"] = prof.muteAudio;
            pObj["volume"] = prof.volume;

            root[std::to_string(i)] = pObj;
        }

        std::ofstream file(m_configPath);
        if (file.is_open()) {
            file << root.dump(matjson::NO_INDENTATION);
        }
    } catch (const std::exception& e) {
        log::error("Error al guardar perfiles: {}", e.what());
    }
}

SceneProfile& ProfileManager::getProfile(SceneType scene) {
    return m_profiles[static_cast<size_t>(scene)];
}

void ProfileManager::setProfile(SceneType scene, const SceneProfile& profile) {
    m_profiles[static_cast<size_t>(scene)] = profile;
    save();
}

cocos2d::CCNode* ProfileManager::createBackgroundNode(SceneType scene, const cocos2d::CCSize& targetSize) {
    const auto& prof = getProfile(scene);

    if (prof.bgType == BgType::DefaultGame || prof.filePath.empty()) {
        return nullptr;
    }

    std::filesystem::path fullPath = prof.filePath;
    if (!std::filesystem::exists(fullPath)) {
        auto resolved = CCFileUtils::get()->fullPathForFilename(prof.filePath.c_str(), 0);
        if (!resolved.empty()) {
            fullPath = std::string(resolved);
        }
    }

    if (!std::filesystem::exists(fullPath)) {
        log::warn("Archivo no encontrado: {}", prof.filePath);
        return nullptr;
    }

    // 1. Nodo de Video con escalado a pantalla completa
    if (prof.bgType == BgType::Video || isVideoExtension(fullPath)) {
        auto videoSprite = VideoSprite::create(fullPath);
        if (videoSprite) {
            videoSprite->setFitMode(prof.fitMode);
            videoSprite->setTargetSize(targetSize);
            videoSprite->setLoop(prof.loop);
            videoSprite->setVolume(prof.volume);
            videoSprite->setMute(prof.muteAudio);
            videoSprite->setOpacity(static_cast<GLubyte>(std::clamp(prof.opacity, 0.0f, 1.0f) * 255.0f));
            videoSprite->setPosition(targetSize * 0.5f + CCPoint{prof.posX, prof.posY});
            if (prof.zoom != 1.0f) {
                videoSprite->setScale(videoSprite->getScale() * prof.zoom);
            }
            return videoSprite;
        }
    }
    // 2. Sprite estándar (Imagen / GIF) con escalado completo
    else {
        auto sprite = CCSprite::create(fullPath.string().c_str());
        if (sprite) {
            sprite->setOpacity(static_cast<GLubyte>(std::clamp(prof.opacity, 0.0f, 1.0f) * 255.0f));
            sprite->setPosition(targetSize * 0.5f + CCPoint{prof.posX, prof.posY});

            auto size = sprite->getContentSize();
            if (size.width > 0 && size.height > 0) {
                float sx = targetSize.width / size.width;
                float sy = targetSize.height / size.height;
                float s = (prof.fitMode == FitMode::Cover) ? std::max(sx, sy) :
                          (prof.fitMode == FitMode::Contain) ? std::min(sx, sy) : sx;

                if (prof.fitMode == FitMode::Stretch) {
                    sprite->setScaleX(sx * prof.zoom);
                    sprite->setScaleY(sy * prof.zoom);
                } else {
                    sprite->setScale(s * prof.zoom);
                }
            }
            return sprite;
        }
    }

    return nullptr;
}

// Recarga instantánea del fondo en la escena actual
void reloadCurrentSceneBackground() {
    auto currentScene = CCDirector::sharedDirector()->getRunningScene();
    if (!currentScene) return;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // 1. Menú Principal
    if (auto menuGameLayer = currentScene->getChildByType<MenuGameLayer>(0)) {
        while (auto oldBg = menuGameLayer->getChildByID("custom-menu-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::MainMenu, winSize);
        if (newBg) {
            newBg->setID("custom-menu-background"_spr);
            menuGameLayer->addChild(newBg, -999);
        }
        auto& prof = ProfileManager::get()->getProfile(SceneType::MainMenu);
        if (prof.hideGround) {
            if (menuGameLayer->m_groundLayer) {
                menuGameLayer->m_groundLayer->setVisible(false);
                menuGameLayer->m_groundLayer->setScale(0.f);
                menuGameLayer->m_groundLayer->setPosition({ -9999.f, -9999.f });
            }
            if (auto ground = menuGameLayer->getChildByType<GJGroundLayer>(0)) {
                ground->setVisible(false);
                ground->setScale(0.f);
                ground->setPosition({ -9999.f, -9999.f });
            }
        } else {
            if (menuGameLayer->m_groundLayer) {
                menuGameLayer->m_groundLayer->setVisible(true);
                menuGameLayer->m_groundLayer->setScale(1.f);
                menuGameLayer->m_groundLayer->setPosition({ 0.f, 0.f });
            }
            if (auto ground = menuGameLayer->getChildByType<GJGroundLayer>(0)) {
                ground->setVisible(true);
                ground->setScale(1.f);
                ground->setPosition({ 0.f, 0.f });
            }
        }
        if (prof.hidePlayers) {
            if (menuGameLayer->m_playerObject) {
                menuGameLayer->m_playerObject->setVisible(false);
                menuGameLayer->m_playerObject->setPosition({ -9999.f, -9999.f });
            }
            if (auto children = menuGameLayer->getChildren()) {
                for (unsigned int i = 0; i < children->count(); ++i) {
                    if (auto child = dynamic_cast<CCNode*>(children->objectAtIndex(i))) {
                        if (child->getID() != "custom-menu-background"_spr) {
                            if (auto ps = dynamic_cast<CCParticleSystem*>(child)) {
                                ps->stopSystem();
                                ps->resetSystem();
                                ps->setVisible(false);
                            } else if (dynamic_cast<PlayerObject*>(child)) {
                                child->setVisible(false);
                            }
                        }
                    }
                }
            }
        }
    }
    // 2. RobTop Levels
    if (auto levelSelect = currentScene->getChildByType<LevelSelectLayer>(0)) {
        while (auto oldBg = levelSelect->getChildByID("custom-levelselect-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::LevelSelect, winSize);
        if (newBg) {
            newBg->setID("custom-levelselect-background"_spr);
            levelSelect->addChild(newBg, -999);
        }
    }
    // 3. Online Level Browser
    if (auto browser = currentScene->getChildByType<LevelBrowserLayer>(0)) {
        while (auto oldBg = browser->getChildByID("custom-levelbrowser-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::LevelBrowser, winSize);
        if (newBg) {
            newBg->setID("custom-levelbrowser-background"_spr);
            browser->addChild(newBg, -999);
        }
    }
    // 4. Tiendas
    if (auto shop = currentScene->getChildByType<GJShopLayer>(0)) {
        while (auto oldBg = shop->getChildByID("custom-shop-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::Shop, winSize);
        if (newBg) {
            newBg->setID("custom-shop-background"_spr);
            shop->addChild(newBg, -999);
        }
    }
    // 5. Secret Rewards
    if (auto vault = currentScene->getChildByType<SecretRewardsLayer>(0)) {
        while (auto oldBg = vault->getChildByID("custom-chests-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::Chests, winSize);
        if (newBg) {
            newBg->setID("custom-chests-background"_spr);
            vault->addChild(newBg, -999);
        }
    }
    // 6. Garage
    if (auto garage = currentScene->getChildByType<GJGarageLayer>(0)) {
        while (auto oldBg = garage->getChildByID("custom-garage-background"_spr)) {
            oldBg->removeFromParentAndCleanup(true);
        }
        auto newBg = ProfileManager::get()->createBackgroundNode(SceneType::Garage, winSize);
        if (newBg) {
            newBg->setID("custom-garage-background"_spr);
            garage->addChild(newBg, -999);
        }
    }
}
