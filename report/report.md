# Cel-Shaded Tech Demo

**CS4515 3D Computer Graphics and Animation — Final Project Report**

Group 40 · Bogdan Mocanu · Mihai Santai

*Draft, work in progress. Last updated 3 November 2026.*

---

## 1. Work per group member

### Minimal features

| Feature | What was built | Bogdan Mocanu | Mihai Santai | Status |
|---|---|---|---|---|
| Multiple viewpoints | Top (orthographic), orbit and chase cameras, switching between four views | 50% | 50% | Done |
| Advanced shading | Blinn-Phong and PBR (Cook-Torrance with GGX) | 100% | 0% | Done |
| Material textures | Albedo, roughness, metallic and ambient occlusion maps¹ | 50% | 50% | Done |
| Normal mapping | Tangent-space normal map with a TBN matrix² | 100% | 0% | Done |
| Environment mapping | Skybox and roughness-dependent reflections³ | 50% | 50% | Done |
| Smooth paths | Closed C1 loop of six cubic Bézier curves, a cart riding it, path toggle | 100% | 0% | Done |
| Hierarchical transformations | Robot arm | 0% | 100% | Done |

### Extra features

| Feature | What was built | Bogdan Mocanu | Mihai Santai | Status |
|---|---|---|---|---|
| Cel shading | Banded diffuse light, shadow tint, hard highlight | 0% | 100% | Done |
| Rim light | Lit-side silhouette highlight | 50% | 50% | Done |
| Outlines | Sobel edge detection on depth and normals | 100% | 0% | Done |
| HDR and tonemapping | Float framebuffer, exposure, Reinhard tonemapping | 100% | 0% | Done |
| Bloom (post-processing glow) | Bright pass and separable Gaussian blur | 100% | 0% | Done |
| Shadow mapping | Sun shadow map with PCF | 0% | 100% | In the demo |
| Shadows for multiple light sources | Shadow maps for both point lights | 0% | 100% | In the demo |
| Constant speed along a Bézier curve | Arc-length table | 0% | 100% | In the demo |
| Inverted-hull outlines | | – | planned | Planned |
| Particle effects | | – | planned | Planned |
| Animated textures | | – | planned | Planned |
| Day-night cycle | | – | planned | Planned |
| Minimap | | – | planned | Planned |


Not graded: the window, render loop and UI panel (shared), frame timing, pause and vsync (Bogdan Mocanu), and the free camera, which is the course framework's camera from Practical 4.

---

## 2. The demo

The demo is a small cel-shaded scene on a ground plane under a sky. It contains:

- a gold metallic dragon in the middle [15];
- four spheres whose roughness goes from 0.15 to 0.90;
- a cube with a painted-metal texture set;
- a purple cylinder and a robot arm;
- a glowing cart that rides a closed Bézier track around the scene.

A sun and two coloured point lights (off at the start) light the scene, and the sky is a cube map [16]. Everything runs in one application, and every feature can be switched on and off in the **Settings** window.

| Input | Action |
|---|---|
| 1, 2, 3, 4 | Free, top, orbit and chase camera |
| W A S D, R / F, hold left mouse | Move and look around (free camera) |
| Drag with left mouse, scroll | Rotate and zoom (orbit camera) |

### Render pipeline

Every frame goes through these passes:

```
1. Shadow maps    depth seen from the sun and from the two point lights
2. Scene pass     into the HDR framebuffer: RGBA16F colour, RGBA16F normal, depth texture
                  objects (main shader), point-light markers, skybox, Bézier path
3. Bloom          bright pass at half resolution, then a ping-pong Gaussian blur
4. Final pass     to the screen: + bloom, × exposure, outlines, Reinhard, gamma 2.2
5. Overlays       debug geometry drawn on top of the final image
```

The scene pass writes the surface normal to a second colour attachment next to the colour. Together with the depth texture, this lets the final pass find outlines without drawing the scene a second time.

---

## 3. Minimal features

### 3.1 Multiple viewpoints

*Bogdan Mocanu and Mihai Santai · UI: Camera → Mode, or keys 1 to 4*

Each frame has one view matrix and one projection matrix. A camera only decides those two matrices, so no other part of the pipeline needs to know which camera is active. All view matrices are built with `glm::lookAt`, as in Practical 4 [1].

- **Free camera (1).** This is the framework camera from Practical 4. W A S D and R / F move it, and the mouse turns it. Its step is scaled by the frame time (2.4 units per second), so it moves at the same speed at any frame rate.
- **Top camera (2).** It looks straight down from a height of 10, with −z as the up direction on screen. It uses an orthographic projection (`glm::ortho`). The "Top view size" slider sets the half-height of the view box, and the width follows the window's aspect ratio. An orthographic view does not shrink objects with distance, so the scene reads like a map and the Bézier track shows its true shape.
- **Orbit camera (3).** It sits on a sphere around the dragon. For yaw $\psi$, pitch $\varphi$ and distance $r$, its position is
  $$\mathbf{p} = \mathbf{t} + r\,(\cos\varphi\sin\psi,\ \sin\varphi,\ \cos\varphi\cos\psi)$$
  where $\mathbf{t}$ is the middle of the dragon. Dragging changes $\psi$ and $\varphi$, and scrolling changes $r$ between 0.5 and 10. The pitch is clamped to ±1.5 rad, just under ±π/2. At the poles the view direction would be parallel to the up vector passed to `lookAt`, and the view would flip.
- **Chase camera (4).** It follows the cart along the Bézier path. It sits 0.6 units behind the cart, on the side away from the scene centre, and 0.25 units above it. It looks at the centre of the scene rather than along the track, so the dragon and the other objects stay in view during the whole lap.

