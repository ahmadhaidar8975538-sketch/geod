#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <string>

#include "BuildGenerator.hpp"

using namespace geode::prelude;

namespace {
    constexpr std::array<char const*, 10> DIFFICULTIES = {
        "Easy",
        "Normal",
        "Hard",
        "Harder",
        "Insane",
        "Easy Demon",
        "Medium Demon",
        "Hard Demon",
        "Insane Demon",
        "Extreme Demon",
    };

    CCLayerColor* makeButtonSprite(
        ccColor4B color,
        float width,
        float height,
        char const* text,
        float textScale
    ) {
        auto background = CCLayerColor::create(color, width, height);
        if (!background) {
            return nullptr;
        }

        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        if (!label) {
            return nullptr;
        }

        label->setScale(textScale);
        label->setPosition({ width / 2.f, height / 2.f });
        background->addChild(label);
        return background;
    }

    bool parsePositiveNumber(CCTextInputNode* input, double& value) {
        if (!input) {
            return false;
        }

        std::string const text(input->getString());
        if (text.empty()) {
            return false;
        }

        char* end = nullptr;
        value = std::strtod(text.c_str(), &end);

        return end != text.c_str()
            && *end == '\0'
            && std::isfinite(value)
            && value > 0.0;
    }
}

class AutoBuildLayoutPopup final : public Popup {
protected:
    geode::Ref<LevelEditorLayer> m_editor;
    CCLabelBMFont* m_difficultyLabel = nullptr;
    CCTextInputNode* m_bpmInput = nullptr;
    CCTextInputNode* m_lengthInput = nullptr;
    int m_difficulty = 0;

    bool init(LevelEditorLayer* editor) {
        if (!editor || !Popup::init(320.f, 245.f)) {
            return false;
        }

        m_editor = editor;
        this->setTitle("AutoBuildLayout");

        auto difficultyHeading = CCLabelBMFont::create("Difficulty", "bigFont.fnt");
        difficultyHeading->setScale(.45f);
        difficultyHeading->setAnchorPoint({ 0.f, .5f });
        difficultyHeading->setPosition({ 36.f, 169.f });
        m_mainLayer->addChild(difficultyHeading);

        m_difficultyLabel = CCLabelBMFont::create(
            DIFFICULTIES[m_difficulty],
            "bigFont.fnt"
        );
        m_difficultyLabel->setScale(.48f);
        m_difficultyLabel->setPosition({ 183.f, 169.f });
        m_mainLayer->addChild(m_difficultyLabel);

        auto previous = CCLabelBMFont::create("<", "bigFont.fnt");
        previous->setScale(.7f);
        auto previousButton = CCMenuItemSpriteExtra::create(
            previous,
            this,
            menu_selector(AutoBuildLayoutPopup::onPreviousDifficulty)
        );
        previousButton->setPosition({ 112.f, 169.f });
        m_buttonMenu->addChild(previousButton);

        auto next = CCLabelBMFont::create(">", "bigFont.fnt");
        next->setScale(.7f);
        auto nextButton = CCMenuItemSpriteExtra::create(
            next,
            this,
            menu_selector(AutoBuildLayoutPopup::onNextDifficulty)
        );
        nextButton->setPosition({ 254.f, 169.f });
        m_buttonMenu->addChild(nextButton);

        auto bpmHeading = CCLabelBMFont::create("BPM", "bigFont.fnt");
        bpmHeading->setScale(.45f);
        bpmHeading->setAnchorPoint({ 0.f, .5f });
        bpmHeading->setPosition({ 36.f, 119.f });
        m_mainLayer->addChild(bpmHeading);

        m_bpmInput = CCTextInputNode::create(
            120.f, 30.f, "120", "bigFont.fnt"
        );
        if (!m_bpmInput) {
            return false;
        }
        m_bpmInput->setAllowedChars("0123456789");
        m_bpmInput->setMaxLabelLength(3);
        m_bpmInput->setString("120");
        m_bpmInput->setPosition({ 216.f, 119.f });
        m_mainLayer->addChild(m_bpmInput);

        auto lengthHeading = CCLabelBMFont::create("Length (seconds)", "bigFont.fnt");
        lengthHeading->setScale(.4f);
        lengthHeading->setAnchorPoint({ 0.f, .5f });
        lengthHeading->setPosition({ 36.f, 76.f });
        m_mainLayer->addChild(lengthHeading);

        m_lengthInput = CCTextInputNode::create(
            120.f, 30.f, "30", "bigFont.fnt"
        );
        if (!m_lengthInput) {
            return false;
        }
        m_lengthInput->setAllowedChars("0123456789.");
        m_lengthInput->setMaxLabelLength(6);
        m_lengthInput->setString("30");
        m_lengthInput->setPosition({ 216.f, 76.f });
        m_mainLayer->addChild(m_lengthInput);

        auto createSprite = makeButtonSprite(
            { 55, 190, 105, 255 },
            112.f,
            34.f,
            "Create",
            .55f
        );
        if (!createSprite) {
            return false;
        }

        auto createButton = CCMenuItemSpriteExtra::create(
            createSprite,
            this,
            menu_selector(AutoBuildLayoutPopup::onCreate)
        );
        createButton->setPosition({ 160.f, 30.f });
        m_buttonMenu->addChild(createButton);

        return true;
    }

