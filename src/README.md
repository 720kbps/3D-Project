# Source layout

```
src/
  application.cpp   Main loop, scene list, UI panel. Shared: edit carefully, keep it thin.
  core/             SCAFFOLDING. Mesh, texture, camera. Not graded features.
  features/         YOUR CODE. One .h/.cpp pair per feature, written by its owner.
shaders/            All GLSL. Flat folder (CMake copies shaders/*.glsl), so prefix feature
                    shaders with the feature name: bezier_vert.glsl, outline_frag.glsl, ...
docs/               Plan, contributions, references, screenshots for the report.
```

## Rules

**core/** holds helpers no one gets points for. Each file starts with a comment saying where it came
from (course starter, exercise 4, Mihai). Don't put feature logic here. Touch it only when a
feature truly needs it (e.g. a new vertex attribute), and say so in the commit message.

**features/** is where graded work goes. Each feature:

1. Has its own `features/<name>.h` and `.cpp` (e.g. `bezier.h` / `bezier.cpp`). A shader-only feature like PBR may just be `.glsl` files.
2. Starts with a comment: what it does, who wrote it, references used.
3. Is added to `add_executable` in `CMakeLists.txt` under "Features".
4. Gets an on/off toggle in `Application::drawUI()` (the brief requires it).
5. Is called from `application.cpp`, not the other way round. Features don't include `application.cpp`.

One author per feature file keeps the contribution table on page 1 of the report honest and easy.

**Shaders**: after adding a new `.glsl` file, reload CMake (File > Reload CMake Project).

## Things marked PLACEHOLDER

- `shaders/shader_frag.glsl`: fixed-direction diffuse light and gamma 2.2. Replaced by PBR (task 2.2)
  and the HDR/tonemap pass.
- `core/camera.*`: exercise 4 free camera, moves a fixed step per frame instead of per second.
  Replaced or extended by the camera feature (task 2.1).
