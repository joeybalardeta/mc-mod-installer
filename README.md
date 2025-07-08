# Minecraft Mod Installer

This is a C++ automation script designed to set up a modded Minecraft instance on Windows. It simplifies the process of getting a modded environment ready by handling prerequisite checks and installations automatically.

## Features

- **Java Installation:** Automatically checks if a compatible version of Java is installed. If not, it downloads and installs Adoptium OpenJDK.
- **Fabric Loader Installation:** Checks if the Fabric mod loader is installed for a specific Minecraft version. If not, it downloads and runs the Fabric installer with the correct parameters.
- **Directory Setup:** Creates the necessary directories for the modded installation.
- **File Downloads:** Includes functionality to download files from the internet, used for fetching the Java and Fabric installers.

## Configuration

The script can be configured by modifying the global constants at the top of the `main.cpp` file:

- `JAVA_INSTALLER_URL`: The URL for the Java installer.
- `FABRIC_INSTALLER_URL`: The URL for the Fabric installer.
- `FABRIC_LOADER_VERSION`: The target version of the Fabric loader to install.
- `MINECRAFT_VERSION`: The target Minecraft version for the Fabric installation.

## How to Use

1.  **Configure:** Open `main.cpp` and adjust the constant variables to your desired versions.
2.  **Compile:** Compile the C++ code using a C++14 compatible compiler.
3.  **Run:** Execute the compiled program. The script will perform the checks and installations, logging its progress to the console.
