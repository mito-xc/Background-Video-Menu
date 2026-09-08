#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include "../profile/ProfileManager.hpp"

using namespace geode::prelude;

namespace UIHelpers {

    // Crea un botón con estilo de píldora redondeada moderna y borde coloreado
    inline CCMenuItemSpriteExtra* createPillButton(
        const char* text,
        cocos2d::CCSize size,
        cocos2d::ccColor3B bgColor,
        cocos2d::ccColor3B borderColor,
        cocos2d::ccColor3B textColor,
        float fontScale,
        cocos2d::CCObject* target,
        cocos2d::SEL_MenuHandler selector,
        const char* font = "bigFont.fnt"
    ) {
        // Contenedor del sprite normal
        auto container = cocos2d::CCNode::create();
        container->setContentSize(size);
        container->setAnchorPoint({0.5f, 0.5f});

        // Capa de fondo exterior (Borde)
        auto border = CCScale9Sprite::create("square02_001.png");
        border->setContentSize(size);
        border->setColor(borderColor);
        border->setOpacity(255);
        border->setPosition(size * 0.5f);
        container->addChild(border, 1);

        // Capa de fondo interior (Relleno)
        auto inner = CCScale9Sprite::create("square02_001.png");
        inner->setContentSize({size.width - 2.5f, size.height - 2.5f});
        inner->setColor(bgColor);
        inner->setOpacity(255);
        inner->setPosition(size * 0.5f);
        container->addChild(inner, 2);

        // Texto centrado
        auto label = cocos2d::CCLabelBMFont::create(text, font);
        label->setScale(fontScale);
        label->setColor(textColor);
        label->setPosition(size * 0.5f);
        container->addChild(label, 3);

        auto btn = CCMenuItemSpriteExtra::create(container, target, selector);
        btn->setContentSize(size);
        return btn;
    }

    // Crea una tarjeta sombreada con fondo oscuro y borde sutil
    inline CCScale9Sprite* createCardBackground(cocos2d::CCSize size, cocos2d::ccColor3B borderColor = {20, 35, 50}) {
        auto card = CCScale9Sprite::create("square02_001.png");
        card->setContentSize(size);
        card->setColor(borderColor);
        card->setOpacity(240);

        auto inner = CCScale9Sprite::create("square02_001.png");
        inner->setContentSize({size.width - 2.5f, size.height - 2.5f});
        inner->setColor({8, 14, 22}); // Fondo oscuro profundo
        inner->setOpacity(240);
        inner->setPosition(size * 0.5f);
        card->addChild(inner, 1);

        return card;
    }

}
