#pragma once

#include <Geode/Geode.hpp>
#include "../video/VideoSprite.hpp"
#include <string>
#include <array>
#include <filesystem>

// ============================================================================
// Soporte de Idiomas (Español / Inglés)
// Localization Support
// ============================================================================
enum class Language {
    Spanish = 0,
    English = 1
};

// ============================================================================
// Tipos de Escenas soportadas
// Supported game scenes for customized backgrounds
// ============================================================================
enum class SceneType {
    MainMenu = 0,     // Menú Principal (MenuGameLayer)
    Shop,             // Tiendas (GJShopLayer)
    Chests,           // Sala de Cofres / Vault (SecretRewardsLayer)
    LevelSelect,      // Selector de niveles oficiales de RobTop (LevelSelectLayer)
    LevelBrowser,     // Buscador de niveles online (LevelBrowserLayer)
    Garage,           // Garage / Personalización de iconos (GJGarageLayer)
    COUNT
};

inline const char* getSceneName(SceneType type, Language lang = Language::Spanish) {
    if (lang == Language::Spanish) {
        switch (type) {
            case SceneType::MainMenu: return "Menú Principal";
            case SceneType::Shop: return "Tiendas";
            case SceneType::Chests: return "Sala de Cofres";
            case SceneType::LevelSelect: return "Niveles RobTop";
            case SceneType::LevelBrowser: return "Niveles Online";
            case SceneType::Garage: return "Personalización";
            default: return "Desconocido";
        }
    } else {
        switch (type) {
            case SceneType::MainMenu: return "Main Menu";
            case SceneType::Shop: return "Shop";
            case SceneType::Chests: return "Chests Room";
            case SceneType::LevelSelect: return "RobTop Levels";
            case SceneType::LevelBrowser: return "Level Browser";
            case SceneType::Garage: return "Garage";
            default: return "Unknown";
        }
    }
}

enum class BgType {
    DefaultGame = 0,  // Fondo original del juego
    ImageOrGif,       // Imagen estática o GIF animado
    Video             // Video (.mp4, .mov, .wmv, .m4v, etc.)
};

// ============================================================================
// Configuración de cada Perfil de Fondo
// Individual scene profile settings
// ============================================================================
struct SceneProfile {
    BgType bgType{BgType::DefaultGame};
    std::string filePath{""};
    FitMode fitMode{FitMode::Cover};
    float opacity{1.0f};

    // Ajuste de encuadre y posición / Position offset & Zoom
    float posX{0.0f};
    float posY{0.0f};
    float zoom{1.0f};

    // Opciones visuales avanzadas / Advanced visual options
    bool hidePlayers{false};        // Ocultar cubos del menú
    bool hideGround{false};         // Ocultar suelo
    bool transparentLayers{true};   // Hacer transparentes fondos de niveles

    // Opciones de audio y reproducción / Playback & Audio
    bool loop{true};
    bool muteAudio{true};
    float volume{1.0f};
};

// ============================================================================
// Gestor de Perfiles (Persistencia JSON y Creación de Nodos de Fondo)
// Profile Manager
// ============================================================================
class ProfileManager {
public:
    static ProfileManager* get();

    void load();
    void save();

    SceneProfile& getProfile(SceneType scene);
    void setProfile(SceneType scene, const SceneProfile& profile);

    Language getLanguage() const { return m_language; }
    void setLanguage(Language lang) { m_language = lang; save(); }

    // Instancia el nodo de fondo (VideoSprite o CCSprite) adaptado a pantalla completa
    // Instantiates the full-screen background node
    cocos2d::CCNode* createBackgroundNode(SceneType scene, const cocos2d::CCSize& targetSize);

    bool isVideoExtension(const std::filesystem::path& path) const;

private:
    ProfileManager();
    Language m_language{Language::Spanish};
    std::array<SceneProfile, static_cast<size_t>(SceneType::COUNT)> m_profiles;
    std::filesystem::path m_configPath;
};

// Función global para recargar instantáneamente el fondo de la pantalla activa
// Global function to immediately reload current active scene background
void reloadCurrentSceneBackground();
