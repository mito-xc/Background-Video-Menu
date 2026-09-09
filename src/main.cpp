#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "video/VideoSprite.hpp"
#include "profile/ProfileManager.hpp"
#include "ui/BackgroundPopup.hpp"

using namespace geode::prelude;

// ============================================================================
// Inicialización del Mod al cargar Geometry Dash
// Mod entrypoint and initialization
// ============================================================================
$on_mod(Loaded) {
    // Añadir rutas de configuración y recursos al sistema de archivos de Cocos
    for (auto path : {
        string::pathToString(getMod()->getConfigDir()),
        string::pathToString(getMod()->getSaveDir()),
        string::pathToString(getMod()->getTempDir())
    }) {
        CCFileUtils::get()->addPriorityPath(path.c_str());
    }

    // Cargar perfiles guardados del usuario
    ProfileManager::get()->load();
}

// ============================================================================
// Hook: MenuGameLayer (Menú Principal - Fondo 100% Pantalla Completa y Limpio)
// Hook for Main Menu background, removing default dark tiles & hiding players
// ============================================================================
#include <Geode/modify/MenuGameLayer.hpp>
class $modify(MenuGameLayerExt, MenuGameLayer) {
    void cleanGround() {
        if (m_groundLayer) {
            m_groundLayer->setVisible(false);
            m_groundLayer->setScale(0.f);
            m_groundLayer->setPosition({ -9999.f, -9999.f });
        }
        if (auto ground = this->getChildByType<GJGroundLayer>(0)) {
            ground->setVisible(false);
            ground->setScale(0.f);
            ground->setPosition({ -9999.f, -9999.f });
        }
        if (auto children = this->getChildren()) {
            for (unsigned int i = 0; i < children->count(); ++i) {
                if (auto g = dynamic_cast<GJGroundLayer*>(children->objectAtIndex(i))) {
                    g->setVisible(false);
                    g->setScale(0.f);
                    g->setPosition({ -9999.f, -9999.f });
                }
            }
        }
    }

    void cleanPlayersAndParticles() {
        if (m_playerObject) {
            m_playerObject->setVisible(false);
            m_playerObject->setPosition({ -9999.f, -9999.f });
        }
        if (auto children = this->getChildren()) {
            for (unsigned int i = 0; i < children->count(); ++i) {
                if (auto child = dynamic_cast<CCNode*>(children->objectAtIndex(i))) {
                    if (child->getID() != "custom-menu-background"_spr) {
                        if (auto ps = dynamic_cast<CCParticleSystem*>(child)) {
                            ps->stopSystem();
                            ps->resetSystem();
                            ps->setVisible(false);
                        } else if (dynamic_cast<PlayerObject*>(child)) {
                            child->setVisible(false);
                            child->setPosition({ -9999.f, -9999.f });
                        }
                    }
                }
            }
        }
    }

    bool init() {
        if (!MenuGameLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::MainMenu);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            // Ocultar los recuadros, sprites, gradientes y texturas por defecto
            // Hide default background sprite tiles, batch nodes and tint layers
            if (auto children = this->getChildren()) {
                for (unsigned int i = 0; i < children->count(); ++i) {
                    if (auto child = dynamic_cast<CCNode*>(children->objectAtIndex(i))) {
                        if (dynamic_cast<CCSprite*>(child) || 
                            dynamic_cast<CCSpriteBatchNode*>(child) ||
                            dynamic_cast<CCLayerColor*>(child) ||
                            dynamic_cast<CCLayerGradient*>(child)) {
                            child->setVisible(false);
                        }
                    }
                }
            }

            // Inyectar el fondo personalizado a pantalla completa
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::MainMenu, winSize);
            if (bgNode) {
                bgNode->setID("custom-menu-background"_spr);
                this->addChild(bgNode, -999);
            }

            // Ocultar suelo si está activado
            if (profile.hideGround) {
                cleanGround();
            }
        }

        // Ocultar cubos de jugadores y sus partículas si está activado
        if (profile.hidePlayers) {
            cleanPlayersAndParticles();
        }

        return true;
    }

    void update(float dt) {
        MenuGameLayer::update(dt);

        auto& profile = ProfileManager::get()->getProfile(SceneType::MainMenu);
        if (profile.hidePlayers) {
            cleanPlayersAndParticles();
        }
        if (profile.hideGround) {
            cleanGround();
        }
    }
};

