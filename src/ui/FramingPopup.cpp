#include "FramingPopup.hpp"
#include "UIHelpers.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

using namespace geode::prelude;

FramingPopup* FramingPopup::create(SceneType scene) {
    auto ret = new FramingPopup();
    if (ret && ret->init(410.f, 295.f, scene)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool FramingPopup::init(float width, float height, SceneType scene) {
    if (!Popup::init(width, height, "GJ_square01.png")) {
        return false;
    }

    m_scene = scene;
    auto lang = ProfileManager::get()->getLanguage();
    const char* titleText = (lang == Language::Spanish) ? "Ajustar Encuadre y Zoom" : "Adjust Framing & Zoom";
    this->setTitle(titleText, "goldFont.fnt", 0.65f);

    auto winSize = m_mainLayer->getContentSize();
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    auto screenWinSize = CCDirector::sharedDirector()->getWinSize();

    // Texto de ayuda superior (bien separado del título)
    const char* hintText = (lang == Language::Spanish) 
        ? "Arrastra con el mouse para encuadrar tu pantalla" 
        : "Drag with mouse to frame your screen";
    auto hintLabel = CCLabelBMFont::create(hintText, "chatFont.fnt");
    hintLabel->setScale(0.48f);
    hintLabel->setColor({180, 205, 230});
    hintLabel->setPosition({winSize.width / 2.f, winSize.height - 42.f});
    m_mainLayer->addChild(hintLabel, 10);

    // ----------------------------------------------------
    // RECUADRO 1 (EXTERIOR): Canvas con Recorte (Clipping)
    // ----------------------------------------------------
    m_viewportContainer = CCNode::create();
    m_viewportContainer->setContentSize(m_canvasSize);
    m_viewportContainer->setAnchorPoint({0.5f, 0.5f});
    m_viewportContainer->setPosition({winSize.width / 2.f, winSize.height - 124.f});
    m_mainLayer->addChild(m_viewportContainer, 5);

    // Borde exterior elegante del canvas
    auto outerBorder = CCScale9Sprite::create("square02_001.png");
    outerBorder->setContentSize({m_canvasSize.width + 6.f, m_canvasSize.height + 6.f});
    outerBorder->setColor({25, 45, 65});
    outerBorder->setOpacity(240);
    outerBorder->setPosition({winSize.width / 2.f, winSize.height - 124.f});
    m_mainLayer->addChild(outerBorder, 4);

    // Máscara de recorte (Stencil) para que el video NUNCA se salga del canvas
    auto stencil = CCDrawNode::create();
    CCPoint canvasRect[4] = {
        {0, 0},
        {m_canvasSize.width, 0},
        {m_canvasSize.width, m_canvasSize.height},
        {0, m_canvasSize.height}
    };
    ccColor4F white = {1.f, 1.f, 1.f, 1.f};
    stencil->drawPolygon(canvasRect, 4, white, 0, white);

    m_clipNode = CCClippingNode::create(stencil);
    m_clipNode->setContentSize(m_canvasSize);
    m_clipNode->setAnchorPoint({0.f, 0.f});
    m_clipNode->setPosition({0.f, 0.f});
    m_viewportContainer->addChild(m_clipNode, 2);

    // Fondo del canvas interior
    auto canvasBg = CCLayerColor::create({8, 12, 18, 255}, m_canvasSize.width, m_canvasSize.height);
    m_clipNode->addChild(canvasBg, 1);

    // Contenedor de la vista previa del fondo (detrás de las máscaras)
    auto bgHolder = CCNode::create();
    bgHolder->setContentSize(m_canvasSize);
    bgHolder->setTag(111);
    m_clipNode->addChild(bgHolder, 2);

    refreshPreviewNode();

    // ----------------------------------------------------
    // RECUADRO 2 (INTERIOR): Pantalla GD 16:9 con Sombreado Periférico
    // ----------------------------------------------------
    float screenX = (m_canvasSize.width - m_screenSize.width) / 2.f;  // 50.f
    float screenY = (m_canvasSize.height - m_screenSize.height) / 2.f; // ~9.3f

    // Máscaras oscuras exteriores (Lo que NO se verá en la pantalla del juego)
    // 1. Franja Superior
    auto topMask = CCLayerColor::create({0, 0, 0, 190}, m_canvasSize.width, screenY);
    topMask->setPosition({0, screenY + m_screenSize.height});
    m_clipNode->addChild(topMask, 10);

    // 2. Franja Inferior
    auto btmMask = CCLayerColor::create({0, 0, 0, 190}, m_canvasSize.width, screenY);
    btmMask->setPosition({0, 0});
    m_clipNode->addChild(btmMask, 10);

    // 3. Franja Izquierda
    auto leftMask = CCLayerColor::create({0, 0, 0, 190}, screenX, m_screenSize.height);
    leftMask->setPosition({0, screenY});
    m_clipNode->addChild(leftMask, 10);

    // 4. Franja Derecha
    auto rightMask = CCLayerColor::create({0, 0, 0, 190}, screenX, m_screenSize.height);
    rightMask->setPosition({screenX + m_screenSize.width, screenY});
    m_clipNode->addChild(rightMask, 10);

    // Marco Rectangular Puro y Nítido de Color Celeste / Cian
    auto rectBorder = CCDrawNode::create();
    CCPoint rectPts[4] = {
        {screenX, screenY},
        {screenX + m_screenSize.width, screenY},
        {screenX + m_screenSize.width, screenY + m_screenSize.height},
        {screenX, screenY + m_screenSize.height}
    };
    ccColor4F cyanLine = {0.f, 0.9f, 1.f, 1.f};
    ccColor4F transFill = {0.f, 0.f, 0.f, 0.f};
    rectBorder->drawPolygon(rectPts, 4, transFill, 1.5f, cyanLine);
    m_clipNode->addChild(rectBorder, 15);

    // Etiqueta del área visible
    const char* viewLabelText = (lang == Language::Spanish) ? "[ Pantalla GD ]" : "[ GD Screen ]";
    auto viewLabel = CCLabelBMFont::create(viewLabelText, "chatFont.fnt");
    viewLabel->setScale(0.46f);
    viewLabel->setColor({0, 230, 255});
    viewLabel->setPosition({m_canvasSize.width / 2.f, screenY + m_screenSize.height - 8.f});
    m_clipNode->addChild(viewLabel, 16);

    // ----------------------------------------------------
    // Tarjeta Sombreada de Controles Inferiores
    // ----------------------------------------------------
    auto controlsCard = UIHelpers::createCardBackground({380.f, 72.f}, {25, 45, 65});
    controlsCard->setPosition({winSize.width / 2.f, 46.f});
    m_mainLayer->addChild(controlsCard, 1);

    auto controlsMenu = CCMenu::create();
    controlsMenu->setPosition({0, 0});
    m_mainLayer->addChild(controlsMenu, 10);

    // Fila 1: Zoom con entrada de texto directa
    auto zoomLabel = CCLabelBMFont::create("Zoom:", "chatFont.fnt");
    zoomLabel->setScale(0.62f);
    zoomLabel->setColor({255, 255, 255});
    zoomLabel->setAnchorPoint({0.f, 0.5f});
    zoomLabel->setPosition({winSize.width / 2.f - 170.f, 62.f});
    m_mainLayer->addChild(zoomLabel, 10);

    // Botón [-]
    auto zoomOutBtn = UIHelpers::createPillButton(
        "-",
        {22.f, 22.f},
        {10, 18, 28},
        {0, 180, 220},
        {0, 225, 255},
        0.55f,
        this,
        menu_selector(FramingPopup::onZoomOut),
        "bigFont.fnt"
    );
    zoomOutBtn->setPosition({winSize.width / 2.f - 118.f, 62.f});
    controlsMenu->addChild(zoomOutBtn);

    // Input Editable de Zoom
    m_zoomInput = TextInput::create(46.f, "100", "bigFont.fnt");
    m_zoomInput->setCommonFilter(CommonFilter::Uint);
    m_zoomInput->setMaxCharCount(4);
    m_zoomInput->setScale(0.65f);
    m_zoomInput->setPosition({winSize.width / 2.f - 80.f, 62.f});
    m_zoomInput->setString(std::to_string(static_cast<int>(prof.zoom * 100.f)));
    m_zoomInput->setCallback([this](std::string const& str) {
        try {
            if (!str.empty()) {
                int pct = std::stoi(str);
                if (pct >= 10 && pct <= 1000) {
                    auto& prof = ProfileManager::get()->getProfile(m_scene);
                    prof.zoom = pct / 100.0f;
                    ProfileManager::get()->save();
                    reloadCurrentSceneBackground();
                    refreshPreviewNode();
                    updateInfoText();
                }
            }
        } catch (...) {}
    });
    m_mainLayer->addChild(m_zoomInput, 10);

    auto pctSymbol = CCLabelBMFont::create("%", "chatFont.fnt");
    pctSymbol->setScale(0.62f);
    pctSymbol->setColor({200, 225, 245});
    pctSymbol->setPosition({winSize.width / 2.f - 52.f, 62.f});
    m_mainLayer->addChild(pctSymbol, 10);

    // Botón [+]
    auto zoomInBtn = UIHelpers::createPillButton(
        "+",
        {22.f, 22.f},
        {10, 18, 28},
        {0, 180, 220},
        {0, 225, 255},
        0.55f,
        this,
        menu_selector(FramingPopup::onZoomIn),
        "bigFont.fnt"
    );
    zoomInBtn->setPosition({winSize.width / 2.f - 32.f, 62.f});
    controlsMenu->addChild(zoomInBtn);

    // Modos de Ajuste (Cover, Contain, Stretch)
    const char* fitNames[] = { "Cover", "Contain", "Stretch" };
    for (int i = 0; i < 3; ++i) {
        bool selected = (static_cast<int>(prof.fitMode) == i);
        auto fitBtn = UIHelpers::createPillButton(
            fitNames[i],
            {52.f, 22.f},
            selected ? cocos2d::ccColor3B{0, 185, 215} : cocos2d::ccColor3B{10, 18, 28},
            selected ? cocos2d::ccColor3B{0, 230, 255} : cocos2d::ccColor3B{25, 55, 80},
            selected ? cocos2d::ccColor3B{255, 255, 255} : cocos2d::ccColor3B{180, 210, 235},
            0.45f,
            this,
            menu_selector(FramingPopup::onFitModeClicked),
            "chatFont.fnt"
        );
        fitBtn->setTag(i);
        fitBtn->setPosition({winSize.width / 2.f + 25.f + i * 54.f, 62.f});
        controlsMenu->addChild(fitBtn);
    }

    // Fila 2: Centrar, Posición y Listo
    const char* centerText = (lang == Language::Spanish) ? "Centrar" : "Center";
    auto centerBtn = UIHelpers::createPillButton(
        centerText,
        {64.f, 24.f},
        {10, 20, 32},
        {0, 180, 220},
        {0, 225, 255},
        0.5f,
        this,
        menu_selector(FramingPopup::onResetPos),
        "chatFont.fnt"
    );
    centerBtn->setPosition({winSize.width / 2.f - 125.f, 30.f});
    controlsMenu->addChild(centerBtn);

    // Información de coordenadas
    m_infoLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_infoLabel->setScale(0.58f);
    m_infoLabel->setColor({255, 255, 255});
    m_infoLabel->setPosition({winSize.width / 2.f - 5.f, 30.f});
    m_mainLayer->addChild(m_infoLabel, 10);
    updateInfoText();

    // Botón Listo / Guardar
    const char* doneText = (lang == Language::Spanish) ? "Listo" : "Done";
    auto doneBtn = UIHelpers::createPillButton(
        doneText,
        {74.f, 24.f},
        {0, 185, 215},
        {0, 230, 255},
        {255, 255, 255},
        0.52f,
        this,
        menu_selector(FramingPopup::onCloseBtn),
        "goldFont.fnt"
    );
    doneBtn->setPosition({winSize.width / 2.f + 125.f, 30.f});
    controlsMenu->addChild(doneBtn);

    return true;
}

void FramingPopup::refreshPreviewNode() {
    auto bgHolder = m_clipNode ? m_clipNode->getChildByTag(111) : nullptr;
    if (!bgHolder) return;

    bgHolder->removeAllChildrenWithCleanup(true);
    m_previewBgNode = nullptr;

    auto screenWinSize = CCDirector::sharedDirector()->getWinSize();
    m_previewBgNode = ProfileManager::get()->createBackgroundNode(m_scene, screenWinSize);
    if (m_previewBgNode) {
        float ratio = m_screenSize.width / screenWinSize.width;
        m_previewBgNode->setScale(m_previewBgNode->getScale() * ratio);
        bgHolder->addChild(m_previewBgNode, 1);
        updatePreviewTransform();
    } else {
        auto lang = ProfileManager::get()->getLanguage();
        const char* noFile = (lang == Language::Spanish) ? "Sin Fondo Seleccionado" : "No Background Selected";
        auto noFileLabel = CCLabelBMFont::create(noFile, "chatFont.fnt");
        noFileLabel->setScale(0.65f);
        noFileLabel->setColor({160, 180, 200});
        noFileLabel->setPosition({m_canvasSize.width / 2.f, m_canvasSize.height / 2.f});
        bgHolder->addChild(noFileLabel, 2);
    }
}

void FramingPopup::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
}

void FramingPopup::onExit() {
    CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
    Popup::onExit();
}

bool FramingPopup::ccTouchBegan(CCTouch* touch, CCEvent*) {
    if (!m_viewportContainer) return false;

    auto touchLoc = touch->getLocation();
    auto localPos = m_viewportContainer->convertToNodeSpace(touchLoc);
    auto size = m_viewportContainer->getContentSize();

    if (localPos.x >= 0.f && localPos.y >= 0.f && localPos.x <= size.width && localPos.y <= size.height) {
        m_isDragging = true;
        m_lastTouchPos = touchLoc;
        return true;
    }
    return false;
}

void FramingPopup::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (!m_isDragging) return;

    auto touchLoc = touch->getLocation();
    auto delta = touchLoc - m_lastTouchPos;
    m_lastTouchPos = touchLoc;

    auto screenWinSize = CCDirector::sharedDirector()->getWinSize();
    float ratio = m_screenSize.width / screenWinSize.width;

    if (ratio > 0.001f) {
        auto& prof = ProfileManager::get()->getProfile(m_scene);
        prof.posX += delta.x / ratio;
        prof.posY += delta.y / ratio;

        updatePreviewTransform();
        updateInfoText();

        ProfileManager::get()->save();
        reloadCurrentSceneBackground();
    }
}

