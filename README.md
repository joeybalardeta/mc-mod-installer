﻿# Minecraft Mod Installer

A comprehensive C++ automation tool designed to completely set up a modded Minecraft instance for "The Cove - Season 8" modpack on Windows. This installer handles everything from prerequisite checks to complete modpack installation, making it easy to get up and running with minimal user intervention.

## Features

### Automated Installation & Setup
- **Java Installation & Management:** 
  - Automatically checks for compatible Java version (Java 21+ required)
  - Downloads and installs Oracle JDK 22 if no suitable version is found
  - Advanced Java detection in common installation directories
  - Automatic PATH management and environment variable updates
  - Falls back to multiple search strategies if initial detection fails

- **Fabric Loader Installation:**
  - Checks for existing Fabric installations for Minecraft 1.20.1
  - Downloads and runs the official Fabric installer (v1.0.3) with correct parameters
  - Automatically configures for the target Minecraft version and Fabric loader version

- **Minecraft Launcher Integration:**
  - Creates a custom launcher profile named "The Cove - Season 8 (1.20.1)"
  - Configures optimized JVM arguments for better performance
  - Sets up separate game directory for the modded installation
  - Automatically detects and configures the correct Java executable path

- **Complete Modpack Setup:**
  - Downloads "The Cove - Season 8" modpack from the configured URL
  - Extracts mods to the dedicated modded installation directory
  - Smart download caching - skips re-downloading if modpack already exists
  - Interactive workflow - prompts user to launch and close Minecraft before mod installation

### Technical Features
- **Robust Directory Management:** Creates necessary directory structures automatically
- **Safe File Operations:** Uses secure Windows APIs for file and network operations
- **Version Comparison:** Intelligent version string parsing and comparison
- **Network Downloads:** Built-in HTTP download functionality using WinINet
- **JSON Profile Management:** Reads and modifies Minecraft launcher profiles safely
- **Environment Integration:** Handles Windows environment variables and PATH updates
- **Error Handling:** Comprehensive error checking and user-friendly messages

## Configuration

The installer is pre-configured for "The Cove - Season 8" but can be customized by modifying the constants in `constants.hpp`.

### Java Configuration
- Oracle JDK 22 installer from official download archive
- Minimum required Java version: 21

### Fabric Configuration  
- Fabric installer version: 1.0.3 from official Maven repository
- Target Fabric loader version: 0.16.14

### Modpack Configuration
- Target Minecraft version: 1.20.1
- Modpack source: "The Cove - Season 8" client mods package from Dropbox

## How to Use

### Prerequisites
- Windows operating system
- Administrator privileges (recommended for Java installation)
- Active internet connection
- Minecraft Java Edition installed

### Installation Steps

1. **Download & Compile:**
   - Clone or download this repository
   - Compile using Visual Studio or any C++14 compatible compiler
   - Ensure all dependencies are linked (WinINet, Windows API)

2. **Run the Installer:**
   - Execute the compiled program as Administrator (recommended)
   - The installer will automatically:
     - Check and install Java if needed
     - Install Fabric loader for Minecraft 1.20.1
     - Create the modded installation directories
     - Set up a Minecraft launcher profile
     - Download and install the modpack

3. **Launch & Play:**
   - Use the "The Cove - Season 8 (1.20.1)" profile in the Minecraft launcher
   - Enjoy your modded Minecraft experience!

## Project Structure
```bash
mc-mod-installer/
├── main.cpp              # Main application logic and orchestration
├── constants.hpp         # Configuration constants
├── filesystem.hpp        # File system function declarations  
├── filesystem.cpp        # File operations, downloads, and utility functions
├── json.hpp              # JSON library for launcher profile management
└── README.md             # This file
```

## Technical Requirements

- **Compiler:** C++14 compatible compiler (Visual Studio 2017+ recommended)
- **Platform:** Windows 7/8/10/11 (x64)
- **Dependencies:** Windows SDK, WinINet library
- **Runtime:** Windows with .NET Framework (for Minecraft launcher compatibility)

## Troubleshooting

### Java Installation Issues
- Run as Administrator if Java installation fails
- Restart your computer if Java is not detected after installation
- Check Windows PATH manually if automatic detection fails

### Fabric Installation Issues  
- Ensure Minecraft has been run at least once before installing Fabric
- Check that the `.minecraft` directory exists in your user profile
- Verify internet connectivity for downloading the Fabric installer

### Modpack Download Issues
- Check internet connectivity
- Verify the modpack URL is accessible
- Ensure sufficient disk space for the modpack download and extraction

## License
MIT License

Copyright (c) 2025 Joey Balardeta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
