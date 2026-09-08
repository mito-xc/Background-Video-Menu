#include "BackgroundPopup.hpp"
#include "FramingPopup.hpp"
#include "UIHelpers.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/binding/Slider.hpp>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#endif

using namespace geode::prelude;

BackgroundPopup* BackgroundPopup::create(SceneType initialScene) {
    auto ret = new BackgroundPopup();
    if (ret && ret->init(395.f, 260.f, initialScene)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

static void promptNativeFilePicker(std::function<void(std::string)> callback) {
#ifdef _WIN32
    std::thread([callback = std::move(callback)]() {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        IFileOpenDialog* pFileOpen = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen)))) {
            COMDLG_FILTERSPEC rgSpec[] = {
                { L"Media Files (*.mp4, *.mov, *.wmv, *.png, *.jpg, *.gif)", L"*.mp4;*.mov;*.wmv;*.m4v;*.avi;*.png;*.jpg;*.jpeg;*.gif;*.webp;*.jxl;*.qoi" },
                { L"Video Files (*.mp4, *.mov, *.wmv)", L"*.mp4;*.mov;*.wmv;*.m4v;*.avi" },
                { L"Image Files (*.png, *.jpg, *.gif)", L"*.png;*.jpg;*.jpeg;*.gif;*.webp;*.jxl;*.qoi" },
                { L"All Files (*.*)", L"*.*" }
            };
            pFileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
            if (SUCCEEDED(pFileOpen->Show(NULL))) {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                    PWSTR pszFilePath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        if (size_needed > 1) {
                            std::string strPath(size_needed - 1, 0);
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &strPath[0], size_needed, NULL, NULL);
                            geode::queueInMainThread([callback, strPath]() {
                                callback(strPath);
                            });
                        }
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }).detach();
#endif
}

bool BackgroundPopup::init(float width, float height, SceneType initialScene) {
    if (!Popup::init(width, height, "GJ_square01.png")) {
        return false;
    }

    m_currentScene = initialScene;
    this->setTitle("Menu Background", "goldFont.fnt", 0.7f);

    auto winSize = m_mainLayer->getContentSize();

    // ----------------------------------------------------
    // Botón de Cambio de Idioma (Top Right / Esquina Superior)
    // ----------------------------------------------------
    auto langMenu = CCMenu::create();
    langMenu->setPosition({0, 0});
    m_mainLayer->addChild(langMenu, 20);

    auto lang = ProfileManager::get()->getLanguage();
    const char* langStr = (lang == Language::Spanish) ? "ES" : "EN";
    auto langBtn = UIHelpers::createPillButton(
        langStr,
        {36.f, 20.f},
        {10, 20, 32},
        {0, 180, 220},
        {0, 225, 255},
        0.4f,
        this,
        menu_selector(BackgroundPopup::onToggleLanguage),
        "chatFont.fnt"
    );
    langBtn->setPosition({winSize.width - 32.f, winSize.height - 20.f});
    langBtn->setTag(777);
    langMenu->addChild(langBtn);

    // ----------------------------------------------------
    // Barra de Pestañas Superiores / Top Tab Bar
    // ----------------------------------------------------
    m_tabsMenu = CCMenu::create();
    m_tabsMenu->setPosition({winSize.width / 2.f, winSize.height - 44.f});
    m_mainLayer->addChild(m_tabsMenu, 10);

    // Menú de contenidos y tarjetas
    m_contentMenu = CCMenu::create();
    m_contentMenu->setPosition({0, 0});
    m_mainLayer->addChild(m_contentMenu, 10);

    refreshContent();
    return true;
}