The outline pass (Section 4.3) linearises depth differently for the top camera, because an orthographic depth buffer is already linear.

| Free camera | Top camera (orthographic) |
|---|---|
| ![Free camera](images/2.1-multiple-viewpoints/1_free.png) | ![Top camera](images/2.1-multiple-viewpoints/2_top.png) |
| **Orbit camera** | **Chase camera** |
| ![Orbit camera](images/2.1-multiple-viewpoints/3_orbit.png) | ![Chase camera](images/2.1-multiple-viewpoints/4_chase.png) |

### 3.2 Advanced shading: Blinn-Phong and PBR

*Bogdan Mocanu · UI: Shading → Blinn-Phong / PBR / Cel*

Both models are evaluated per light in `shadeLight()` (`shaders/shader_frag.glsl`). There are three lights: the sun, which is a directional light, and two point lights. The radiance of a point light falls off with the square of the distance, $L_i = \Phi / d^2$. Both models use the same material inputs (albedo $\mathbf{c}$, roughness, metallic), so they can be compared on the same objects.

**PBR.** This is the Cook-Torrance microfacet model [3] with a Lambert diffuse term:

$$L_o = \left(k_D\,\frac{\mathbf{c}}{\pi} + \frac{D\,G\,F}{4\,(\mathbf{n}\cdot\mathbf{v})(\mathbf{n}\cdot\mathbf{l})}\right) L_i\,(\mathbf{n}\cdot\mathbf{l})$$

- **D** is the GGX normal distribution [4]. It gives the share of microfacets that face the half vector $\mathbf{h}$:
  $D = \dfrac{\alpha^2}{\pi\left((\mathbf{n}\cdot\mathbf{h})^2(\alpha^2-1)+1\right)^2}$, with $\alpha = \text{roughness}^2$. Squaring the roughness makes the slider feel more even [5].
- **G** is Smith's shadowing-masking term with Schlick-GGX [5]. It gives the share of microfacets that are not hidden by other microfacets:
  $G_1(\mathbf{x}) = \dfrac{\mathbf{n}\cdot\mathbf{x}}{(\mathbf{n}\cdot\mathbf{x})(1-k)+k}$, with $k = (\text{roughness}+1)^2/8$ and $G = G_1(\mathbf{v})\,G_1(\mathbf{l})$.
- **F** is Schlick's Fresnel approximation [6]: $F = F_0 + (1-F_0)(1-\mathbf{h}\cdot\mathbf{v})^5$, with $F_0 = \text{mix}(0.04,\ \mathbf{c},\ \text{metallic})$. A non-metal reflects about 4% of the light at normal incidence, and a metal reflects in its own colour.
- $k_D = (1-F)(1-\text{metallic})$. Light that is reflected as specular cannot also be diffused, and metals have no diffuse part.

The roughness is clamped to at least 0.05, so $D$ stays finite on very smooth surfaces.

**Blinn-Phong** [7]. This model is $L_o = \left(\frac{\mathbf{c}}{\pi} + 0.25\,(\mathbf{n}\cdot\mathbf{h})^s\right)L_i\,(\mathbf{n}\cdot\mathbf{l})$. The shininess comes from the same roughness: $s = 2/\alpha^2 - 2$ is the Phong exponent that matches a microfacet lobe of width $\alpha$ [4]. So one roughness value drives both models.

Both models add a constant ambient term, $\mathbf{c}\cdot a\cdot\text{AO}$.

**Why PBR.** Phong-style models need a shininess and a specular strength tuned by hand for each scene, and they can reflect more light than arrives. The microfacet model conserves energy (close enough), its parameters have a physical meaning, and it matches the roughness/metallic texture sets that are freely available (Section 3.3). We chose GGX over Beckmann because its long tail gives the soft glow around highlights that real materials have, and it is the usual real-time choice [5]. Blinn-Phong stays in the demo as a toggle for comparison.

| Blinn-Phong | PBR |
|---|---|
| ![Blinn-Phong](images/2.2-advanced-shading/blinn_phong.png) | ![PBR](images/2.2-advanced-shading/pbr.png) |

*Same scene and lights. The spheres from left to right have roughness 0.90, 0.65, 0.40 and 0.15.*

### 3.3 Material textures

*Bogdan Mocanu and Mihai Santai · UI: Albedo / Roughness / Metallic / AO maps (top of Settings); constant values per object under Materials*

The cube uses the PaintedMetal006 texture set from ambientCG [14] at 1K resolution: colour (albedo), roughness, metalness, ambient occlusion (AO) and a normal map (Section 3.4). Each map is optional per object. If an object has no map, or the map is switched off in the UI, the shader uses the object's constant value instead (`baseColor`, `roughness`, `metallic`, and AO = 1). This makes every map switchable on its own, and the Materials panel can change the constants of all the other objects while the demo runs.

**Colour spaces.** The albedo map and the skybox are loaded as sRGB textures (`GL_SRGB8_ALPHA8`), so the GPU turns them into linear values when it samples them. The roughness, metalness, AO and normal maps are loaded as linear `GL_RGBA8`, because they store numbers, not colours. Constant colours in the code are converted with `glm::convertSRGBToLinear`, and the final pass applies gamma 1/2.2. All lighting therefore happens in linear space, where adding up the light from several sources is physically correct.

**Ambient occlusion** multiplies only the ambient term and the environment reflection. AO describes how much indirect light reaches a crevice. Direct light is already handled by $\mathbf{n}\cdot\mathbf{l}$.

We chose roughness/metallic maps over separate $k_d$, $k_s$ and shininess maps because they are exactly the inputs of the PBR model in Section 3.2.