void FramingPopup::ccTouchEnded(CCTouch*, CCEvent*) {
    m_isDragging = false;
}

void FramingPopup::ccTouchCancelled(CCTouch*, CCEvent*) {
    m_isDragging = false;
}

void FramingPopup::updatePreviewTransform() {
    if (!m_previewBgNode) return;

    auto screenWinSize = CCDirector::sharedDirector()->getWinSize();
    float ratio = m_screenSize.width / screenWinSize.width;
    auto& prof = ProfileManager::get()->getProfile(m_scene);

    CCPoint center = { m_canvasSize.width / 2.f, m_canvasSize.height / 2.f };
    m_previewBgNode->setPosition(center + CCPoint{ prof.posX * ratio, prof.posY * ratio });
}

void FramingPopup::updateInfoText() {
    if (!m_infoLabel) return;
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    auto lang = ProfileManager::get()->getLanguage();
    int px = static_cast<int>(prof.posX);
    int py = static_cast<int>(prof.posY);

    const char* prefix = (lang == Language::Spanish) ? "Pos: " : "Pos: ";
    std::string text = prefix + std::string("(") + std::to_string(px) + ", " + std::to_string(py) + ")";
    m_infoLabel->setString(text.c_str());
}

void FramingPopup::onZoomIn(CCObject*) {
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    prof.zoom = std::clamp(prof.zoom + 0.1f, 0.1f, 10.0f);
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();

    refreshPreviewNode();
    updateInfoText();

    if (m_zoomInput) {
        m_zoomInput->setString(std::to_string(static_cast<int>(prof.zoom * 100.f)), false);
    }
}

void FramingPopup::onZoomOut(CCObject*) {
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    prof.zoom = std::clamp(prof.zoom - 0.1f, 0.1f, 10.0f);
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();

    refreshPreviewNode();
    updateInfoText();

    if (m_zoomInput) {
        m_zoomInput->setString(std::to_string(static_cast<int>(prof.zoom * 100.f)), false);
    }
}

void FramingPopup::onResetPos(CCObject*) {
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    prof.posX = 0.0f;
    prof.posY = 0.0f;
    prof.zoom = 1.0f;
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();

    refreshPreviewNode();
    updateInfoText();

    if (m_zoomInput) {
        m_zoomInput->setString("100", false);
    }
}

void FramingPopup::onFitModeClicked(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_scene);
    prof.fitMode = static_cast<FitMode>(btn->getTag());
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();

    refreshPreviewNode();
}

void FramingPopup::onCloseBtn(CCObject*) {
    this->onClose(nullptr);
}