void BackgroundPopup::refreshContent() {
    m_contentMenu->removeAllChildrenWithCleanup(true);
    m_tabsMenu->removeAllChildrenWithCleanup(true);
    m_mainLayer->removeChildByTag(8888);
    m_mainLayer->removeChildByTag(8889);

    auto winSize = m_mainLayer->getContentSize();
    auto& profile = ProfileManager::get()->getProfile(m_currentScene);
    auto lang = ProfileManager::get()->getLanguage();

    // ----------------------------------------------------
    // Actualizar Pestañas Superiores (Tabs)
    // ----------------------------------------------------
    const char* tabNames[] = { "Main", "Shop", "Vault", "RobTop", "Online", "Garage" };
    float tabStartX = -150.f;
    for (int i = 0; i < 6; ++i) {
        bool isCurrent = (static_cast<int>(m_currentScene) == i);
        
        CCMenuItemSpriteExtra* tabBtn = nullptr;
        if (isCurrent) {
            // Pestaña Activa: Relleno Cian brillante con texto blanco
            tabBtn = UIHelpers::createPillButton(
                tabNames[i],
                {56.f, 24.f},
                {0, 185, 215},
                {0, 230, 255},
                {255, 255, 255},
                0.45f,
                this,
                menu_selector(BackgroundPopup::onTabClicked),
                "goldFont.fnt"
            );
        } else {
            // Pestaña Inactiva: Fondo oscuro con borde cian tenue
            tabBtn = UIHelpers::createPillButton(
                tabNames[i],
                {56.f, 24.f},
                {10, 16, 26},
                {25, 60, 90},
                {200, 220, 240},
                0.45f,
                this,
                menu_selector(BackgroundPopup::onTabClicked),
                "goldFont.fnt"
            );
        }

        tabBtn->setTag(i);
        tabBtn->setPositionX(tabStartX + i * 60.f);
        m_tabsMenu->addChild(tabBtn);
    }

    // ----------------------------------------------------
    // TARJETA 1: Archivo y Acciones (Media Card)
    // ----------------------------------------------------
    auto card1Bg = UIHelpers::createCardBackground({365.f, 48.f}, {25, 45, 65});
    card1Bg->setPosition({winSize.width / 2.f, winSize.height - 88.f});
    card1Bg->setTag(8888);
    m_mainLayer->addChild(card1Bg, 1);

    // Caja de texto de nombre de archivo
    auto inputBg = CCScale9Sprite::create("square02_001.png");
    inputBg->setContentSize({155.f, 28.f});
    inputBg->setColor({5, 8, 14});
    inputBg->setOpacity(255);
    inputBg->setPosition({winSize.width / 2.f - 95.f, winSize.height - 88.f});
    m_mainLayer->addChild(inputBg, 2);

    std::string displayFileName = (lang == Language::Spanish) ? "Fondo por defecto..." : "Default Background...";
    if (profile.bgType != BgType::DefaultGame && !profile.filePath.empty()) {
        std::filesystem::path p(profile.filePath);
        displayFileName = p.filename().string();
        if (displayFileName.length() > 20) {
            displayFileName = displayFileName.substr(0, 18) + "...";
        }
    }

    m_fileNameLabel = CCLabelBMFont::create(displayFileName.c_str(), "chatFont.fnt");
    m_fileNameLabel->setScale(0.68f);
    m_fileNameLabel->setColor({220, 240, 255});
    m_fileNameLabel->setAnchorPoint({0.f, 0.5f});
    m_fileNameLabel->setPosition({winSize.width / 2.f - 165.f, winSize.height - 88.f});
    m_contentMenu->addChild(m_fileNameLabel, 5);

    // Botones de Tarjeta 1
    // 1. [ 🔍 Buscar / Browse ]
    const char* browseText = (lang == Language::Spanish) ? "Buscar" : "Browse";
    auto browseBtn = UIHelpers::createPillButton(
        browseText,
        {66.f, 28.f},
        {10, 18, 28},
        {0, 180, 220},
        {0, 225, 255},
        0.52f,
        this,
        menu_selector(BackgroundPopup::onBrowseFileClicked),
        "chatFont.fnt"
    );
    // Añadir icono de lupa si es posible
    if (auto findSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png")) {
        findSpr->setScale(0.38f);
        findSpr->setPosition({14.f, 14.f});
        browseBtn->getNormalImage()->addChild(findSpr, 10);
        // Desplazar texto ligeramente a la derecha
        if (auto lbl = browseBtn->getNormalImage()->getChildren()) {
            for (unsigned int k = 0; k < lbl->count(); ++k) {
                if (auto lNode = dynamic_cast<CCLabelBMFont*>(lbl->objectAtIndex(k))) {
                    lNode->setPositionX(38.f);
                }
            }
        }
    }
    browseBtn->setPosition({winSize.width / 2.f + 32.f, winSize.height - 88.f});
    m_contentMenu->addChild(browseBtn);

    // 2. [ Ajustar / Adjust ]
    const char* adjustText = (lang == Language::Spanish) ? "Ajustar" : "Adjust";
    auto adjustBtn = UIHelpers::createPillButton(
        adjustText,
        {62.f, 28.f},
        {10, 18, 28},
        {0, 180, 220},
        {0, 225, 255},
        0.52f,
        this,
        menu_selector(BackgroundPopup::onOpenFramingPopup),
        "chatFont.fnt"
    );
    adjustBtn->setPosition({winSize.width / 2.f + 102.f, winSize.height - 88.f});
    m_contentMenu->addChild(adjustBtn);

    // 3. [ ✕ ] Botón simétrico rojo redondeado
    auto clearBtn = UIHelpers::createPillButton(
        "X",
        {28.f, 28.f},
        {35, 10, 15},
        {220, 50, 60},
        {255, 70, 80},
        0.55f,
        this,
        menu_selector(BackgroundPopup::onClearFileClicked),
        "bigFont.fnt"
    );
    clearBtn->setPosition({winSize.width / 2.f + 154.f, winSize.height - 88.f});
    m_contentMenu->addChild(clearBtn);

    // ----------------------------------------------------
    // TARJETA 2: Opciones y Ajustes (Settings Card)
    // ----------------------------------------------------
    auto card2Bg = UIHelpers::createCardBackground({365.f, 74.f}, {25, 45, 65});
    card2Bg->setPosition({winSize.width / 2.f, winSize.height - 158.f});
    card2Bg->setTag(8889);
    m_mainLayer->addChild(card2Bg, 1);

    // Fila 1: Slider de Opacidad
    const char* opText = (lang == Language::Spanish) ? "Opacidad:" : "Opacity:";
    auto opLabel = CCLabelBMFont::create(opText, "chatFont.fnt");
    opLabel->setScale(0.68f);
    opLabel->setColor({255, 255, 255});
    opLabel->setAnchorPoint({0.f, 0.5f});
    opLabel->setPosition({winSize.width / 2.f - 165.f, winSize.height - 138.f});
    m_contentMenu->addChild(opLabel);

    m_opacitySlider = Slider::create(this, menu_selector(BackgroundPopup::onOpacitySliderChanged), 0.58f);
    m_opacitySlider->setPosition({winSize.width / 2.f + 5.f, winSize.height - 138.f});
    m_opacitySlider->setValue(profile.opacity);
    m_contentMenu->addChild(m_opacitySlider);

    int opPercent = static_cast<int>(profile.opacity * 100.f);
    m_opacityPercentLabel = CCLabelBMFont::create((std::to_string(opPercent) + "%").c_str(), "chatFont.fnt");
    m_opacityPercentLabel->setScale(0.68f);
    m_opacityPercentLabel->setColor({255, 255, 255});
    m_opacityPercentLabel->setPosition({winSize.width / 2.f + 145.f, winSize.height - 138.f});
    m_contentMenu->addChild(m_opacityPercentLabel);

    // Fila 2: Casillas de Verificación (Checkboxes)
    float toggleY = winSize.height - 174.f;

    // 1. Silenciar Video (Mute)
    auto muteToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(BackgroundPopup::onToggleMute), 0.55f
    );
    muteToggle->toggle(profile.muteAudio);
    muteToggle->setPosition({winSize.width / 2.f - 150.f, toggleY});
    m_contentMenu->addChild(muteToggle);

    const char* muteText = (lang == Language::Spanish) ? "Silenciar Video" : "Mute Video";
    auto muteLabel = CCLabelBMFont::create(muteText, "chatFont.fnt");
    muteLabel->setScale(0.62f);
    muteLabel->setAnchorPoint({0.f, 0.5f});
    muteLabel->setPosition({winSize.width / 2.f - 132.f, toggleY});
    m_contentMenu->addChild(muteLabel);

    // 2. Bucle (Loop)
    auto loopToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(BackgroundPopup::onToggleLoop), 0.55f
    );
    loopToggle->toggle(profile.loop);
    loopToggle->setPosition({winSize.width / 2.f - 24.f, toggleY});
    m_contentMenu->addChild(loopToggle);

    const char* loopText = (lang == Language::Spanish) ? "Bucle" : "Loop";
    auto loopLabel = CCLabelBMFont::create(loopText, "chatFont.fnt");
    loopLabel->setScale(0.62f);
    loopLabel->setAnchorPoint({0.f, 0.5f});
    loopLabel->setPosition({winSize.width / 2.f - 6.f, toggleY});
    m_contentMenu->addChild(loopLabel);

    // 3. Opciones Contextuales por Escena
    if (m_currentScene == SceneType::MainMenu) {
        auto playersToggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(BackgroundPopup::onToggleHidePlayers), 0.55f
        );
        playersToggle->toggle(profile.hidePlayers);
        playersToggle->setPosition({winSize.width / 2.f + 55.f, toggleY});
        m_contentMenu->addChild(playersToggle);

        const char* pText = (lang == Language::Spanish) ? "Ocultar Jugadores" : "Hide Players";
        auto playersLabel = CCLabelBMFont::create(pText, "chatFont.fnt");
        playersLabel->setScale(0.62f);
        playersLabel->setAnchorPoint({0.f, 0.5f});
        playersLabel->setPosition({winSize.width / 2.f + 73.f, toggleY});
        m_contentMenu->addChild(playersLabel);
    } else if (m_currentScene == SceneType::LevelSelect || m_currentScene == SceneType::LevelBrowser) {
        auto transToggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(BackgroundPopup::onToggleTransparentLayers), 0.55f
        );
        transToggle->toggle(profile.transparentLayers);
        transToggle->setPosition({winSize.width / 2.f + 50.f, toggleY});
        m_contentMenu->addChild(transToggle);

        const char* tText = (lang == Language::Spanish) ? "Capas Transparentes" : "Transparent UI";
        auto transLabel = CCLabelBMFont::create(tText, "chatFont.fnt");
        transLabel->setScale(0.62f);
        transLabel->setAnchorPoint({0.f, 0.5f});
        transLabel->setPosition({winSize.width / 2.f + 68.f, toggleY});
        m_contentMenu->addChild(transLabel);
    } else {
        auto groundToggle = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(BackgroundPopup::onToggleHideGround), 0.55f
        );
        groundToggle->toggle(profile.hideGround);
        groundToggle->setPosition({winSize.width / 2.f + 55.f, toggleY});
        m_contentMenu->addChild(groundToggle);

        const char* gText = (lang == Language::Spanish) ? "Ocultar Suelo" : "Hide Ground";
        auto groundLabel = CCLabelBMFont::create(gText, "chatFont.fnt");
        groundLabel->setScale(0.62f);
        groundLabel->setAnchorPoint({0.f, 0.5f});
        groundLabel->setPosition({winSize.width / 2.f + 73.f, toggleY});
        m_contentMenu->addChild(groundLabel);
    }

    // ----------------------------------------------------
    // Botón Inferior: [ Aplicar Cambios ] (Solid Cyan Primary Button)
    // ----------------------------------------------------
    const char* applyText = (lang == Language::Spanish) ? "Aplicar Cambios" : "Apply Changes";
    auto applyBtn = UIHelpers::createPillButton(
        applyText,
        {180.f, 32.f},
        {0, 185, 215},
        {0, 230, 255},
        {255, 255, 255},
        0.58f,
        this,
        menu_selector(BackgroundPopup::onApplyChanges),
        "goldFont.fnt"
    );
    applyBtn->setPosition({winSize.width / 2.f, 22.f});
    m_contentMenu->addChild(applyBtn);
}