| All maps off | All maps on |
|---|---|
| ![Maps off](images/2.3-material-textures/maps_off_pbr.png) | ![Maps on](images/2.3-material-textures/maps_on_pbr.png) |

*PBR shading. Left: the cube's constants (white, roughness 0.5, not metallic). Right: all five maps.*

### 3.4 Normal mapping

*Bogdan Mocanu · UI: Normal maps, Normal strength*

Every vertex carries a tangent as well as a normal: the direction of increasing $u$ in the texture, plus a sign $w$ that says whether the UVs are mirrored². The vertex shader transforms the normal with the inverse transpose of the model matrix and the tangent with the model matrix. The fragment shader then:

1. Makes the tangent perpendicular to the normal again (Gram-Schmidt): $T = \text{normalize}\big(T - (T\cdot N)\,N\big)$. Interpolating across a triangle breaks this property.
2. Builds the bitangent, $B = (N \times T)\,w$.
3. Reads the map and moves it from $[0,1]$ to $[-1,1]$: $\mathbf{n}_t = 2\,\text{rgb} - 1$. It multiplies $n_{t,x}$ and $n_{t,y}$ by the "Normal strength" slider (0 to 3).
4. Moves the normal into world space: $N = \text{normalize}\big([T\ B\ N]\,\mathbf{n}_t\big)$.

Changing the normal per pixel instead of the geometry is the idea behind Blinn's bump mapping [8]; a normal map stores the changed normal directly.

**Why tangent space.** A tangent-space map does not depend on how the object is placed or rotated, and it works on any mesh with UVs. An object-space map belongs to one mesh only. We use the "NormalGL" version of the texture because its green channel follows OpenGL's convention, with $v$ pointing up. Scaling $x$ and $y$ before normalising is a cheap way to flatten or exaggerate the bumps. The outline pass (Section 4.3) gets the geometric normal, not the mapped one, so the texture's small bumps do not produce ink lines.

| Normal map off | Normal map on |
|---|---|
| ![Normal map off](images/2.4-normal-mapping/normal_map_off_low_sun.png) | ![Normal map on](images/2.4-normal-mapping/normal_map_on_low_sun.png) |

*Close-up of the cube with a low sun, so the light grazes the surface and the bumps cast visible shading.*

### 3.5 Environment mapping

*Bogdan Mocanu and Mihai Santai · UI: Environment → Skybox, Reflections*

**Skybox.** A cube map with six faces is drawn on a unit cube³. The view matrix has its translation removed (`mat3(view)`), so the sky stays infinitely far away when the camera moves. The vertex shader outputs `gl_Position = clip.xyww`, so after the perspective divide the depth is exactly 1, the far plane. The skybox is drawn after the opaque objects with `glDepthFunc(GL_LEQUAL)`. It then only fills pixels that nothing else covers, and no shading work is wasted on hidden sky.

**Reflections.** Looking up the surroundings in the reflected direction is the classic environment-mapping idea of Blinn and Newell [9]. Every surface samples the same cube map in the reflected direction, $R = \text{reflect}(-V, N)$, using the normal-mapped $N$. Rough surfaces should show a blurry reflection, so the mipmap level depends on the roughness:

$$\text{lod} = \text{roughness}\cdot\log_2(\text{size})$$

A perfectly smooth surface reads level 0, which is sharp, and roughness 1 reads the smallest level. The reflection is weighted with Schlick's Fresnel term, using $\mathbf{n}\cdot\mathbf{v}$ and the same $F_0$ as in Section 3.2, and multiplied by AO. Metals reflect strongly and in their own colour, while non-metals mostly reflect at grazing angles.

**Why this approach.** The exact method pre-filters the cube map with the GGX lobe for each roughness level (the "split sum" approximation [5]). That needs extra pre-computation passes. The ordinary mipmap chain is much simpler and shows the right trend, with rougher surfaces looking blurrier. Its limitation is that a box-filtered mipmap is not exactly the GGX blur.

| Reflections off | Reflections on |
|---|---|
| ![Reflections off](images/2.5-environment-mapping/reflections_off_pbr.png) | ![Reflections on](images/2.5-environment-mapping/reflections_on_pbr.png) |

*PBR shading. The dragon is metallic with roughness 0.3, so the sky colour shows up in its reflection.*

### 3.6 Smooth paths: a Bézier loop

*Bogdan Mocanu · UI: Bezier path → Show path and cart, Cart speed*

The track is a closed loop of six cubic Bézier segments (`src/features/bezier_path.cpp`). It is defined by six anchor points $\mathbf{A}_i$ that the track passes through, and one handle vector $\mathbf{h}_i$ per anchor. Segment $i$ goes from anchor $i$ to anchor $i+1$ and has the control points

$$\mathbf{P}_0 = \mathbf{A}_i,\quad \mathbf{P}_1 = \mathbf{A}_i + \mathbf{h}_i,\quad \mathbf{P}_2 = \mathbf{A}_{i+1} - \mathbf{h}_{i+1},\quad \mathbf{P}_3 = \mathbf{A}_{i+1}$$

$$\mathbf{B}(u) = (1-u)^3\mathbf{P}_0 + 3(1-u)^2u\,\mathbf{P}_1 + 3(1-u)u^2\,\mathbf{P}_2 + u^3\mathbf{P}_3, \qquad u\in[0,1]$$

Neighbouring segments share the handle at their common anchor. The end of segment $i$ and the start of segment $i+1$ therefore have the same derivative, $\mathbf{B}'_i(1) = 3\mathbf{h}_{i+1} = \mathbf{B}'_{i+1}(0)$. This makes the path C1 continuous: it has no kinks, and the cart's velocity does not jump between segments. The whole path uses one parameter $t \in [0, 6)$. The segment index is $\lfloor t\rfloor$ and $u$ is the fractional part. $t$ wraps around with `fmod`, so the loop closes.

