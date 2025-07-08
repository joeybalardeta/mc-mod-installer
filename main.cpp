#include <iostream>
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <wininet.h>
#include <algorithm>
#pragma comment(lib, "wininet.lib")

// constants
const std::string JAVA_INSTALLER_URL = "https://github.com/adoptium/temurin17-binaries/releases/latest/download/OpenJDK17U-jre_x64_windows_hotspot_17.msi";
const std::string FABRIC_INSTALLER_URL = "https://maven.fabricmc.net/net/fabricmc/fabric-installer/1.0.3/fabric-installer-1.0.3.jar";
const std::string FABRIC_LOADER_VERSION = "0.14.9"; // Fabric loader version
const std::string MINECRAFT_VERSION = "1.20.1"; // Minecraft version to install Fabric for

// define all functions here to avoid linking issues

// cor functions
std::vector<std::string> split_path(const std::string& path, char delimiter);
void create_directory(const std::string& path);
bool is_java_installed();
void validate_java_installation();
bool is_fabric_installed(const std::string& minecraft_dir, const std::string& mcversion, const std::string& loader_version);
void validate_fabric_installation(const std::string& mcversion, const std::string& loader_version);
void download_file(const std::string& url, const std::string& output_path);
std::string safe_getenv(const char* var);
bool is_version_greater_or_equal(const std::string& installed_version, const std::string& required_version);

// test functions
void test_all();
void test_file_download();

// Helper to split a path into its components
std::vector<std::string> split_path(const std::string& path, char delimiter = '\\') {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            parts.push_back(item);
        }
    }
    return parts;
}

// Recursively create directories in a path (Windows API)
void create_directory(const std::string& path) {
    std::vector<std::string> parts = split_path(path);
    std::string current;
    if (path.size() > 1 && path[1] == ':') {
        current = parts[0] + "\\";
        parts.erase(parts.begin());
    }
    for (const auto& part : parts) {
        if (!current.empty() && current.back() != '\\')
            current += "\\";
        current += part;
        if (!CreateDirectoryA(current.c_str(), NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "Failed to create directory: " << current << std::endl;
                return;
            }
        } else {
            std::cout << "Created directory: " << current << std::endl;
        }
    }
}

// Check if Java is installed by running 'java -version'
bool is_java_installed() {
    int result = system("java -version >nul 2>&1");
    if (result == 0) {
        std::cout << "Java is installed." << std::endl;
        return true;
    } else {
        std::cout << "Java is NOT installed." << std::endl;
        return false;
    }
}

void validate_java_installation() {
    if (is_java_installed()) {
        std::cout << "Java is properly installed." << std::endl;
        return;
    }

    std::cout << "Java is not installed. Attempting to download and install Adoptium OpenJDK..." << std::endl;
    std::string temp_dir = safe_getenv("TEMP");
    std::string java_installer_path = temp_dir + "\\OpenJDK17U-jre_x64_windows_hotspot_17.msi";

    download_file(JAVA_INSTALLER_URL, java_installer_path);

    std::cout << "Running Java installer..." << std::endl;
    std::string install_cmd = "msiexec /i \"" + java_installer_path + "\" /qn /norestart";
    int result = system(install_cmd.c_str());
    if (result != 0) {
        std::cerr << "Java installer failed. Please install Java manually from https://adoptium.net/" << std::endl;
        exit(1);
    }

    // Re-check Java installation
    if (is_java_installed()) {
        std::cout << "Java installed successfully." << std::endl;
    } else {
        std::cerr << "Java installation failed. Please install Java manually from https://adoptium.net/" << std::endl;
        exit(1);
    }
}

// Helper function to compare version strings (e.g., "0.16.14" >= "0.14.9")
bool is_version_greater_or_equal(const std::string& installed_version, const std::string& required_version) {
    std::vector<int> inst_parts, req_parts;
    std::stringstream inst_ss(installed_version), req_ss(required_version);
    std::string item;

    while (std::getline(inst_ss, item, '.')) {
        inst_parts.push_back(std::stoi(item));
    }
    while (std::getline(req_ss, item, '.')) {
        req_parts.push_back(std::stoi(item));
    }

    size_t max_len = std::max(inst_parts.size(), req_parts.size());
    inst_parts.resize(max_len, 0);
    req_parts.resize(max_len, 0);

    for (size_t i = 0; i < max_len; ++i) {
        if (inst_parts[i] > req_parts[i]) return true;
        if (inst_parts[i] < req_parts[i]) return false;
    }
    return true; // Versions are equal
}

