#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "../profile/ProfileManager.hpp"

// ============================================================================
// Ventana emergente moderna con diseño en tarjetas, estilo cian y soporte multilenguaje
// Modern card-based UI with cyan styling and bilingual support (ES / EN)
// ============================================================================
class BackgroundPopup : public geode::Popup {
protected:
    SceneType m_currentScene{SceneType::MainMenu};

    cocos2d::CCLabelBMFont* m_fileNameLabel{nullptr};
    cocos2d::CCLabelBMFont* m_opacityPercentLabel{nullptr};
    cocos2d::CCLabelBMFont* m_volumePercentLabel{nullptr};
    cocos2d::CCMenu* m_contentMenu{nullptr};
    cocos2d::CCMenu* m_tabsMenu{nullptr};
    Slider* m_opacitySlider{nullptr};
    Slider* m_volumeSlider{nullptr};

    bool init(float width, float height, SceneType initialScene);
    void refreshContent();

    // Eventos
    void onTabClicked(cocos2d::CCObject* sender);
    void onBrowseFileClicked(cocos2d::CCObject* sender);
    void onClearFileClicked(cocos2d::CCObject* sender);
    void onOpenFramingPopup(cocos2d::CCObject* sender);
    void onToggleLanguage(cocos2d::CCObject* sender);
    void onToggleHidePlayers(cocos2d::CCObject* sender);
    void onToggleHideGround(cocos2d::CCObject* sender);
    void onToggleTransparentLayers(cocos2d::CCObject* sender);
    void onToggleLoop(cocos2d::CCObject* sender);
    void onToggleMute(cocos2d::CCObject* sender);
    void onOpacitySliderChanged(cocos2d::CCObject* sender);
    void onVolumeSliderChanged(cocos2d::CCObject* sender);
    void onApplyChanges(cocos2d::CCObject* sender);

public:
    static BackgroundPopup* create(SceneType initialScene = SceneType::MainMenu);
};