**Drawing.** The curve is sampled once at start-up, 50 points per segment, and uploaded to a vertex buffer. It is drawn as a `GL_LINE_LOOP` with a plain unlit shader. The curve never changes, so there is no work to do on the CPU per frame. The cart is a small emissive sphere at $\mathbf{B}(t)$, with $t = \text{time} \times \text{speed}$ when the constant-speed option (Section 4.8) is switched off. The speed is given in segments per second.

**Why anchors and handles.** Describing each segment by four free control points does not keep the joins smooth by itself. With shared handles, C1 continuity holds by construction, and there are fewer numbers to tune.

| Path hidden | Path shown | Path from the top camera |
|---|---|---|
| ![Path off](images/2.7-bezier-path/path_off.png) | ![Path on](images/2.7-bezier-path/path_on.png) | ![Path top view](images/2.7-bezier-path/path_top_view.png) |

### 3.7 Hierarchical transformations

*Mihai Santai · UI: Robot arm*

A hierarchical model positions each part relative to its parent. The model matrix of a part is the parent's model matrix times the part's own local transformation, $M_{\text{child}} = M_{\text{parent}}\,T\,R\,S$. Moving or rotating a joint therefore moves every part below it in the hierarchy.

The robot arm is built in code from three of the project's unit meshes, `cube.obj`, `cylinder.obj` and `sphere.obj`, following the robot arm of Practical 1 [1]. It has ten parts in one chain, and five angles move them:

```
pedestal (cylinder, fixed)
└─ turret (box)                  turns about y
   └─ shoulder (sphere)          bends about x
      └─ upper arm (box)
         └─ elbow (sphere)       bends about x
            └─ forearm (box)
               └─ wrist (sphere) bends about x
                  └─ hand (box)
                     ├─ left finger   opens about z by the grip angle
                     └─ right finger  opens about z by minus the grip angle
```

**Joint frames.** Every joint has a frame, a 4×4 matrix that holds the joint's position and orientation in the world. A frame is built from its parent's frame by walking along the parent to the joint and then rotating by the joint's angle $\theta_i$:

$$F_i = F_{i-1}\;T(0,\ \ell_{i-1},\ 0)\;R(\theta_i)$$

Here $\ell_{i-1}$ is the length of the parent part. $R$ is a rotation about $y$ for the turret, about $x$ for the shoulder, elbow and wrist, and about $z$ for the fingers. Turning the shoulder changes $F_{\text{shoulder}}$ and therefore every frame below it, so the whole arm follows.

**Drawn parts.** A box is drawn from its frame with $M_i = F_i\;T(0,\ \ell_i/2,\ 0)\;S(\mathbf{s}_i)$. The scale $S$ is the last factor, so it is applied first, to the unit mesh only. The half-length translation moves the mesh so that it starts at the joint instead of being centred on it. The spheres at the shoulder, elbow and wrist are drawn at the frame's origin and cover the gaps when a joint is bent.

**Motion.** With "Animate" on, the angles are sine waves of the simulation time $t$, the same clock that "Time → Pause" stops. The shoulder, elbow and wrist share one wave with a phase offset that grows along the arm, so the motion travels from the shoulder to the wrist, like the wave of the snake in Practical 1:

$$\theta_k(t) = \theta_k^{\text{rest}} + a_k \sin(0.8\,t - 0.6\,k),\qquad (\theta^{\text{rest}}, a) = (0.40, 0.25),\ (0.90, 0.35),\ (0.50, 0.40)\ \text{rad for } k = 0, 1, 2$$

The turret swings by $0.6\sin(0.3\,t)$ rad, and the grip opens and closes between 0 and 0.5 rad at twice the speed, $0.25 + 0.25\sin(1.6\,t)$. With "Animate" off, five sliders (Turret, Shoulder, Elbow, Wrist, Grip) pose the arm by hand. "Show joint axes" draws the x (red), y (green) and z (blue) axes of seven joint frames on top of the final image.

**Code.** `src/features/robot_arm.h/.cpp` computes the ten part matrices in `RobotArm::update(time)`. Each frame, `application.cpp` copies them into ten ordinary scene objects, so shading, shadows and outlines apply to the arm like to any other object.

**Why this approach.** The scale never enters a joint frame. If it did, every child would be stretched by its parent's size, and the arm would no longer join up. Putting the pivot at the joint instead of at the middle of the mesh makes a rotation hinge the part at its end, as an elbow does. Using the shared time makes the animation independent of the frame rate. As a check, in three extreme poses every child starts at its parent's joint to within $10^{-7}$ units.

| Pose A | Shoulder moved | Joint axes |
|---|---|---|
| ![Pose A](images/2.8-hierarchical-transformations/pose_a.png) | ![Shoulder moved](images/2.8-hierarchical-transformations/pose_b_shoulder_moved.png) | ![Joint axes](images/2.8-hierarchical-transformations/joint_axes.png) |

*Left and middle: the same pose, with the shoulder angle changed. Everything above the shoulder moves with it. Right: the joint axes, which turn with their joints.*

---

## 4. Extra features

### 4.1 Cel shading

*Mihai Santai · UI: Shading → Cel, then Bands, Shadow tint, Highlight threshold, Highlight strength*

Cel shading replaces smooth shading with a few flat tones, like a hand-drawn cartoon. It starts from the PBR model in Section 3.2 and changes three things.

**Banded diffuse.** The diffuse factor $\mathbf{n}\cdot\mathbf{l}$ is rounded down to one of $b$ levels, where $b$ is the "Bands" slider (2 to 5):

