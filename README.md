# CoRSkinning

**CoRSkinning** is a C++/OpenGL project that implements and visualizes different **skinning techniques for animating digital humans**, with a focus on **Optimized Centers of Rotation (CoR)**. It was developed as part of the dissertation project *"Creation of Animatable Digital Humans from Photogrammetry"* at the University of Surrey.  

The code provides a real-time renderer, FBX importer, and GLSL shaders to compare **Linear Blend Skinning (LBS)**, **Dual Quaternion Skinning (DQS)**, and **Optimized CoR Skinning** on reconstructed meshes.

---

## Project Motivation

Creating realistic digital humans requires accurate deformation when animated. Traditional methods like **LBS** suffer from artifacts such as joint collapse and the "candy-wrapper effect." **DQS** improves volume preservation but introduces bulging artifacts.  

**Optimized CoR Skinning** precomputes per-vertex centers of rotation to interpolate rigid transformations at runtime, producing smoother, more natural deformations while maintaining GPU efficiency.  

This project builds a **testbed to implement, evaluate, and visualize** these methods in the context of animatable digital humans reconstructed via photogrammetry.

---

## Features

- **FBX Importer**: Extracts mesh, skeleton, weights, and animation data via Autodesk FBX SDK  
- **Skinning Techniques**: Implementations of  
  - Linear Blend Skinning (LBS)  
  - Dual Quaternion Skinning (DQS)  
  - Optimized Centers of Rotation (CoR)  
- **GLSL Shaders**: GPU-accelerated skinning and real-time rendering  
- **Visualization**: Compare skinning methods interactively in an OpenGL window  
- **Data Export**: Save `.cor` files with precomputed centers of rotation for reuse  
- **Profiling Support**: Benchmark skinning performance  

---

## Requirements

- **C++17 or later**  
- **CMake 3.15+**  
- **OpenGL 4.0+**  
- **GLEW / FreeGLUT**   
- **Autodesk FBX SDK** (for FBX file loading)

On Ubuntu/Debian:

```bash
sudo apt-get install build-essential cmake libglew-dev freeglut3-dev
```

On macOS (Homebrew):

```bash
brew install cmake glew freeglut
```

---

## Setting Up the FBX SDK

1. **Download the SDK**  
   - [Autodesk FBX SDK](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0)  
   - Default locations:  
     - Windows: `C:/Program Files/Autodesk/FBX/FBX SDK/2020.0`  
     - Linux/Mac: `/usr/autodesk/fbx/2020.0`  

2. **Link in CMake** (example: Windows/VS2019):  

```cmake
set(FBX_SDK_PATH "C:/Program Files/Autodesk/FBX/FBX SDK/2020.0")
include_directories("${FBX_SDK_PATH}/include")
link_directories("${FBX_SDK_PATH}/lib/vs2019/x64/release")
target_link_libraries(CoRSkinning libfbxsdk-md.lib)
```

Linux/macOS example:  

```cmake
set(FBX_SDK_PATH "/usr/autodesk/fbx/2020.0")
include_directories("${FBX_SDK_PATH}/include")
link_directories("${FBX_SDK_PATH}/lib/gcc/x64/release")
target_link_libraries(CoRSkinning fbxsdk)
```

> Ensure the compiler version matches the SDK build (e.g., VS2019 with the VS2019 SDK).  
> On Linux, you may need `-D_GLIBCXX_USE_CXX11_ABI=0` depending on your FBX SDK version.  

---

## Building

```bash
git clone https://github.com/jraboufakher/CoRSkinning.git
cd CoRSkinning
mkdir build && cd build
cmake ..
make
```

---

## Usage

Run the executable with an FBX model:

```bash
./CoRSkinning <path_to_fbx_file>
```

Controls:
- **Mouse Drag** – Rotate camera  
- **Scroll** – Zoom  
- **Keys** – Switch skinning mode (`1 = LBS`, `2 = DQS`, `3 = CoR`)  

Generated `.cor` and log files are saved in `cor_output/`.

---

## Evaluation Summary

- **LBS** – Efficient but suffers from joint collapse and "candy-wrapper" artifacts  
- **DQS** – Preserves volume, avoids collapse, but introduces bulging artifacts  
- **CoR** – Precomputed centers of rotation provide smoother, artifact-free deformations with GPU efficiency  

Evaluation used both **visual inspection** and **Hausdorff distance metrics** between meshes to quantify deformation quality.

---

## References

- Kavan, L., Collins, S., Zara, J., & O'Sullivan, C. (2008). *Geometric skinning with approximate dual quaternion blending*. ACM TOG.  
- Le, B., & Hodgins, J. (2016). *Real-time skeletal skinning with optimized centers of rotation*. ACM TOG.  
- Autodesk FBX SDK documentation.  

---

## Module Header
**EEE3017 — Year 3 Project**  
**Assignment:** Creation of Animatable Digital Humans from Photogrammetry  
**Student:** Jana Abou Fakher (ID: 6655682), Academic Year: 2024–25

---

## Acknowledgements

This project builds upon and incorporates code from the **OptimisedCentresOfRotationSkinning** library by Paul Bittner et al.  
Repository: [pmbittner/OptimisedCentresOfRotationSkinning](https://github.com/pmbittner/OptimisedCentresOfRotationSkinning)  

That library is released under the **MIT License**, and I have adapted its CoR-calculation and GLSL shader logic (with modifications) into this project.  

---

## License

Copyright (c) 2025 Jana Abou Fakher

This project was created as part of **EEE3017 – Year 3 Project** at the **University of Surrey**.  
The author retains copyright. The University of Surrey holds a non-exclusive license to use this work for teaching and assessment purposes.

All other rights reserved. Redistribution or use outside of the University context requires the author’s permission.
