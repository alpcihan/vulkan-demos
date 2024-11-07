# Vulkan Demos

A series of demos to experiment with the Vulkan API.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Demos](#demos)
   - [Demo-01: Triangle](#demo-01-triangle)
   - [Demo-02: Forward Rendering](#demo-02-forward-rendering)
   - [Demo-03: Deferred Rendering + Post Processing](#demo-03-deferred-rendering--post-processing)
   - [Demo-04: Instanced Rendering + Dynamic Batching](#demo-04-instanced-rendering--dynamic-batching)
   - [Demo-05: GPU Driven Rendering](#demo-05-gpu-driven-rendering)
   - [Demo-06: Path Tracing + Motion Vectors](#demo-06-path-tracing--motion-vectors)
   - [Demo-07: Path Tracing with BVH](#demo-07-path-tracing-with-bvh)
   - [Demo-08: Path Tracing with RTX](#demo-08-path-tracing-with-rtx)
3. [License](#license)


## Getting started

- Download the repository with it's dependencies with: 
    ```
    git clone --recursive https://github.com/alpcihan/vulkan-demos
    ```
- Use `CMakeLists.txt` at the root to build the project with CMake. It will create corresponding demo executables with `demo-<id>` naming format (e.g., `demo-01.exe`).

## Demos

### Demo-01 (Triangle)

#### Features
1) A true work of art

#### Screenshot
<img width="360" alt="" src="./resources/screenshots/demo-01.jpg">


### Demo-02 (Forward Rendering)

#### Features:
1) Forward rendering
2) Blinn-Phong shading

#### Screenshot
<img width="360" alt="" src="./resources/screenshots/demo-02.jpg">


### Demo-03 (Deferred Rendering + Post Processing)

#### Features:
1) Deferred rendering
2) Blinn-Phong shading with dozens of ligts
3) Post processing pass with dithering effect

#### Screenshots:
<img width="360" alt="" src="./resources/screenshots/demo-03_without-post-processing.jpg">
<br>
<img width="360" alt="" src="./resources/screenshots/demo-03_post-processing.JPG">


### Demo-04 (Instanced Rendering + Dynamic Batching)

#### Features:
1) Instanced Rendering
2) Dynamic Batching
3) Runtime mesh loading

#### How to use
##### Runtime obj loading
- Add <modelname\>.obj and <modelname\>.yaml under [resources/models](./resources/models)
- Add <modelname\>.png under [resources/textures](./resources/textures)
##### Runtime config update
- Update <modelname\>.yaml (position, scale, instance count, pattern)
##### Camera controller
- Use WASD, Shift Space and arrows to move the scene camera.

#### Video
<img width="360" alt="" src="resources/screenshots/demo-04.gif">


### Demo-05 (GPU Driven Rendering)

#### Features
1) GPU driven rendering pipeline
2) Instanced indirect rendering
3) Frustum culling with compute shader

#### How to use
##### Camera controller
- Use WASD, Shift Space and arrows to move the scene camera.

#### Video
<img width="360" alt="" src="resources/screenshots/demo-05.gif">


### Demo-06 (Path Tracing + Motion Vectors)

#### Features
1) Path tracing with Monte Carlo method
2) Motion vector texture generation
3) Depth texture generation
4) Recovering diffuse light info with depth + motion vector thresholds

#### Video
<img width="360" alt="" src="resources/screenshots/demo-06_motion-vectors.gif">


### Demo-07 (Path Tracing with BVH)

#### Features
1) Path tracing using BVH

#### Screenshot
<img width="360" alt="" src="resources/screenshots/demo-07.jpg">


### Demo-08 (Path Tracing with RTX)

#### Features
1) Path tracing using RTX

#### Screenshot
<img width="360" alt="" src="resources/screenshots/demo-08.jpg">


## Licens
[MIT license](./LICENSE).