$$x = (\mathbf{n}\cdot\mathbf{l})\,b,\qquad \text{banded} = \lfloor x\rfloor + \text{smoothstep}\big(1-w,\ 1,\ \text{fract}(x)\big),\qquad \text{light} = \min\!\left(\frac{\text{banded}}{b-1},\ 1\right)$$

Here $w = \text{fwidth}(x)$, which is how much $x$ changes from one pixel to the next. A plain `floor` gives jagged, stair-stepped band edges. The smoothstep spreads each edge over about one pixel at any distance, which anti-aliases it.

**Shadow tint.** The grey ambient term is replaced by a coloured tint, dark blue by default. Shadows then look cool rather than grey, as in illustration.

**Hard highlight.** The highlight is switched on where the GGX term $D\,(\mathbf{n}\cdot\mathbf{l})$ is above a threshold, again with a smoothstep of width `fwidth` for a clean edge. It is white on non-metals and the albedo colour on metals. Because it uses $D$, the highlight still depends on the material: smooth spheres get a crisp spot, and on rough spheres it disappears.

**Why this approach.** It follows the toon shading of Practical 3 [1] (discretising the diffuse light) and the hard-shading idea of Lake et al. [12]. Lake et al. read the bands from a 1D texture. Rounding in the shader needs no texture and lets the number of bands change from the UI. Reusing GGX for the highlight keeps the cel look tied to the same material parameters as PBR.

| Cel | PBR |
|---|---|
| ![Cel](images/3.1-cel-shading/cel.png) | ![PBR](images/3.1-cel-shading/pbr.png) |

*Three bands. On the right, the same scene with smooth PBR shading.*

### 4.2 Rim light

*Bogdan Mocanu and Mihai Santai · UI: Shading → Cel → Rim light, Rim color, Rim size; on or off per object under Materials*

The rim light is a thin bright band along an object's silhouette, on the side facing the sun:

$$\text{rim} = \big(1 - \max(\mathbf{n}\cdot\mathbf{v}, 0)\big)\cdot\max(\mathbf{n}\cdot\mathbf{l}, 0)^{0.1}$$

The first factor is close to 1 where the surface turns away from the camera, at the silhouette. The second factor keeps the rim on the lit side only. Raised to the power 0.1, it is almost 1 everywhere the sun reaches and falls to 0 only at the shadow line. The result goes through a smoothstep at $1 - \text{rimSize}$, with an `fwidth` edge, so the rim is a hard-edged band like the cel bands. The ground has the rim switched off, because its flat silhouette would otherwise light up at the horizon.

| Rim light off | Rim light on |
|---|---|
| ![Rim off](images/3.2-rim-light/rim_off.png) | ![Rim on](images/3.2-rim-light/rim_on.png) |

*Look at the right edges of the spheres.*

### 4.3 Outlines from edge detection

*Bogdan Mocanu · UI: Outlines → Show outlines, Depth threshold, Normal threshold, Thickness, Ink color, Debug: show edges only*

The outlines are found in screen space, in the final pass (`shaders/tonemap_frag.glsl`). The scene pass stores two images that this pass reads: the depth texture, and the surface normal in a second colour attachment. For every pixel, the pass applies the Sobel operator [13] to both images over a 3×3 neighbourhood:

$$G_x = \begin{bmatrix}-1&0&1\\-2&0&2\\-1&0&1\end{bmatrix} * I,\qquad G_y = \begin{bmatrix}-1&-2&-1\\0&0&0\\1&2&1\end{bmatrix} * I,\qquad |G| = \sqrt{G_x^2 + G_y^2}$$

- **Depth edges** find silhouettes, where one object is in front of another or of the sky. The depth buffer is not linear, so it is first converted back to a distance: $z = \dfrac{2nf}{f + n - (2d-1)(f-n)}$ for the perspective cameras and $z = n + d\,(f-n)$ for the orthographic top camera. The gradient is then divided by the pixel's own distance. Without this, near objects would get far more edges than distant ones, and the sloping ground would turn black near the horizon.
- **Normal edges** find creases, where the depth is continuous but the surface turns, such as the edges of the cube and the folds of the dragon. Here $G_x$ and $G_y$ are vectors, and their combined length is used.

A pixel becomes ink if either value is above its threshold. "Thickness" scales the distance between the samples, which makes the lines wider.

**Why screen space.** One pass outlines the whole scene, independently of how many meshes there are, and it finds the inner creases as well as the silhouettes. Edge detection on depth and normal images comes from Saito and Takahashi [11]. The alternative, drawing an enlarged back-face copy of each mesh (the "inverted hull", planned in Section 4.9), only gives silhouettes and costs an extra draw per object.

| Outlines off | Outlines on | Debug: edges only |
|---|---|---|
| ![Outlines off](images/3.4-edge-detection-outlines/outlines_off.png) | ![Outlines on](images/3.4-edge-detection-outlines/outlines_on.png) | ![Edges only](images/3.4-edge-detection-outlines/edge_debug_view.png) |

### 4.4 HDR rendering and tonemapping

*Bogdan Mocanu · UI: HDR → Exposure, Tonemapping (Reinhard)*

The scene is rendered into a framebuffer with 16-bit float colour (`GL_RGBA16F`) instead of the 8-bit screen. A normal screen clips every value above 1, but here the sun has intensity 3 and the cart is emissive with values up to 3. Keeping those values is what lets bloom (Section 4.5) find the parts that are brighter than white. The final pass draws one triangle that covers the whole screen, with its corners computed from `gl_VertexID`, so it needs no vertex buffer. For every pixel it then:

