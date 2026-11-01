# Neugri Engine: documentación

## Guía rápida

1. Abre `NeugriEngine.exe`. Baker House se carga sola.
2. Arrastra cualquier `.fbx` a la ventana para importarlo, o usa `File > Load from Assets`.
3. Selecciona un objeto en **Hierarchy** y arrastra un `.png`/`.dds` para cambiarle la textura.
4. Navega con el botón derecho + WASD, la rueda, Alt + clic izquierdo y `F`, como en Unity.

La lista completa de controles y opciones está en el [README](../README.md).

## Arquitectura

```
src/
├── main.cpp                  Crea Application, añade el editor y ejecuta el bucle principal
├── Engine/                   Librería del motor (sin ImGui)
│   ├── Core/                 Application, Module, Log, EngineInfo
│   ├── Modules/              Window, Input, Resources, Scene, Camera3D, Renderer3D
│   ├── Scene/                GameObject, Component, Transform, Mesh, Material
│   ├── Resources/            Mesh (VAO/VBO/EBO), Texture, Shader, Framebuffer, AABB
│   ├── Importers/            ModelImporter (Assimp), Primitives
│   └── Platform/             SystemInfo (hardware, versiones, memoria)
└── Editor/                   Librería del editor (Dear ImGui)
    ├── ModuleEditor          ImGui, dockspace, menú principal y atajos
    └── Windows/              Scene, Hierarchy, Inspector, Console, Configuration, About
```

### Bucle principal

`PreUpdate` (input, nuevo frame) → `Update` (ficheros arrastrados, cámara, GameObjects) → `PostUpdate` (renderer → editor) → `SwapWindow`.

### Flujo de importación

`fichero arrastrado` → `ModuleInput` (SDL_EVENT_DROP_FILE) → `ModuleScene::HandleDroppedFile`
- modelo → `ModelImporter::Import` → `aiImportFileExWithProperties` → un GameObject por `aiNode` y uno por `aiMesh` → `Mesh` en VRAM y textura difusa con DevIL
- textura → `ModuleResources::LoadTexture` (DevIL → RGBA → `glTexImage2D` + mipmaps) → se aplica al `ComponentMaterial` del seleccionado y de sus hijos
