# Tech demo plan

**Status: draft, to be agreed by both members.** Edit freely; everything marked **TBD** needs a decision.
Battle-plan tasks: 0.1 (scene and feature list) and the working agreements of 0.2.

## Scenario and art direction

A small floating **workshop diorama** that the viewer explores freely. There is no player character: the scene is mostly static, with a few animated pieces that make each feature easy to see.

Art direction in one sentence (**TBD: confirm or rewrite**): *physically based materials underneath, Overwatch-style cel shading and Borderlands-style ink outlines on top, with a switch to compare both.*

## Layout (top view, about 8 x 8 m)

```
                         -Z (back)
   +-------------------------------------------------+
   |  [2] brick wall (normal mapping)                 |
   |        * light orbiting in front of it           |
   |                                                  |
   |  [3] chrome orb   rough orb        [4] robot arm |
   |      (environment mapping)         on a base     |
   |                                    (hierarchy)   |
   |  [1] PBR sphere grid                             |
   |      o o o o   metallic ->                       |
   |      o o o o   roughness v                       |
   |                                                  |
   |   ~ ~ ~ [5] drone loop around everything ~ ~ ~   |
   +-------------------------------------------------+
     -X                      +Z (front)             +X
   Skybox all around (environment map + lighting).
```

| Station | Shows | Moves? |
|---|---|---|
| 1 PBR sphere grid | Advanced shading (PBR), material textures | No |
| 2 Brick wall | Normal mapping | A small light orbits in front, so the bumps catch light |
| 3 Two orbs | Environment mapping (chrome vs rough) | No |
| 4 Robot arm | Hierarchical transformations | Yes: looping idle animation (reach, grab, swing, release) |
| 5 Drone | Bézier path (3+ cubic segments), chase camera | Yes: flies the loop; thruster particles |

Every station gets a saved camera preset, so the video can cut straight to it.

## Feature list

### Minimum features (all required)

| Feature (handout wording) | Battle plan | Track | Owner |
|---|---|---|---|
| Multiple viewpoints (free, orbit, top, chase camera) | 2.1 | B | Mihai |
| Advanced shading (PBR) | 2.2 | A | Member A (TBD) |
| Material textures | 2.3 | A | Member A (TBD) |
| Normal mapping | 2.4 | A | Member A (TBD) |
| Environment mapping | 2.5 (+ 2.6 IBL) | A | Member A (TBD) |
| Smooth paths (Bézier, 3+ cubic segments, toggleable curve) | 2.7 | B | Mihai |
| Hierarchical transformations (animated robot arm) | 2.8 | B | Mihai |

### Extras (target 5 to 7, fixed now to limit scope)

Proposal, in order of value for this style versus effort (**TBD: keep, drop or swap**):

| # | Extra | Battle plan | Track | Owner |
|---|---|---|---|---|
| 1 | Cel shading: toon ramp, stylised specular and rim light | 3.1, 3.2 | A | Member A (TBD) |
| 2 | Outlines: screen-space edge detection (post-processing) compared with inverted hull | 3.4, 3.3 | A / B | Member A (edge detection), Mihai (inverted hull) |
| 3 | Shadows, then shadows for multiple light sources | 3.5, 4.1 | B | Mihai |
| 4 | Bloom (post-processing glow) | 4.2 | A | Member A (TBD) |
| 5 | Constant speed along the Bézier path | 4.3 | B | Mihai |
| 6 | Particle system with animated (flipbook) textures | 4.4, 4.5 | B | Mihai |
| Stretch | Day-night cycle, minimap, SSAO, hatching | 4.6, 4.7, 4.8, 3.6 | | |

Tracks: **A (rendering)** = Member A (TBD), **B (systems)** = Mihai.

Rule: each feature has exactly **one** owner (per-author grading). Swap freely, but the report's contribution table must match who wrote the code.

## Controls (proposal, **TBD**)

| Input | Action |
|---|---|
| Right mouse drag + W/A/S/D, Q/E, Shift | Free camera (look, move, up/down, faster) |
| Tab | Cycle camera mode (free, orbit, top, chase) |
| 1 to 5 | Jump to the station presets |
| T | Start or stop the cinematic tour |
| P | Pause simulation time |
| F12 | Screenshot to `docs/screens/` (already works) |

## Working agreements (0.2)

- **Branches:** one branch per feature (`feature/<name>`). Merge into `main` only when it builds without warnings, runs without `[GL]` errors in the console, and has its UI toggle.
- **Commits:** a commit contains one person's work only, so the git history backs up the contribution table.
- **Build:** new `.cpp` files must be added to `add_executable` in `CMakeLists.txt`; after adding a `.glsl` file, re-run CMake configure.
- **Never commit:** build folders, `.idea/`, `.vscode/`, `imgui.ini` (all ignored already).
- **Both machines:** after every merge to `main`, the other member pulls and builds. Different GPUs and drivers catch different GLSL errors.
- **Hand-in:** create the zip with `git archive` so no build files get in (-0.5 penalty otherwise).
