#define NOMINMAX

#include "filesystem.hpp"
#include "json.hpp"

#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <wininet.h>
#include <fstream>
#pragma comment(lib, "wininet.lib")


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
        }
        else {
            std::cout << "Created directory: " << current << std::endl;
        }
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

// Add a new Minecraft launcher profile for the modded install
void add_minecraft_launcher_profile(const std::string& minecraft_dir, const std::string& modded_install_dir, const std::string& fabric_loader_version, const std::string& mc_version, const std::string& profile_name) {
    using json = nlohmann::json;
    std::string profiles_path = minecraft_dir + "\\launcher_profiles.json";
    std::ifstream in(profiles_path);
    if (!in) {
        std::cerr << "Could not open launcher_profiles.json for reading: " << profiles_path << std::endl;
        return;
    }
    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse launcher_profiles.json: " << e.what() << std::endl;
        return;
    }
    in.close();

    std::string profile_id = "fabric-modded-" + mc_version;
    std::string last_version_id = "fabric-loader-" + fabric_loader_version + "-" + mc_version;

    // Add or update the profile
    j["profiles"][profile_id] = {
        {"name", profile_name},
        {"lastVersionId", last_version_id},
        {"gameDir", modded_install_dir},
        {"type", "custom"},
        {"icon", "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAA\
IAAAACABAMAAAAxEHz4AAAAGFBMVEUAAAA4NCrb0LTGvKW8spyAem2uppSakn5Ssn\
MLAAAAAXRSTlMAQObYZgAAAJ5JREFUaIHt1MENgCAMRmFWYAVXcAVXcAVXcH3bhCY\
NkYjcKO8dSf7v1JASUWdZAlgb0PEmDSMAYYBdGkYApgf8ER3SbwRgesAf0BACMD1g\
B6S9IbkEEBfwY49oNj4lgLhA64C0o9R9RABTAvp4SX5kB2TA5y8EEAK4pRrxB9QcA\
4QBWkj3GCAMUCO/xwBhAI/kEsCagCHDY4AwAC3VA6t4zTAMj0OJAAAAAElFTkSuQmCC"},
        {"javaArgs", "-Xmx4G -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32M"}
    };

    std::ofstream out(profiles_path);
    if (!out) {
        std::cerr << "Could not open launcher_profiles.json for writing: " << profiles_path << std::endl;
        return;
    }
    out << j.dump(4);
    out.close();
    std::cout << "Added/updated Minecraft launcher profile: " << profile_name << std::endl;
}