#pragma once

#include <memory>
#include <string>

class GameObject;

// Imports a 3D model (FBX, OBJ, glTF) with Assimp and builds a GameObject tree:
// one GameObject per node and one per mesh, each one with Transform + Mesh + Material.
// No Assimp type escapes this file.
namespace ModelImporter
{
    struct Settings
    {
        // FBX files store centimeters (UnitScaleFactor); the engine works in meters
        bool convertUnitsToMeters = true;
        // Blender exports add a -90 degrees X rotation to the root objects that the geometry
        // does not need (the Baker House would lie on its back). Undo it for Blender files.
        bool fixBlenderAxis = true;
    };

    Settings& GetSettings();
    std::unique_ptr<GameObject> Import(const std::string& path);
}