1. adds the bloom;
2. multiplies by the exposure;
3. draws the outline ink;
4. applies Reinhard tonemapping [10] per channel, $c' = \dfrac{c}{1+c}$;
5. applies gamma, $c'^{1/2.2}$.

**Why Reinhard.** It is the simplest global operator. It maps $[0,\infty)$ into $[0,1)$ without clipping, and it leaves dark values almost unchanged, because $c/(1+c)\approx c$ for small $c$. Exposure scales the image before it, like a camera's shutter time. Its limitation is that bright colours lose saturation and never become pure white.

| Exposure 2, no tonemapping | Exposure 2, Reinhard |
|---|---|
| ![Tonemapping off](images/1.6-hdr-tonemapping/exposure2_tonemap_off.png) | ![Tonemapping on](images/1.6-hdr-tonemapping/exposure2_tonemap_on.png) |

*Left: values above 1 are clipped (sky, lit sides of the spheres). Right: Reinhard compresses them back into range, at the cost of a duller image.*

### 4.5 Bloom

*Bogdan Mocanu · UI: Bloom → Show bloom, Bloom intensity, Bloom threshold, Blur passes, Debug: show bloom only; Emissive colour per object under Materials*

Bloom makes very bright parts of the image glow into their surroundings, the way a camera lens does. It uses the HDR colour from Section 4.4 and runs at half the screen resolution (`src/features/bloom.cpp`).

1. **Bright pass.** For every pixel with brightness $m = \max(r, g, b)$, the colour is scaled by $\max(m - T, 0)/m$, where $T$ is the threshold. Only the part of the brightness above the threshold is kept, and the colour's hue stays the same.
2. **Blur.** A Gaussian blur with 9 taps (the centre plus 4 on each side) is applied horizontally, then vertically. A 2D Gaussian is separable, so the two 1D passes give the same result as one 9×9 kernel, with 18 texture reads per pixel instead of 81. The two passes swap between two textures ("ping-pong"), so one is read while the other is written. The pair is repeated $N$ times ("Blur passes", default 5). Repeating a Gaussian blur gives a wider Gaussian, about $\sqrt{N}$ times as wide.
3. **Composite.** The final pass adds $\text{bloom}\times\text{intensity}$ to the scene colour before exposure and tonemapping.

Objects glow when their emissive colour is above the threshold. In the demo these are the cart and the markers of the point lights.

**Why this approach.** Half resolution costs a quarter of the pixels, and in screen pixels the blur reaches twice as far. A separable Gaussian with repeated passes is the standard simple bloom [2]. Downsampling through a chain of mipmaps would give a wider glow for less work, but it needs one framebuffer per level and more passes.

| Bloom off | Bloom on | Debug: bloom only |
|---|---|---|
| ![Bloom off](images/4.2-bloom/bloom_off.png) | ![Bloom on](images/4.2-bloom/bloom_on.png) | ![Bloom only](images/4.2-bloom/bloom_debug_view.png) |

*The debug view shows the blurred bright-pass texture that is added to the image.*

### 4.6 Shadow mapping

*Mihai Santai · UI: Shadows*

A shadow map tells the shader which points the sun can see. The first pass of the frame renders the scene's depth from the sun's point of view into a depth texture (Section 2, pass 1) [17]. In the main shader, every fragment is transformed into the same light space. Its depth is compared with the stored depth, and the fragment is in shadow if the map holds something nearer to the light. A small depth bias stops surfaces from shadowing themselves ("shadow acne").

Percentage-closer filtering (PCF) [18] softens the shadow edge. Instead of one comparison, the shader compares the fragment with several neighbouring texels and averages the results, which gives a shadow value between 0 and 1.

**Shadow map.** The sun's map is a 2048 × 2048 depth texture (`GL_DEPTH_COMPONENT32F`, nearest filtering, clamped at the edges) with its own framebuffer, as in Practical 4 [1]. The sun's rays are parallel, so the light camera uses an orthographic projection, not a perspective one. It sits 6 units from the scene centre in the direction of the sun, looks at the centre, and sees a box of $8 \times 8$ units (near plane 0.5, far plane 12). One texel therefore covers about $8/2048 \approx 0.004$ units. The box covers the ground plane, which is 6 units wide, and all the objects.

**Test.** The fragment's world position is multiplied by the light's projection and view matrix, divided by $w$, and mapped from $[-1, 1]$ to $[0, 1]$. The result is a texture coordinate and a depth. The fragment is lit when

$$d_{\text{fragment}} - b \le d_{\text{map}},$$

where $b$ is the bias, 0.003 by default, in the same $[0, 1]$ depth units (about 0.035 world units for this box). At lower values the lit side of the spheres showed fine speckles, which is shadow acne. At 0.003 the speckles were gone, and at the values tested above it (up to 0.005) shadows still touched their objects.

**PCF.** The shader averages the test over a square of $(2r+1)^2$ texels around the point, by default $r = 1$, so 9 comparisons (the slider goes from $r = 1$ to 4, which is 81). The shadow value is the fraction of lit samples, between 0 and 1. The comparisons are done by hand in the shader, as in Practical 4, so that every step stays visible.

**How the shadow enters the lighting.** The sun's radiance is multiplied by the shadow value before it goes into `shadeLight()`. In PBR mode both the diffuse and the specular part of the sun vanish in full shadow. In cel mode the banded diffuse light and the hard highlight vanish as well, and the surface keeps only the cool shadow tint of Section 4.1 and the point lights. Ambient light, environment reflections, rim light and emissive colour are not affected by shadows. A fragment outside the map (outside $[0,1]^2$, or beyond the far plane) counts as lit.

