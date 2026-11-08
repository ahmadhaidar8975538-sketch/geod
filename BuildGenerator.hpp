#pragma once

#include <Geode/Geode.hpp>

#include <cstddef>

class LevelEditorLayer;

class BuildGenerator {
public:
    // Returns false if the editor fails to add a generated object.
    // Objects already added before a failure are not rolled back.
    static bool generate(
        LevelEditorLayer* editor,
        int difficulty,
        double bpm,
        double lengthSeconds,
        std::size_t& objectsPlaced
    );
};
