#include "BuildGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace geode::prelude;

namespace {
    // Approximate Geometry Dash normal-speed travel in editor units per second.
    constexpr double UNITS_PER_SECOND = 311.0;
    constexpr int MAX_BEATS = 2000;

    constexpr double DENSITY_BY_DIFFICULTY[] = {
        .20, // Easy
        .27, // Normal
        .34, // Hard
        .41, // Harder
        .48, // Insane
        .53, // Easy Demon
        .58, // Medium Demon
        .63, // Hard Demon
        .68, // Insane Demon
        .72, // Extreme Demon
    };

    constexpr double DOUBLE_SPIKE_CHANCE[] = {
        .00, .00, .02, .05, .08,
        .12, .17, .22, .27, .32,
    };

    std::uint32_t rollForBeat(std::uint32_t beat, std::uint32_t salt = 0) {
        // Deterministic hash: the same inputs produce the same layout.
        std::uint32_t value =
            beat * 747796405u + salt * 2891336453u + 277803737u;
        value = ((value >> ((value >> 28u) + 4u)) ^ value) * 277803737u;
        return (value >> 22u) ^ value;
    }

    bool addSpike(LevelEditorLayer* editor, double x) {
        // Object ID 8 is the standard upright spike. In the default editor
        // coordinate system, y=105 places it on the default ground.
        auto objectString = fmt::format(
            "1,8,2,{:.2f},3,105",
            x
        );

        return editor->addObjectFromString(objectString) != nullptr;
    }
}

bool BuildGenerator::generate(
    LevelEditorLayer* editor,
    int difficulty,
    double bpm,
    double lengthSeconds,
    std::size_t& objectsPlaced
) {
    objectsPlaced = 0;

    constexpr auto difficultyCount =
        sizeof(DENSITY_BY_DIFFICULTY) / sizeof(DENSITY_BY_DIFFICULTY[0]);

    if (!editor
        || difficulty < 0
        || difficulty >= static_cast<int>(difficultyCount)
        || !std::isfinite(bpm)
        || !std::isfinite(lengthSeconds)
        || bpm <= 0.0
        || lengthSeconds <= 0.0) {
        return false;
    }

    auto totalBeats = static_cast<int>(
        std::floor(bpm * lengthSeconds / 60.0)
    );
    totalBeats = std::clamp(totalBeats, 0, MAX_BEATS);

    if (totalBeats < 5) {
        return true;
    }

    auto const beatSpacing = UNITS_PER_SECOND * 60.0 / bpm;
    auto const density = DENSITY_BY_DIFFICULTY[difficulty];
    auto const doubleChance = DOUBLE_SPIKE_CHANCE[difficulty];

    // Leave room at the beginning and end of the generated section.
    double lastHazardX = -10000.0;

    for (int beat = 2; beat < totalBeats - 2; ++beat) {
        auto const x = static_cast<double>(beat) * beatSpacing;
        auto const selection =
            rollForBeat(static_cast<std::uint32_t>(beat), 11u) % 100u;

        if (static_cast<double>(selection) >= density * 100.0) {
            continue;
        }

        // Avoid placing independent hazards too close together at high BPM.
        if (x - lastHazardX < 65.0) {
            continue;
        }

        if (!addSpike(editor, x)) {
            return false;
        }
        ++objectsPlaced;
        lastHazardX = x;

        auto const doubleRoll =
            rollForBeat(static_cast<std::uint32_t>(beat), 97u) % 100u;

        if (static_cast<double>(doubleRoll) < doubleChance * 100.0) {
            constexpr double DOUBLE_SPIKE_OFFSET = 30.0;

            if (!addSpike(editor, x + DOUBLE_SPIKE_OFFSET)) {
                return false;
            }

            ++objectsPlaced;
            lastHazardX = x + DOUBLE_SPIKE_OFFSET;
        }
    }

    return true;
}
