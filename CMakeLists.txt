cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
    add_definitions(-DGLES_SILENCE_DEPRECATION)
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(main)

# Archivos fuente del mod (Video, Perfiles y UI nativa en Cocos2d)
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/video/VideoDecoder.cpp
    src/video/VideoSprite.cpp
    src/profile/ProfileManager.cpp
    src/ui/BackgroundPopup.cpp
    src/ui/FramingPopup.cpp
)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

# Librerías de aceleración de hardware y decodificación multimedia de Windows (WMF)
if (WIN32)
    target_link_libraries(${PROJECT_NAME}
        mfreadwrite
        mfplat
        mfuuid
        wmcodecdspuuid
        Propsys
    )
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    target_link_libraries(${PROJECT_NAME} "-framework CoreGraphics")
endif()
