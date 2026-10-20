# References

Feeds the report's reference list. Add a row the moment you use a source, asset or technique from somewhere.
Following a tutorial's maths is fine if you write the code yourself and cite it; pasted code earns 0 points even when cited.

## Assets

| File | Source | Licence | Used for |
|---|---|---|---|
| `resources/dragon.obj` | Course material (practical 3). Appears to be the Stanford Dragon (Stanford 3D Scanning Repository); **confirm before citing** | Course-provided | Test scene |
| `resources/checkerboard.png` | Course project template | Course-provided | Test texture |
| `resources/debug/*` | Generated test images (orientation pattern, direction-coloured cube map) | n/a | Debug views only; not shown in the final demo |

## Techniques used so far

| Where in the code | Source |
|---|---|
| ACES tonemapping curve (`shaders/tonemap_frag.glsl`) | K. Narkowicz, "ACES Filmic Tone Mapping Curve", blog post, 2016 |
| sRGB encoding (`shaders/tonemap_frag.glsl`) | IEC 61966-2-1:1999, Multimedia systems and equipment: Default RGB colour space (sRGB) |
| Cube map face orientation (`src/texture.cpp`) | The OpenGL 4.1 Core Profile Specification, section "Cube Map Texture Selection" |
| Tangent frames for normal mapping (`src/tangents.cpp`) | E. Lengyel, "Computing Tangent Space Basis Vectors for an Arbitrary Mesh", Terathon Software, 2001 |

## Libraries (provided with the course framework)

GLM, GLFW, Dear ImGui, ImGuizmo, stb_image / stb_image_write, tinyobjloader, {fmt}, glad, toml++, nativefiledialog, Catch2.

## Planned (cite when the feature is implemented)

| Feature | Source |
|---|---|
| PBR (GGX, Smith, Schlick), IBL split sum | B. Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013 course notes |
| Toon ramp shading | A. Lake, C. Marshall, M. Harris, M. Blackstein, "Stylized Rendering Techniques for Scalable Real-Time 3D Animation", NPAR 2000 |
| Stylised shading (warm/cool, rim) | J. Mitchell, M. Francke, D. Eng, "Illustrative Rendering in Team Fortress 2", NPAR 2007 |
| Shadow mapping | L. Williams, "Casting Curved Shadows on Curved Surfaces", SIGGRAPH 1978 |
| Screen-space outlines | T. Saito, T. Takahashi, "Comprehensible Rendering of 3-D Shapes", SIGGRAPH 1990; P. Decaudin, "Cartoon-Looking Rendering of 3D-Scenes", INRIA Research Report 2919, 1996 |
| Bloom | J. Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare", SIGGRAPH 2014 course; M. Kawase, "Frame Buffer Postprocessing Effects in DOUBLE-S.T.E.A.L (Wreckless)", GDC 2003 |
| SSAO | M. Mittring, "Finding Next Gen: CryEngine 2", SIGGRAPH 2007 course |
| Hatching | E. Praun, H. Hoppe, M. Webb, A. Finkelstein, "Real-Time Hatching", SIGGRAPH 2001 |