void BackgroundPopup::onTabClicked(CCObject* sender) {
    auto btn = static_cast<CCNode*>(sender);
    m_currentScene = static_cast<SceneType>(btn->getTag());
    refreshContent();
}

void BackgroundPopup::onToggleLanguage(CCObject*) {
    auto cur = ProfileManager::get()->getLanguage();
    auto next = (cur == Language::Spanish) ? Language::English : Language::Spanish;
    ProfileManager::get()->setLanguage(next);

    // Actualizar botón de idioma
    if (auto langBtn = m_mainLayer->getChildByTag(777)) {
        // En refreshContent o recreación
    }
    refreshContent();
}

void BackgroundPopup::onBrowseFileClicked(CCObject*) {
    auto scene = m_currentScene;
    promptNativeFilePicker([this, scene](std::string pickedPath) {
        auto& prof = ProfileManager::get()->getProfile(scene);
        prof.filePath = pickedPath;
        if (ProfileManager::get()->isVideoExtension(pickedPath)) {
            prof.bgType = BgType::Video;
        } else {
            prof.bgType = BgType::ImageOrGif;
        }
        ProfileManager::get()->save();
        reloadCurrentSceneBackground();

        if (m_currentScene == scene) {
            this->refreshContent();
        }
    });
}

void BackgroundPopup::onClearFileClicked(CCObject*) {
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.bgType = BgType::DefaultGame;
    prof.filePath = "";
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
    refreshContent();
}

void BackgroundPopup::onOpenFramingPopup(CCObject*) {
    FramingPopup::create(m_currentScene)->show();
}

void BackgroundPopup::onToggleHidePlayers(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.hidePlayers = !toggle->isToggled();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}

void BackgroundPopup::onToggleHideGround(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.hideGround = !toggle->isToggled();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}

void BackgroundPopup::onToggleTransparentLayers(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.transparentLayers = !toggle->isToggled();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}

void BackgroundPopup::onToggleLoop(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.loop = !toggle->isToggled();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}

void BackgroundPopup::onToggleMute(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.muteAudio = !toggle->isToggled();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}

void BackgroundPopup::onOpacitySliderChanged(CCObject*) {
    if (!m_opacitySlider) return;
    auto& prof = ProfileManager::get()->getProfile(m_currentScene);
    prof.opacity = m_opacitySlider->getValue();
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();

    if (m_opacityPercentLabel) {
        int percent = static_cast<int>(prof.opacity * 100.f);
        m_opacityPercentLabel->setString((std::to_string(percent) + "%").c_str());
    }
}

void BackgroundPopup::onApplyChanges(CCObject*) {
    ProfileManager::get()->save();
    reloadCurrentSceneBackground();
}