    void updateDifficultyLabel() {
        m_difficultyLabel->setString(DIFFICULTIES[m_difficulty]);
    }

    void onPreviousDifficulty(CCObject*) {
        m_difficulty = (
            m_difficulty + static_cast<int>(DIFFICULTIES.size()) - 1
        ) % static_cast<int>(DIFFICULTIES.size());

        this->updateDifficultyLabel();
    }

    void onNextDifficulty(CCObject*) {
        m_difficulty = (m_difficulty + 1)
            % static_cast<int>(DIFFICULTIES.size());

        this->updateDifficultyLabel();
    }

    void onCreate(CCObject*) {
        double bpm = 0.0;
        double lengthSeconds = 0.0;

        if (!parsePositiveNumber(m_bpmInput, bpm)
            || !parsePositiveNumber(m_lengthInput, lengthSeconds)
            || bpm < 30.0
            || bpm > 300.0
            || lengthSeconds < 5.0
            || lengthSeconds > 180.0) {
            FLAlertLayer::create(
                "Invalid settings",
                "Enter a BPM from 30 to 300 and a length from 5 to 180 seconds.",
                "OK"
            )->show();
            return;
        }

        std::size_t objectsPlaced = 0;
        if (!BuildGenerator::generate(
                m_editor,
                m_difficulty,
                bpm,
                lengthSeconds,
                objectsPlaced
            )) {
            FLAlertLayer::create(
                "Generation stopped",
                "The editor could not add an object. Some objects may already have been placed.",
                "OK"
            )->show();
            return;
        }

        this->onClose(nullptr);
        createQuickPopup(
            "Layout created",
            fmt::format(
                "Added {} spike objects. The layout uses the editor's default ground.",
                objectsPlaced
            ),
            "OK"
        );
    }

public:
    static AutoBuildLayoutPopup* create(LevelEditorLayer* editor) {
        auto popup = new AutoBuildLayoutPopup();
        if (popup && popup->init(editor)) {
            popup->autorelease();
            return popup;
        }

        CC_SAFE_DELETE(popup);
        return nullptr;
    }
};

class $modify(AutoBuildLayoutEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto menu = this->getChildByID("editor-buttons-menu");
        if (!menu) {
            log::warn("AutoBuildLayout: editor-buttons-menu was not found.");
            return true;
        }

        auto sprite = makeButtonSprite(
            { 145, 55, 205, 255 },
            34.f,
            34.f,
            "E",
            .65f
        );
        if (!sprite) {
            log::warn("AutoBuildLayout: failed to create the editor button.");
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(AutoBuildLayoutEditorUI::onAutoBuildLayout)
        );
        button->setID("autobuildlayout-button");
        menu->addChild(button);
        menu->updateLayout();

        return true;
    }

    void onAutoBuildLayout(CCObject*) {
        if (auto editor = LevelEditorLayer::get()) {
            if (auto popup = AutoBuildLayoutPopup::create(editor)) {
                popup->show();
            }
        }
    }
};
