# Project rules

CS4515 final project: a cel-shaded tech demo in C++ / OpenGL. Two authors: Bogdan (rendering and shading) and Mihai (animation, cameras, systems). Plan with shared progress: https://claude.ai/artifact/WZTqst4zNGbMDz7kiPk2Yd

## Course rules

- **Credit is per author.** Each feature has one owner. Don't write or rewrite code in the other person's feature files.
- **Every feature gets a UI toggle** in `Application::drawUI()` and must be clearly visible.
- **OpenGL 4.1 core (macOS).** No compute shaders, SSBOs, image load/store, DSA (`glCreate*`) or `glDebugMessageCallback`.

## How we work

- **Keep it simple.** Pick the simplest approach that works and use just enough code, but keep it decent quality. Add complexity only after testing shows it's needed. We are beginners in computer graphics and want to understand every line.
- **Comments:** only short labels for logical steps, like `// load mesh into memory`. No explanations, file headers or paragraphs in code. Explain things in chat instead.
- do not make branches. the developers are gonna make them as they wish
- **Ask before** pushing to `main`, deleting branches or deleting the other person's code.
- Keep in mind that the project is done by Computer Graphics beginners, so we don't expect perfect code. It is more important to have readable code that demonstrates understanding of the concepts, even if it is not the most efficient or elegant solution.

## Code layout

```
src/application.cpp   window, scene, render loop, UI. Shared: keep it thin
src/core/             mesh, texture, camera helpers. Not graded
src/features/         graded code, one .h/.cpp per feature, written by its owner
shaders/              all GLSL, flat; prefix feature shaders (bezier_vert.glsl)
resources/            .obj meshes and textures
```

- Meshes are `.obj` files in `resources/`, loaded with `GPUMesh::loadMeshGPU`. No generated shapes in code.
- New `.cpp` files go in `add_executable` in `CMakeLists.txt`. After adding a `.glsl` file, reconfigure CMake.
