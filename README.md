# Divine
Learn Vulkan from Vulkan samples.
The author is too lazy to write a formal README😚.

## Table of Contents
*[Prerequired Tools](#prerequired-tools)

*[Build&Run](#buildrun)

*[BriefIntro](#brief-intro)

*[Blending](#blending)

*[Scene](#our-scene)

## Prerequired Tools
+ [C/C++ Compiler](https://llvm.org)
+ [CMake](https://cmake.org)
+ [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

## Build&Run
### Download this repository
Start by cloning this repository. Open a terminal in your working directory and type in the following commands.
```bash
    git clone --recursive https://github.com/BravoMando/Divine.git
```
If the repository was cloned non-recursively, use the following commands to clone the necessary submodules.
```bash
    git submodule update --init
    # Alternatively using git submodule update --remote to get the latest version if you prefer
```
### Configure&Build
```bash
    mkdir build
    cd build
    cmake .. -G "Your generator" # e.g. "Unix Makefiles"
    cmake --build .
```

## Brief Intro
😚.

## Our scene
😚


## 😚
[Cadnav](http://www.cadnav.com)

Vulkan stage logical order:
VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT
VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT
VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