**Why this approach.** An orthographic map suits a light with parallel rays, and a fixed box around the small scene needs no fitting to the camera. A 2048 map is sharp enough at this scene size. The fixed box has a limit: at some sun directions, the outermost corners of the ground can fall outside the box, where nothing is shadowed. Hardware depth comparison (`sampler2DShadow`) would give a free 2 × 2 filter, but a manual comparison follows the exercise and is easier to explain.

| Shadows off | Shadows on | Debug: shadow-map depth |
|---|---|---|
| ![Shadows off](images/3.5-shadow-mapping/shadows_off.png) | ![Shadows on](images/3.5-shadow-mapping/shadows_on.png) | ![Shadow map depth](images/3.5-shadow-mapping/debug_shadow_map_depth.png) |

*The debug view shows the depth seen from the sun, darker is nearer to the sun.*

| PCF off, close-up | PCF radius 4, close-up | Bias 0 |
|---|---|---|
| ![PCF off](images/3.5-shadow-mapping/pcf_off_closeup.png) | ![PCF radius 4](images/3.5-shadow-mapping/pcf_radius4_closeup.png) | ![Bias 0](images/3.5-shadow-mapping/bias_zero_acne.png) |

*Left and middle: the robot's shadow edge, hard and blocky without PCF and soft with radius 4. Right: with the bias at 0, the whole floor and the spheres are covered with acne stripes.*

### 4.7 Shadows for multiple light sources

*Mihai Santai · UI: Shadows, Point lights*

Each of the two coloured point lights has its own shadow map (Section 2, pass 1). A point light shines in every direction, so a single flat depth image from one viewpoint cannot cover the scene around it.

**Light camera.** Each point light has its own 2048 × 2048 depth map, built by the same code as the sun's. The light camera sits at the light and looks straight down. It uses a perspective projection like the light camera of Practical 4 [1], with a field of view of 140°, aspect ratio 1 because the map is square, near plane 0.2 and far plane 8. It is one map per light, not a cube map of six faces. Both lights hang above the scene, and everything they light is below them, so a downward cone is enough, and it costs 2 depth passes per frame instead of 12.

**Limits of the cone.** A point outside the cone, more than 70° away from straight down, counts as lit. At a light height of 1 unit, the cone reaches about 2.7 units sideways along the ground. Beyond that distance, shadows are missing, but the light is also weak there, because its radiance falls with the square of the distance. Surfaces that face the light from the side, at the height of the light, are not covered either.

**Test.** It is the sun's test (Section 4.6) with a different matrix, map and bias, in one shader function `shadowVisibility(map, matrix, bias)` for all three lights. The compared depths are the non-linear depths of a perspective projection: they have more precision close to the light and less far away, so a constant bias covers less distance near the light than far from it. That is why the point lights have their own bias, 0.004 by default. At 0.001 and 0.002, the spheres next to the orange light showed ring-shaped acne. At 0.004 it was clean, and the thin shadow of the robot arm stayed complete up to 0.008.

**Combining the lights.** Each light is handled on its own. The sun's radiance is multiplied by its shadow value, and the radiance of each point light (already divided by the squared distance) by its own, and each product goes through `shadeLight()`. A point that is in the shadow of one light still receives the other lights. An object lit by both point lights casts two shadows, and where two shadows overlap the point receives no point light at all. "Debug: shadow test" shows one light's shadow value as white (lit) and black (in shadow).

**Why this approach.** The cone covers what matters in this scene with the least work. A cube map would cover all directions, but it needs six passes and a cube sampler for each light. The shadows of the point lights are hardest to see in the full scene, because the sun's light dominates, so the screenshots below switch the sun off.

| Point-light shadows off | Point-light shadows on |
|---|---|
| ![Point-light shadows off](images/4.1-point-light-shadows/point_shadows_off.png) | ![Point-light shadows on](images/4.1-point-light-shadows/point_shadows_on.png) |

| Off, top view | On, top view |
|---|---|
| ![Off, top view](images/4.1-point-light-shadows/point_shadows_off_top.png) | ![On, top view](images/4.1-point-light-shadows/point_shadows_on_top.png) |

*Both point lights are on and the sun is off. In the top view, every object has two shadows pointing away from the two lights.*

| Debug: orange light | Debug: blue light |
|---|---|
| ![Shadow test, light 1](images/4.1-point-light-shadows/debug_shadow_test_light1_top.png) | ![Shadow test, light 2](images/4.1-point-light-shadows/debug_shadow_test_light2_top.png) |

*White is lit and black is in shadow. Outside the cone of a light, the test returns white.*

### 4.8 Constant speed along a Bézier curve

*Mihai Santai · UI: Constant speed*

With the parameter $t$ of Section 3.6, the cart's speed is $|\mathbf{B}'(t)|$ times the change of $t$ per second. The length of the derivative differs between segments and along a segment, so the cart speeds up and slows down even though $t$ grows steadily. Reparameterising the curve by arc length removes this.

A Bézier curve has no simple closed formula for its length, so a table is built once: the curve is sampled at many values of $t$, and the table stores the accumulated length $s(t)$ at each sample [19]. Each frame, the distance travelled, $s = \text{time}\times\text{speed}$, is wrapped to the total length $L$ of the loop. The two samples around $s$ are found in the table, and $t$ is interpolated linearly between them. The cart is then drawn at $\mathbf{B}(t)$.

**Table.** The curve is sampled 200 times per segment, which is 1200 samples for the six segments. The table stores the accumulated length at each sample, adding up the lengths of the short straight pieces between neighbouring samples. This approximates the curve by a polyline. The error shrinks with the square of the number of pieces, so 200 per segment is enough. The total length of the loop is $L = 15.59$ units.

