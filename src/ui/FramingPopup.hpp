#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "../profile/ProfileManager.hpp"

// ============================================================================
// Ventana interactiva de encuadre con doble recuadro: Canvas exterior con recorte (Clipping)
// y marco interior 16:9 con sombreado periférico (lo visible vs lo que no se ve)
// ============================================================================
class FramingPopup : public geode::Popup {
protected:
    SceneType m_scene{SceneType::MainMenu};
    cocos2d::CCNode* m_previewBgNode{nullptr};
    cocos2d::CCNode* m_viewportContainer{nullptr};
    cocos2d::CCClippingNode* m_clipNode{nullptr};
    cocos2d::CCLabelBMFont* m_infoLabel{nullptr};
    geode::TextInput* m_zoomInput{nullptr};

    // Dimensiones de la pantalla 16:9 interior y del lienzo exterior recortado
    cocos2d::CCSize m_screenSize{230.f, 129.375f};
    cocos2d::CCSize m_canvasSize{330.f, 148.f};

    bool m_isDragging{false};
    cocos2d::CCPoint m_lastTouchPos{0, 0};

    bool init(float width, float height, SceneType scene);
    void updatePreviewTransform();
    void refreshPreviewNode();
    void updateInfoText();

    // Eventos de botones
    void onZoomIn(cocos2d::CCObject* sender);
    void onZoomOut(cocos2d::CCObject* sender);
    void onResetPos(cocos2d::CCObject* sender);
    void onFitModeClicked(cocos2d::CCObject* sender);
    void onCloseBtn(cocos2d::CCObject* sender);

public:
    static FramingPopup* create(SceneType scene);

    // Manejo de toques y arrastre con el mouse
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void onExit() override;
};