// ============================================================================
// Hook: MenuLayer (Botón nativo en el Menú Principal para abrir la Configuración)
// Hook for Main Menu button opening the native Geode/Cocos settings popup
// ============================================================================
#include <Geode/modify/MenuLayer.hpp>
class $modify(CustomMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Añadir botón en el menú lateral derecho si está habilitado en la configuración
        if (Mod::get()->getSettingValue<bool>("SHOW_MENU_BUTTON")) {
            if (auto rightMenu = this->getChildByID("right-side-menu")) {
                auto spr = CCSprite::createWithSpriteFrameName("GJ_paintBtn_001.png");
                if (!spr) {
                    spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
                }
                if (spr) {
                    spr->setScale(0.7f);
                    auto btn = CCMenuItemSpriteExtra::create(
                        spr, this, menu_selector(CustomMenuLayer::onOpenBackgroundSettings)
                    );
                    btn->setID("background-settings-btn"_spr);
                    rightMenu->addChild(btn);
                    rightMenu->updateLayout();
                }
            }
        }

        return true;
    }

    void onOpenBackgroundSettings(CCObject*) {
        BackgroundPopup::create(SceneType::MainMenu)->show();
    }
};

// ============================================================================
// Hook: LevelSelectLayer (Niveles Oficiales de RobTop + Fondo Transparente)
// Hook for RobTop official levels with optional transparent background
// ============================================================================
#include <Geode/modify/LevelSelectLayer.hpp>
class $modify(CustomLevelSelectLayer, LevelSelectLayer) {
    bool init(int p0) {
        if (!LevelSelectLayer::init(p0)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::LevelSelect);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::LevelSelect, winSize);
            if (bgNode) {
                bgNode->setID("custom-levelselect-background"_spr);
                this->addChild(bgNode, -999);
            }

            // Fondo transparente en las cartas de niveles de RobTop
            if (profile.transparentLayers) {
                if (auto bg = this->getChildByType<CCSprite>(0)) {
                    bg->setVisible(false);
                }
                if (auto ground = this->getChildByType<GJGroundLayer>(0)) {
                    ground->setVisible(false);
                }
            }
        }

        return true;
    }
};

// ============================================================================
// Hook: LevelBrowserLayer (Buscador de Niveles Online + Fondo Transparente)
// Hook for Online Level Browser with optional transparent background
// ============================================================================
#include <Geode/modify/LevelBrowserLayer.hpp>
class $modify(CustomLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* p0) {
        if (!LevelBrowserLayer::init(p0)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::LevelBrowser);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::LevelBrowser, winSize);
            if (bgNode) {
                bgNode->setID("custom-levelbrowser-background"_spr);
                this->addChild(bgNode, -999);
            }

            // Fondo transparente en el buscador de niveles
            if (profile.transparentLayers) {
                if (auto bg = this->getChildByType<CCSprite>(0)) {
                    bg->setVisible(false);
                }
            }
        }

        return true;
    }
};

// ============================================================================
// Hook: GJShopLayer (Tiendas)
// Hook for Shop Scene
// ============================================================================
#include <Geode/modify/GJShopLayer.hpp>
class $modify(CustomShopLayer, GJShopLayer) {
    bool init(ShopType p0) {
        if (!GJShopLayer::init(p0)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::Shop);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::Shop, winSize);
            if (bgNode) {
                bgNode->setID("custom-shop-background"_spr);
                this->addChild(bgNode, -999);
            }
        }

        return true;
    }
};

// ============================================================================
// Hook: SecretRewardsLayer (Sala de Cofres / The Vault)
// Hook for Chests Room / The Vault
// ============================================================================
#include <Geode/modify/SecretRewardsLayer.hpp>
class $modify(CustomSecretRewardsLayer, SecretRewardsLayer) {
    bool init(bool p0) {
        if (!SecretRewardsLayer::init(p0)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::Chests);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::Chests, winSize);
            if (bgNode) {
                bgNode->setID("custom-chests-background"_spr);
                this->addChild(bgNode, -999);
            }
        }

        return true;
    }
};

// ============================================================================
// Hook: GJGarageLayer (Icon Kit / Personalización)
// Hook for Garage / Icon Kit
// ============================================================================
#include <Geode/modify/GJGarageLayer.hpp>
class $modify(CustomGarageLayer, GJGarageLayer) {
    bool init() {
        if (!GJGarageLayer::init()) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& profile = ProfileManager::get()->getProfile(SceneType::Garage);

        if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
            auto bgNode = ProfileManager::get()->createBackgroundNode(SceneType::Garage, winSize);
            if (bgNode) {
                bgNode->setID("custom-garage-background"_spr);
                this->addChild(bgNode, -999);
            }
        }

        return true;
    }
};