**Lookup.** The table is sorted, so the sample just beyond the distance $s$ is found by binary search (`std::upper_bound`), which takes about 11 steps for 1201 entries. $t$ is then interpolated linearly between that sample and the one before it. The distance is wrapped to $[0, L)$ first, so the cart loops, and negative distances work too.

**Speed slider.** The slider "Cart speed" keeps its meaning of segments per second, so switching the option does not change how long a lap takes. With the option on, the distance covered per second is $\text{speed}\times L/6$, and the cart's speed is constant. At the default of 0.3, a lap takes 20 seconds in both modes, and the average speed is $0.3 \times 15.59/6 = 0.78$ units per second.

**Measured.** The speed of the cart was measured over one lap by differencing its position every 5 ms:

| | slowest | fastest | fastest / slowest |
|---|---|---|---|
| parameter $t$ grows steadily (option off) | 0.648 | 0.912 | 1.41 |
| arc-length parameterisation (option on) | 0.778 | 0.781 | 1.00 |

*Units per second. Both have the same mean of 0.780 and the same lap time.*

**Comparison.** "Show markers" draws a number of points on the curve. The orange, larger markers are at equal steps of $t$, and the cyan, smaller ones are at equal steps of distance along the curve. Equal steps of $t$ bunch up where the curve is tight and spread out where it is wide. Equal steps of distance are evenly spaced.

**Why this approach.** The table is built once, and each frame costs a binary search. It uses only the position function of the path, so the path code did not have to change. The table has to be rebuilt if the control points are ever edited, which the demo does not allow.

| Markers, top view | Markers, side view | 60 markers, top view |
|---|---|---|
| ![Markers, top view](images/4.3-constant-speed/markers_top_view.png) | ![Markers, side view](images/4.3-constant-speed/markers_side_view.png) | ![60 markers, top view](images/4.3-constant-speed/markers_60_top_view.png) |

*Orange: equal steps of $t$. Cyan: equal steps of distance. In the side view, the orange markers bunch up at the tight turn on the left, while the cyan ones keep their spacing.*

### 4.9 Planned features (Mihai Santai)

*In progress. These sections will be completed when the features are done.*

- **Inverted-hull outlines.** A second pass per mesh that draws only the back faces, pushed outwards along the normals, in the ink colour. It will be compared with the screen-space outlines in Section 4.3.
- **Particle effects.** Based on the particle simulation of Exercise 5 [1].
- **Animated textures.** A sequence of frames shown one after another. The frame is picked from the animation time, so the speed does not depend on the frame rate.
- **Day-night cycle.** The sun's direction, colour and intensity change over time.
- **Minimap.** The top camera renders into a texture, which is shown on a quad in a corner of the screen.

---

## 5. References

**Course material**

[1] CS4515 3D Computer Graphics and Animation, TU Delft, 2026: lecture slides and practical assignments (Practical 1, Practical 3 on shading, Practical 4 on cameras and shadow mapping, Exercise 5 on particles, Exercise 6 on Bézier curves).

**Tutorials**

[2] J. de Vries. *LearnOpenGL*. https://learnopengl.com. Chapters "Cubemaps", "Normal Mapping", "HDR", "Bloom", "PBR: Theory" and "PBR: Lighting". [TODO Bogdan: keep only the chapters actually used]

**Papers**

[3] R. L. Cook and K. E. Torrance. A Reflectance Model for Computer Graphics. *ACM Transactions on Graphics* 1(1), 1982.

[4] B. Walter, S. R. Marschner, H. Li and K. E. Torrance. Microfacet Models for Refraction through Rough Surfaces. *Eurographics Symposium on Rendering*, 2007.

[5] B. Karis. Real Shading in Unreal Engine 4. *SIGGRAPH 2013 Course: Physically Based Shading in Theory and Practice*, 2013.

[6] C. Schlick. An Inexpensive BRDF Model for Physically-based Rendering. *Computer Graphics Forum* 13(3), 1994.

[7] J. F. Blinn. Models of Light Reflection for Computer Synthesized Pictures. *SIGGRAPH*, 1977.

[8] J. F. Blinn. Simulation of Wrinkled Surfaces. *SIGGRAPH*, 1978.

[9] J. F. Blinn and M. E. Newell. Texture and Reflection in Computer Generated Images. *Communications of the ACM* 19(10), 1976.

[10] E. Reinhard, M. Stark, P. Shirley and J. Ferwerda. Photographic Tone Reproduction for Digital Images. *ACM Transactions on Graphics* 21(3), 2002.

[11] T. Saito and T. Takahashi. Comprehensible Rendering of 3-D Shapes. *SIGGRAPH*, 1990.

[12] A. Lake, C. Marshall, M. Harris and M. Blackstein. Stylized Rendering Techniques for Scalable Real-Time 3D Animation. *NPAR*, 2000.

[13] I. Sobel and G. Feldman. A 3×3 Isotropic Gradient Operator for Image Processing. Talk at the Stanford Artificial Intelligence Project, 1968.

[17] L. Williams. Casting Curved Shadows on Curved Surfaces. *SIGGRAPH*, 1978.

[18] W. T. Reeves, D. H. Salesin and R. L. Cook. Rendering Antialiased Shadows with Depth Maps. *SIGGRAPH*, 1987.

[19] B. Guenter and R. Parent. Computing the Arc Length of Parametric Curves. *IEEE Computer Graphics and Applications* 10(3), 1990.

**Assets**

[14] ambientCG. *Painted Metal 006* (CC0). https://ambientcg.com/view?id=PaintedMetal006

[15] Stanford 3D Scanning Repository. *Dragon*. http://graphics.stanford.edu/data/3Dscanrep/ (provided with the course practicals).

[16] Skybox: [TODO Bogdan: source and licence of the six cube map images in `resources/skybox/`].
