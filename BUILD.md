# Building WeaR-emu

This guide covers building WeaR-emu from source on Windows.

---

## Prerequisites

### Required Software

| Software | Version | Download |
|----------|---------|----------|
| **Visual Studio 2022** | 17.8+ | [Download](https://visualstudio.microsoft.com/) |
| **Qt** | 6.10.1+ | [Download](https://www.qt.io/download) |
| **Vulkan SDK** | 1.3.335+ | [Download](https://vulkan.lunarg.com/sdk/home) |
| **CMake** | 3.28+ | [Download](https://cmake.org/download/) |
| **Git** | Latest | [Download](https://git-scm.com/) |

### Visual Studio Components

When installing Visual Studio, ensure these workloads are selected:

- ✅ Desktop development with C++
- ✅ C++ CMake tools for Windows
- ✅ Windows SDK (Latest)
- ✅ MSVC v143 build tools

### Qt Configuration

Install the following Qt components:

```
Qt 6.10.1
├── MSVC 2022 64-bit
├── Qt Multimedia
├── Qt Shader Tools
└── Additional Libraries
    └── Qt 5 Compatibility Module
```

Set the `Qt6_DIR` environment variable:

```powershell
setx Qt6_DIR "C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6"
```

### Vulkan SDK

After installing the Vulkan SDK, verify installation:

```powershell
echo %VULKAN_SDK%
# Should output: C:\VulkanSDK\1.3.335.0
```

---

## Build Instructions

### 1. Clone Repository

```bash
git clone https://github.com/RidTheWann/WeaR-emu.git
cd WeaR-emu
```

### 2. Initialize Submodules

```bash
git submodule update --init --recursive
```

### 3. Configure CMake

#### Option A: Command Line

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

#### Option B: CMake GUI

1. Open CMake GUI
2. Set source directory to `WeaR-emu`
3. Set build directory to `WeaR-emu/build`
4. Click **Configure** → Select "Visual Studio 17 2022", platform "x64"
5. Click **Generate**

### 4. Build

#### Debug Build

```bash
cmake --build . --config Debug
```

#### Release Build

```bash
cmake --build . --config Release
```

#### Or open in Visual Studio

```bash
start WeaR-emu.sln
```

---

## Build Output

After building, executables will be in:

```
build/
├── Debug/
│   └── WeaR-emu.exe
└── Release/
    └── WeaR-emu.exe
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `WEAR_ENABLE_VALIDATION` | `ON` (Debug) | Enable Vulkan validation layers |
| `WEAR_ENABLE_FRAME_GEN` | `ON` | Enable WeaR-Gen frame generation |
| `WEAR_BUILD_TESTS` | `OFF` | Build unit tests |
| `WEAR_USE_ASAN` | `OFF` | Enable AddressSanitizer |

Example:

```bash
cmake .. -DWEAR_ENABLE_VALIDATION=OFF -DWEAR_BUILD_TESTS=ON
```

---

## Troubleshooting

### Qt not found

```
CMake Error: Could not find a package configuration file provided by "Qt6"
```

**Solution:** Set `Qt6_DIR` environment variable:

```powershell
cmake .. -DQt6_DIR="C:/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6"
```

### Vulkan SDK not found

```
CMake Error: Could not find Vulkan
```

**Solution:** Ensure `VULKAN_SDK` is set and run:

```powershell
cmake .. -DVULKAN_SDK="C:/VulkanSDK/1.3.335.0"
```

### Missing DLLs at runtime

Copy required DLLs to build output:

```powershell
windeployqt.exe Release/WeaR-emu.exe
```

### Shader compilation errors

Ensure `glslc` is in PATH:

```powershell
where glslc
# Should output: C:\VulkanSDK\1.3.335.0\Bin\glslc.exe
```

---

## Updating Dependencies

### Update Submodules

```bash
git submodule update --remote --merge
```

### Clean Build

```bash
cd build
cmake --build . --target clean
cmake ..
cmake --build . --config Release
```

---

## Recommended Development Environment

| IDE | Extensions |
|-----|------------|
| **Visual Studio 2022** | C++ Vulkan, Qt VS Tools |
| **VS Code** | C/C++, CMake Tools, Qt Tools |
| **CLion** | Built-in CMake support |

---

## Notes

- WeaR-emu requires a **Vulkan 1.3** capable GPU
- Minimum 4GB VRAM recommended
- Windows 10 21H2 or later for best compatibility
- WeaR-Gen requires 8GB+ VRAM for optimal performance

---

<p align="center">
  <em>Happy Building!</em>
</p>