// Check if a suitable Fabric version is installed
bool is_fabric_installed(const std::string& minecraft_dir, const std::string& mcversion, const std::string& required_loader_version) {
    std::string versions_dir = minecraft_dir + "\\versions";
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((versions_dir + "\\fabric-loader-*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::string dirname = findFileData.cFileName;
            std::string prefix = "fabric-loader-";
            std::string suffix = "-" + mcversion;
            if (dirname.rfind(prefix, 0) == 0 && dirname.size() > suffix.size() && dirname.substr(dirname.size() - suffix.size()) == suffix) {
                std::string installed_loader_version = dirname.substr(prefix.size(), dirname.size() - prefix.size() - suffix.size());
                if (is_version_greater_or_equal(installed_loader_version, required_loader_version)) {
                    std::cout << "Found suitable Fabric version: " << installed_loader_version << " for Minecraft " << mcversion << std::endl;
                    FindClose(hFind);
                    return true;
                }
            }
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
    std::cout << "No suitable Fabric version found for Minecraft " << mcversion << std::endl;
    return false;
}

// Validate Fabric installation by checking if it exists in the Minecraft directory for a specific version
void validate_fabric_installation(const std::string& mcversion, const std::string& loader_version) {
    std::string home_dir = safe_getenv("USERPROFILE");
    std::string minecraft_dir = home_dir + "\\AppData\\Roaming\\.minecraft";
    if (is_fabric_installed(minecraft_dir, mcversion, loader_version)) {
        return;
    }
    std::cout << "Fabric for Minecraft " << mcversion << " (loader " << loader_version << ") is not installed. Attempting to download and install Fabric..." << std::endl;
    // Download Fabric installer
    std::string temp_dir = safe_getenv("TEMP");
    std::string fabric_installer_path = temp_dir + "\\fabric-installer.jar";
    download_file(FABRIC_INSTALLER_URL, fabric_installer_path);

    // Run the Fabric installer for the specific version
    std::string install_cmd = "java -jar \"" + fabric_installer_path + "\" client -dir \"" + minecraft_dir + "\" -mcversion " + mcversion + " -loader " + loader_version;
    int result = system(install_cmd.c_str());
    if (result != 0) {
        std::cerr << "Fabric installer failed. Please install Fabric manually from https://fabricmc.net/use/" << std::endl;
        exit(1);
    }

    // Re-check Fabric installation
    if (is_fabric_installed(minecraft_dir, mcversion, loader_version)) {
        std::cout << "Fabric installed successfully for Minecraft " << mcversion << " (loader " << loader_version << ")." << std::endl;
    } else {
        std::cerr << "Fabric installation failed. Please install Fabric manually from https://fabricmc.net/use/installer/" << std::endl;
        exit(1);
    }
}

// File download logic using WinINet
void download_file(const std::string& url, const std::string& output_path) {
    HINTERNET hInternet = InternetOpenA("MinecraftModInstaller", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "Failed to initialize WinINet." << std::endl;
        exit(1);
    }

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        std::cerr << "Failed to open URL: " << url << std::endl;
        InternetCloseHandle(hInternet);
        exit(1);
    }

    FILE* file = nullptr;
    if (fopen_s(&file, output_path.c_str(), "wb") != 0 || !file) {
        std::cerr << "Failed to open output file: " << output_path << std::endl;
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        exit(1);
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);
    std::cout << "Downloaded: " << url << " to: " << output_path << std::endl;
}

// Safe getenv using _dupenv_s
std::string safe_getenv(const char* var) {
    char* buffer = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buffer, &sz, var) == 0 && buffer != nullptr) {
        std::string value(buffer);
        free(buffer);
        return value;
    }
    return std::string();
}

// test functions

// test all functions
void test_all() {
    std::cout << "Testing all functions..." << std::endl;
    test_file_download();
}

// test file download function
void test_file_download() {
    std::string url = JAVA_INSTALLER_URL;
    std::string home_dir = safe_getenv("USERPROFILE");
    std::string output_path = home_dir + "\\Downloads\\OpenJDK17U-jre_x64_windows_hotspot_17.msi";
    download_file(url, output_path);
    std::cout << "File downloaded to: " << output_path << std::endl;
}

// main function to run the setup script
int main() {
    // Get the user's home directory
    std::string home_dir = safe_getenv("USERPROFILE");
    std::string modded_install_dir = home_dir + "\\Games\\Minecraft\\modded-install";
    std::string minecraft_dir = home_dir + "\\AppData\\Roaming\\.minecraft";

    std::cout << "Creating Minecraft modded install directory" << std::endl;
    create_directory(modded_install_dir);

    // check Java installation, install if not found
    validate_java_installation();

    // check Fabric installation, install if not found
    validate_fabric_installation(MINECRAFT_VERSION, FABRIC_LOADER_VERSION);

    std::cout << "Setup script completed." << std::endl;
    return 0;
}
