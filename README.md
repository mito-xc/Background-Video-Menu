# Background Video Menu 🎬

[![Geode Version](https://img.shields.io/badge/Geode-v5.10.1-brightgreen.svg)](https://geode-sdk.org/)
[![Geometry Dash](https://img.shields.io/badge/Geometry%20Dash-2.2081-blue.svg)](https://store.steampowered.com/app/322170/Geometry_Dash/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()

**Background Video Menu** es un mod de alto rendimiento para **Geometry Dash (Geode)** que permite reproducir **fondos de video (.mp4, .mov, .wmv, .webm)** e imágenes en alta resolución de forma fluida, con aceleración por hardware, encuadre interactivo con el mouse y soporte multiescena independiente.

---

## 🌟 Créditos y Reconocimientos / Credits

> [!IMPORTANT]
> Este proyecto está basado y construido sobre el mod original **[Menu-Background](https://github.com/lil2kki/Menu-Background)** creado por **[lil2kki](https://github.com/lil2kki)** (user95401).
> 
> - **Proyecto Original**: [lil2kki/Menu-Background](https://github.com/lil2kki/Menu-Background)
> - **Desarrollo del Motor de Video e Interfaz Interactiva**: [Mituxs](https://github.com/akarc) & lil2kki.

---

## ✨ Características Principales / Key Features

### 1. 🎬 Motor de Video de Alto Rendimiento (Hardware Accelerated)
- **Decodificación por Hardware**: Utiliza la API nativa de **Windows Media Foundation (WMF)** en un hilo asíncrono secundario para garantizar **0 caídas de FPS** durante el juego.
- **Transmisión de Textura OpenGL Directa**: Los fotogramas se envían a la GPU mediante `glTexSubImage2D` sin recargar texturas en memoria.
- **Formatos de Video Soportados**: `.mp4`, `.mov`, `.wmv`, `.webm`, `.m4v`, `.avi`.
- **Formatos de Imagen Soportados**: `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, `.jxl`, `.qoi`.

### 2. 🖱️ Visor de Encuadre Interactivo con el Ratón (`Framing Viewport`)
- **Arrastre Libre con Mouse**: Abre el menú de **"Ajustar"** y mueve el fondo directamente con el ratón dentro de una pantalla interactiva a proporción 16:9.
- **Zoom Rápido Editable**: Escribe el porcentaje de zoom directamente en la caja de texto (ej. `120%`, `80%`) o usa los botones rápidos `+` / `-`.
- **Modos de Ajuste Inteligentes**:
  - `Cover`: Llena toda la pantalla sin distorsión ni bandas negras.
  - `Contain`: Ajusta el fondo completo dentro de los límites de la pantalla.
  - `Stretch`: Estira el medio a los cuatro bordes de la ventana.
- **Centrado Inmediato**: Botón **"Centrar"** para restaurar el encuadre por defecto.

### 3. 🎵 Gestión de Audio Independiente
- **Mantén la Música del Juego**: Opción para silenciar el audio del video con un solo clic y conservar intacta la banda sonora oficial de Geometry Dash.
- **Control de Bucle (Loop)**: Reproducción continua y fluida de videos en bucle.

### 4. 🗂️ Soporte Multiescena Independiente
Configura fondos, encuadres, opacidades y modos distintos para cada sección del juego:
- 🏠 **Menú Principal (Main Menu)**
- 🛒 **Tiendas (Shops)**
- 🗝️ **Sala de Cofres / The Vault (Secret Rewards)**
- 🏆 **Niveles Oficiales de RobTop (Level Select)**
- 🌐 **Buscador de Niveles Online (Level Browser)**
- 🎨 **Personalización / Icon Kit (Garage)**

### 5. 🧹 Modo Limpieza Total (`Hide Players & UI Cleaner`)
- **Ocultación Completa de Jugadores**: Oculta cubos saltando en el menú y elimina totalmente los sistemas de partículas (`CCParticleSystem`) de estela y salto.
- **Fondo Puro a Pantalla Completa**: Oculta las franjas oscuras, gradientes por defecto y recuadros de RobTop para que el fondo se aprecie al 100% con total nitidez.
- **Capas Transparentes**: Opción para hacer transparentes los fondos de cartas en los niveles oficiales y online.

### 6. ⚡ Aplicación en Vivo (Live Reload)
- Todos los cambios se aplican y visualizan en tiempo real sin necesidad de reiniciar Geometry Dash ni cambiar de pantalla.

---

## 🏗️ Arquitectura Técnica / Technical Architecture

```text
Geometry Dash (Cocos2d-x 2.2081)
│
├── Hook MenuGameLayer / Scene Layers
│   └── VideoSprite / CCSprite Background (Z-Order: -999)
│
├── Video Engine (C++23)
│   ├── VideoDecoder (Windows Media Foundation / IMFSourceReader)
│   │   ├── Asynchronous Decoding Worker Thread
│   │   ├── Double-Buffered Frame Queue
│   │   └── Color Conversion: RGB32 / YUV -> RGBA8888
│   └── VideoSprite
│       └── GPU Upload via glTexSubImage2D on scheduleUpdate()
│
└── UI Native Integration (Geode Popup)
    ├── BackgroundPopup (Diseño compacto en tarjetas de 380x240)
    │   ├── Native Win32 IFileOpenDialog
    │   └── Scene Profile Management (profiles.json)
    └── FramingPopup (Encuadre interactivo 16:9 con Touch Dispatcher)
```

---

## 🛠️ Compilación y Desarrollo / Building

### Requisitos
- **Geode CLI** (v3.8.0 o superior)
- **Geode SDK** (v5.10.1)
- **Visual Studio 2022 Build Tools** (con soporte para C++23 y MSVC v143)
- **Geometry Dash 2.2081**

### Comando de Compilación (PowerShell)
```powershell
geode build
```

El archivo empaquetado `.geode` se generará en la carpeta `build/` y se instalará automáticamente en tu instalación de Geometry Dash vinculada.

---

## 📄 Licencia

Este proyecto se distribuye bajo la licencia original del proyecto **Menu-Background**. Consulta el archivo [LICENSE.txt](LICENSE.txt) para más detalles.
