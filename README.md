# Neugri Engine

Motor 3D hecho desde cero en C++20 + OpenGL para la asignatura de **Motores**.
Esta versión (**v0.1**) corresponde a la **Entrega 1: Visualizador de geometría**: carga de modelos FBX con drag & drop, texturas, jerarquía de GameObjects con componentes, cámara con controles de Unity y un editor hecho con Dear ImGui (docking).

- **Repositorio:** https://github.com/Santi29996/Repositorio-Personal
- **Release:** https://github.com/Santi29996/Repositorio-Personal/releases

## Autor

| Integrante | GitHub |
|---|---|
| Santiago Álvarez Cervera | [Santi29996](https://github.com/Santi29996) |

---

## Cómo usar el motor

Ejecuta `NeugriEngine/NeugriEngine.exe`. Al arrancar se carga automáticamente **Baker House** y la cámara la enfoca.

### Cargar modelos y texturas

| Acción | Cómo |
|---|---|
| Importar un modelo | Arrastra un `.fbx` (también `.obj`, `.gltf`, `.glb`) desde cualquier carpeta del disco a la ventana. Se crea un GameObject por nodo y uno por malla, se coloca al lado de lo que ya hay en la escena y la cámara lo enfoca. |
| Aplicar una textura | Selecciona un GameObject y arrastra un `.png` o `.dds` (también `.tga`, `.bmp`). La textura se aplica al seleccionado y a todos sus hijos con malla. |
| Modelos de ejemplo | `File > Load from Assets` muestra todos los modelos y texturas de la carpeta `Assets` (Baker House, **Lantern** y **BoomBox**, y las texturas `Textures/tiles.png` y `Textures/tiles.dds`). |
| Abrir con el motor | Arrastrar un fichero sobre el `.exe` o pasarlo por línea de comandos lo carga igual que un drag & drop. |
| Formas básicas | `GameObject > Cube / Sphere / Cylinder / Pyramid / Plane / Torus` (generadas por código con normales y UVs). |

### Cámara (controles de Unity)

Los controles funcionan con el ratón sobre la ventana **Scene**.

| Control | Acción |
|---|---|
| Botón derecho (mantener) + mover ratón | Mirar alrededor (free look, el cursor se oculta) |
| Botón derecho + `W` `A` `S` `D` | Moverse como en un FPS |
| Botón derecho + `Q` / `E` | Bajar / subir |
| `Shift` | Duplica la velocidad de movimiento (también del zoom y el paneo) |
| Rueda del ratón | Zoom hacia el punto de referencia |
| `Alt` + botón izquierdo | Orbitar alrededor del objeto (punto de referencia) |
| Botón central | Panear (extra) |
| `F` | Enfocar el GameObject seleccionado (encuadra su bounding box completa) |
| Doble clic en la jerarquía | Enfocar ese GameObject (extra) |

### Ventanas del editor

Todas las ventanas se pueden acoplar (docking), mover y mostrar/esconder desde **View**. `View > Reset layout` restaura la distribución por defecto.

- **Scene:** la escena se renderiza en un framebuffer que se muestra dentro de esta ventana. Tiene una ayuda superpuesta con los controles (checkbox *Help*).
- **Console:** LOG de todo el flujo: inicialización de SDL, OpenGL/glad, DevIL, Assimp e ImGui, carga de geometría (mensajes de Assimp en azul), carga de texturas con DevIL, avisos y errores (amarillo/rojo). Filtro por texto, por nivel, *Clear*, *Copy* y auto-scroll.
- **Configuration:**
  - *Application:* gráficos de FPS y milisegundos y límite de FPS.
  - *Window:* ancho, alto, fullscreen, resizable, borderless y VSync.
  - *Renderer:* depth test, cull face, iluminación, texturas, wireframe, grid, AABB de la selección o de todo, checkers en todas las mallas, longitud de normales, color de fondo, dirección de la luz y estadísticas (draw calls y triángulos).
  - *Camera:* FOV, near/far, velocidades y sensibilidad.
  - *Input:* estado del ratón y modificadores.
  - *Textures & Import:* filtrado lineal/nearest, mipmaps, wrap y opciones de importación de Assimp.
  - *Memory:* gráfico de RAM del proceso, VRAM (si el driver la informa), memoria de mallas y texturas y número de GameObjects.
  - *Hardware & Software:* versiones de SDL3, OpenGL, GLSL, DevIL, Assimp, ImGui y glm; CPUs, RAM, instrucciones SIMD y GPU.
- **Hierarchy:** árbol de GameObjects. Clic para seleccionar; arrastrar un nodo sobre otro para cambiarle el padre (mantiene la posición en el mundo) o sobre el espacio vacío para devolverlo a la raíz. Clic derecho: *Create Empty Child, Move Up/Down, Focus, Delete*. `Supr` borra el seleccionado. `[M]` indica que tiene malla.
- **Inspector:** nombre editable, activar/desactivar el GameObject y cada componente, y:
  - *Transform:* posición, rotación (grados) y escala **editables** (bonus) y *Reset*.
  - *Mesh:* nombre, fichero de origen, vértices, índices, triángulos, normales/UVs, memoria y tamaño de la AABB. Checkboxes para ver las **normales por vértice** y las **normales por cara** (con slider de longitud).
  - *Texture:* path, tamaño en píxeles, memoria y una vista previa. Checkbox **Show checkers texture** para ver la textura de cuadros blancos y negros, color de tinte y *Remove texture*.
  - *Add Component:* añadir Mesh o Texture a un GameObject vacío.
- **About:** nombre del motor + versión, equipo, librerías con su versión y licencia MIT. Se abre y cierra como el resto (`Help > About` o `View > About`).

### Menú principal

- **File:** *New Scene*, *Load from Assets*, *Exit*.
- **View:** mostrar/esconder cada ventana, la demo de ImGui y *Reset layout*.
- **GameObject:** crear un GameObject vacío o una forma básica, enfocar y borrar el seleccionado.
- **Help:** *GitHub Documentation* (`/docs`), *Report a bug* (`/issues`), *Download latest* (`/releases`) y *About*.

---

## Funcionalidades extra

- Transform **editable** desde el inspector, con evaluación perezosa de la matriz global (*dirty flags*).
- **Reparenting** con drag & drop en la jerarquía, conservando la transformación en el mundo; reordenar hijos; crear y borrar GameObjects.
- **Docking** con un layout por defecto generado por código (`DockBuilder`) y opción *Reset layout*.
- **Render a framebuffer** mostrado en la ventana Scene (la escena se adapta al tamaño de la ventana).
- **Pipeline de shaders** (OpenGL 4.6 core con fallback a 4.5/4.3/3.3): VAO + VBO intercalado + EBO, uniforms model/view/projection y luz difusa simple.
- Grid de referencia con ejes de colores, **AABB** de la selección y opción para ver todas las AABB.
- Botón central para **panear** y doble clic en la jerarquía para enfocar.
- Los modelos nuevos se colocan **al lado** de los existentes, apoyados en el suelo, y la cámara los enfoca.
- **Corrección automática** de FBX exportados desde Blender: se quita la rotación de -90° en X que tumbaba la Baker House y se convierten las unidades a metros con el `UnitScaleFactor` del FBX. Ambas opciones se pueden desactivar en *Configuration > Textures & Import*.
- Texturas **embebidas** en el FBX (comprimidas o RAW) y búsqueda del fichero de textura en varias rutas (carpeta del modelo, `Textures/`, `Assets/`).
- Caché de texturas y mallas (un fichero solo se carga una vez); estadísticas de memoria.
- 6 primitivas generadas por código (cubo, esfera, cilindro, pirámide, plano y toro).
- Consola con filtro, niveles, colores y copiar al portapapeles.
- `F12` guarda una captura de pantalla (`screenshot.bmp`).
- Callback de depuración de OpenGL (`GL_DEBUG_OUTPUT`) en las builds Debug.
- El motor fija el directorio de trabajo en la carpeta del `.exe`, así que encuentra `Assets/` aunque se lance desde otro sitio.

## Comentarios para el profesor

- **Separación motor / editor:** el código está en `src/Engine` (librería estática `Engine`, sin ninguna dependencia de ImGui) y `src/Editor` (librería `Editor`). El editor es un módulo más que se añade desde `main.cpp` (`app.AddModule(std::make_unique<Editor::ModuleEditor>())`); el motor no incluye ninguna cabecera del editor. Los componentes no se dibujan a sí mismos: el `WindowInspector` del editor los pinta según su tipo.
- **Módulos** (en orden de ejecución): `Window` (SDL3 + contexto OpenGL + glad), `Input`, `Resources` (DevIL, log de Assimp, caché de texturas/mallas, textura checkers), `Scene` (árbol de GameObjects, selección, ficheros arrastrados), `Camera3D`, `Renderer3D` y, si existe, `Editor`. El orden de pintado es: geometría → debug (grid, AABB, normales) → editor → `SDL_GL_SwapWindow` (en `Application`).
- **GameObjects y componentes:** `GameObject` tiene hijos (`std::unique_ptr`), puntero al padre y una lista de componentes (`ComponentTransform` siempre, `ComponentMesh` y `ComponentMaterial`). Global = global del padre × local.
- **Importadores:** `ModelImporter` (Assimp) y la carga de texturas en `ModuleResources` (DevIL). Ningún tipo de Assimp sale de `ModelImporter.cpp` ni de DevIL de `ModuleResources.cpp`.
- **Baker House:** el FBX viene de Blender 2.78 con una rotación de -90° en X y escala 100 en los nodos, pero la geometría ya está en Y-up. Si se aplica la rotación tal cual, la casa sale tumbada. Lo explicamos en la consola al cargarla y se puede desactivar.
- **Modelos propios:** *Lantern* y *BoomBox* (Khronos glTF Sample Assets, licencia **CC0**), convertidos a FBX con un canal de textura difusa.
- **Dependencias:** se descargan con CMake `FetchContent` desde sus repositorios oficiales (SDL 3.2.24, Assimp 5.4.3, Dear ImGui 1.92.2b docking, DevIL 1.8.0 + libpng 1.6.50 + zlib 1.3.1, glm 1.0.1). glad (OpenGL 4.6 core) está en `third_party/`.

## Compilar desde el código

Requisitos: CMake ≥ 3.24, git y Visual Studio 2022/2026 (o cualquier compilador C++20).

```bash
# Visual Studio
cmake --preset vs2022
cmake --build --preset vs2022-release

# Ninja (Windows/Linux)
cmake --preset ninja-release
cmake --build --preset ninja-release

# Carpeta de release con la estructura de la entrega
cmake --install build/vs2022 --config Release --component Engine --prefix dist/NeugriEngine_v0.1
```

El ejecutable, las DLL y `Assets/` quedan en `build/<preset>/bin/`.

## Licencia

[MIT](LICENSE.md). Las licencias de las librerías y de los modelos de terceros están en la carpeta [`Licences/`](Licences).
