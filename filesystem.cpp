#define NOMINMAX

#include "constants.hpp"
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


// Helper to execute a command and get its output
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        return ""; // or throw an exception
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
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
        {"javaArgs", "-Xmx4G -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32M"},
        {"javaDir", get_javaw_path()}
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

// Get the installed Java version string
std::string get_java_version() {
    std::string output = exec("java -version 2>&1");
    if (output.empty()) {
        return "";
    }

    size_t first_line_end = output.find('\n');
    if (first_line_end == std::string::npos) {
        first_line_end = output.length();
    }
    std::string first_line = output.substr(0, first_line_end);

    size_t first_quote = first_line.find('\"');
    if (first_quote != std::string::npos) {
        size_t second_quote = first_line.find('\"', first_quote + 1);
        if (second_quote != std::string::npos) {
            return first_line.substr(first_quote + 1, second_quote - first_quote - 1);
        }
    }
    return ""; // Return empty if version not found
}

// Get the full path to javaw.exe from a Java installation that meets the required version
std::string get_javaw_path() {
    // First try to find javaw.exe in PATH
    char buffer[MAX_PATH];
    DWORD result = SearchPathA(NULL, "javaw.exe", NULL, MAX_PATH, buffer, NULL);
    if (result > 0 && result < MAX_PATH) {
        std::string javaw_path = buffer;

        // Verify the version by running javaw -version (redirect to java -version)
        std::string version_cmd = "\"" + javaw_path + "\" -version 2>&1";
        std::string output = exec(version_cmd.c_str());
        if (!output.empty()) {
            // Parse version from output similar to get_java_version
            size_t first_line_end = output.find('\n');
            if (first_line_end == std::string::npos) {
                first_line_end = output.length();
            }
            std::string first_line = output.substr(0, first_line_end);

            size_t first_quote = first_line.find('\"');
            if (first_quote != std::string::npos) {
                size_t second_quote = first_line.find('\"', first_quote + 1);
                if (second_quote != std::string::npos) {
                    std::string version = first_line.substr(first_quote + 1, second_quote - first_quote - 1);
                    if (is_version_greater_or_equal(version, REQUIRED_JAVA_VERSION)) {
                        std::cout << "Found suitable javaw.exe in PATH: " << javaw_path << " (version " << version << ")" << std::endl;
                        return javaw_path;
                    }
                }
            }
        }
    }

    // If not found in PATH or version is insufficient, search common installation directories
    std::vector<std::string> search_paths = {
        "C:\\Program Files\\Java",
        "C:\\Program Files\\Oracle",
        "C:\\Program Files\\Eclipse Adoptium",
        "C:\\Program Files\\Eclipse Foundation",
        "C:\\Program Files (x86)\\Java",
        "C:\\Program Files (x86)\\Oracle",
        "C:\\Program Files (x86)\\Eclipse Adoptium",
        "C:\\Program Files (x86)\\Eclipse Foundation"
    };

    for (const std::string& base_path : search_paths) {
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind = FindFirstFileA((base_path + "\\*").c_str(), &findFileData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::string dir_name = findFileData.cFileName;
                    if (dir_name != "." && dir_name != "..") {
                        std::string java_dir = base_path + "\\" + dir_name;
                        std::string javaw_path = java_dir + "\\bin\\javaw.exe";

                        // Check if javaw.exe exists
                        std::ifstream file(javaw_path);
                        if (file.good()) {
                            file.close();

                            // Test version
                            std::string version_cmd = "\"" + javaw_path + "\" -version 2>&1";
                            std::string output = exec(version_cmd.c_str());
                            if (!output.empty()) {
                                // Parse version
                                size_t first_line_end = output.find('\n');
                                if (first_line_end == std::string::npos) {
                                    first_line_end = output.length();
                                }
                                std::string first_line = output.substr(0, first_line_end);

                                size_t first_quote = first_line.find('\"');
                                if (first_quote != std::string::npos) {
                                    size_t second_quote = first_line.find('\"', first_quote + 1);
                                    if (second_quote != std::string::npos) {
                                        std::string version = first_line.substr(first_quote + 1, second_quote - first_quote - 1);
                                        if (is_version_greater_or_equal(version, REQUIRED_JAVA_VERSION)) {
                                            std::cout << "Found suitable javaw.exe: " << javaw_path << " (version " << version << ")" << std::endl;
                                            FindClose(hFind);
                                            return javaw_path;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } while (FindNextFileA(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
    }

    std::cout << "Could not find javaw.exe with version " << REQUIRED_JAVA_VERSION << " or newer." << std::endl;
    return ""; // Return empty string if not found
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