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
#include <fstream>
#include <memory>


// function declarations (to avoid linker errors)

// core functions
bool is_java_installed();
void validate_java_installation();
bool is_fabric_installed(const std::string& minecraft_dir, const std::string& mcversion, const std::string& loader_version);
void validate_fabric_installation(const std::string& mcversion, const std::string& loader_version);
bool is_version_greater_or_equal(const std::string& installed_version, const std::string& required_version);
bool is_modpack_downloaded(const std::string& modpack_zip_path);
void validate_modpack_installation(const std::string& modpack_url);
void refresh_environment_variables();
bool check_java_in_common_locations();
std::string get_java_path();
bool add_java_to_path(const std::string& java_bin_dir);

// function definitions

// Refresh environment variables (broadcasts WM_SETTINGCHANGE)
void refresh_environment_variables() {
    std::cout << "Refreshing environment variables..." << std::endl;
    
    // Broadcast WM_SETTINGCHANGE to notify all windows of environment changes
    DWORD_PTR result = 0;
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 
                      (LPARAM)"Environment", SMTO_ABORTIFHUNG, 
                      5000, &result);
    
    // Small delay to allow system to process the change
    Sleep(2000);
}

// Check for Java in common installation locations without relying on PATH
bool check_java_in_common_locations() {
    std::cout << "Checking Java in common installation locations..." << std::endl;
    
    std::vector<std::string> search_paths = {
        "C:\\Program Files\\Java",
        "C:\\Program Files\\Oracle",
        "C:\\Program Files (x86)\\Java",
        "C:\\Program Files (x86)\\Oracle"
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
                        std::string java_exe = java_dir + "\\bin\\java.exe";
                        
                        // Check if java.exe exists
                        std::ifstream file(java_exe);
                        if (file.good()) {
                            file.close();
                            std::cout << "Found Java installation at: " << java_dir << std::endl;
                            
                            // Test version using full path
                            std::string version_cmd = "\"" + java_exe + "\" -version 2>&1";
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
                                        std::cout << "Found Java version: " << version << " at " << java_exe << std::endl;
                                        if (is_version_greater_or_equal(version, REQUIRED_JAVA_VERSION)) {
                                            FindClose(hFind);
                                            return true;
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
    
    return false;
}

// Check if Java is installed by running 'java -version'
bool is_java_installed() {
    std::string installed_version = get_java_version();
    if (installed_version.empty()) {
        std::cout << "Java is NOT installed." << std::endl;
        return false;
    }

    if (is_version_greater_or_equal(installed_version, REQUIRED_JAVA_VERSION)) {
        std::cout << "Java version " << installed_version << " is installed and meets requirements." << std::endl;
        return true;
    } else {
        std::cout << "Java version " << installed_version << " is installed, but version " << REQUIRED_JAVA_VERSION << " or greater is required." << std::endl;
        return false;
    }
}

void validate_java_installation() {
    if (is_java_installed()) {
        std::cout << "Java is properly installed." << std::endl;
        return;
    }

    std::cout << "Java is not installed. Attempting to download and install Oracle JDK..." << std::endl;
    std::string temp_dir = safe_getenv("TEMP");
    std::string java_installer_path = temp_dir + "\\jdk-22.0.2_windows-x64_bin.msi";

    download_file(JAVA_INSTALLER_URL, java_installer_path);

    std::cout << "Running Java installer..." << std::endl;
    std::string install_cmd = "msiexec /i \"" + java_installer_path + "\" /qn /norestart";
    std::cout << "Java install command: " << install_cmd << std::endl;
    int result = system(install_cmd.c_str());
    if (result != 0) {
        std::cerr << "Java install command failed. Please install Java manually from https://www.oracle.com/java/technologies/javase/jdk22-archive-downloads.html" << std::endl;
        exit(1);
    }

    std::cout << "Java installer completed. Verifying installation..." << std::endl;
    
    // Refresh environment variables to pick up PATH changes
    refresh_environment_variables();
    
    // Try multiple checks with delays to allow for installation completion
    for (int attempt = 1; attempt <= 3; attempt++) {
        std::cout << "Verification attempt " << attempt << "/3..." << std::endl;
        
        // First try the standard PATH-based check
        if (is_java_installed()) {
            std::cout << "Java installed successfully and found in PATH." << std::endl;
            return;
        }
        
        // If PATH check fails, try checking common installation locations
        if (check_java_in_common_locations()) {
            std::cout << "Java installed successfully (found in installation directory)." << std::endl;
            std::cout << "Note: You may need to restart your command prompt for PATH changes to take effect." << std::endl;
            
            // Try to get Java path and add it to current process PATH
            std::string java_path = get_java_path();
            if (!java_path.empty()) {
                std::string java_bin_dir = java_path.substr(0, java_path.find_last_of("\\"));
                if (add_java_to_path(java_bin_dir)) {
                    std::cout << "Added Java to PATH for current process." << std::endl;
                }
            }
            return;
        }
        
        if (attempt < 3) {
            std::cout << "Java not detected yet, waiting 3 seconds before retry..." << std::endl;
            Sleep(3000); // Wait 3 seconds before next attempt
        }
    }
    
    // If all attempts fail, provide more detailed error message
    std::cerr << "Java installation verification failed after 3 attempts." << std::endl;
    std::cerr << "The installer may have completed, but Java is not accessible via PATH or common locations." << std::endl;
    std::cerr << "This can happen if:" << std::endl;
    std::cerr << "1. The installer requires a system restart" << std::endl;
    std::cerr << "2. The installer failed silently" << std::endl;
    std::cerr << "3. Java was installed but PATH was not updated properly" << std::endl;
    std::cerr << "Please try:" << std::endl;
    std::cerr << "- Restart this program as Administrator" << std::endl;
    std::cerr << "- Restart your computer and try again" << std::endl;
    std::cerr << "- Install Java manually from https://www.oracle.com/java/technologies/javase/jdk22-archive-downloads.html" << std::endl;
    exit(1);
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

    // Get the specific Java executable path
    std::string java_path = get_java_path();
    if (java_path.empty()) {
        std::cerr << "Could not find Java executable to run Fabric installer." << std::endl;
        std::cerr << "Please ensure Java is properly installed and try again." << std::endl;
        exit(1);
    }

    // Extract the bin directory from the java path and add it to PATH
    std::string java_bin_dir = java_path.substr(0, java_path.find_last_of("\\"));
    if (!add_java_to_path(java_bin_dir)) {
        std::cerr << "Warning: Could not add Java bin directory to PATH. Trying with full path anyway..." << std::endl;
    }

    // Run the Fabric installer - try with simple 'java' command first since we added to PATH
    std::cout << "Running Fabric installer..." << std::endl;
    
    // First attempt: Use simple 'java' command (should work now that we added to PATH)
    std::string install_cmd = "java -jar \"" + fabric_installer_path + "\" client -dir \"" + minecraft_dir + "\" -mcversion " + mcversion + " -loader " + loader_version;
    std::cout << "Fabric install command: " << install_cmd << std::endl;
    int result = system(install_cmd.c_str());
    
    // If that fails, try with full path as fallback
    if (result != 0) {
        std::cout << "Simple 'java' command failed, trying with full path..." << std::endl;
        
        // Use 8.3 short path names to avoid spaces (Windows API)
        char short_java_path[MAX_PATH];
        if (GetShortPathNameA(java_path.c_str(), short_java_path, MAX_PATH) > 0) {
            std::string short_java_path_str(short_java_path);
            install_cmd = short_java_path_str + " -jar \"" + fabric_installer_path + "\" client -dir \"" + minecraft_dir + "\" -mcversion " + mcversion + " -loader " + loader_version;
            std::cout << "Fabric install command (short path): " << install_cmd << std::endl;
            result = system(install_cmd.c_str());
        } else {
            std::cerr << "Could not get short path name for Java executable." << std::endl;
        }
    }
    
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


bool is_modpack_downloaded(const std::string& modpack_zip_path) {
    std::ifstream file(modpack_zip_path, std::ios::binary);
    if (file) {
        file.close();
        std::cout << "Modpack already downloaded at: " << modpack_zip_path << std::endl;
        return true;
    } else {
        std::cout << "Modpack not found at: " << modpack_zip_path << std::endl;
        return false;
	}
}


void validate_modpack_installation(const std::string& modpack_url) {
    std::cout << "Downloading and installing modpack..." << std::endl;

	std::string temp_dir = safe_getenv("TEMP");
	std::string modpack_zip_path = temp_dir + "\\cove-s8-modpack.zip";
	
    // Check if the modpack is already downloaded
    if (!is_modpack_downloaded(modpack_zip_path)) {
        std::cout << "Modpack not downloaded. Downloading..." << std::endl;
        download_file(modpack_url, modpack_zip_path);
	}
    else {
		std::cout << "Modpack already downloaded. Skipping download." << std::endl;
    }
	// Unzip the modpack into the modded install directory
	std::string modded_install_dir = safe_getenv("USERPROFILE") + "\\Games\\Minecraft\\modded-install\\mods";
    std::string unzip_cmd = "powershell -Command \"Expand-Archive -Path '" + modpack_zip_path + "' -DestinationPath '" + modded_install_dir + "' -Force\"";
    std::cout << "Unzip command: " << unzip_cmd << std::endl;
    int result = system(unzip_cmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to unzip the modpack. Please unzip manually." << std::endl;
        exit(1);
    }
	std::cout << "Modpack unzipped successfully into: " << modded_install_dir << std::endl;	
}

// Add Java's bin directory to the current process PATH
bool add_java_to_path(const std::string& java_bin_dir) {
    std::cout << "Adding Java bin directory to PATH: " << java_bin_dir << std::endl;
    
    // Get current PATH environment variable
    char* current_path = nullptr;
    size_t path_size = 0;
    if (_dupenv_s(&current_path, &path_size, "PATH") != 0) {
        std::cerr << "Failed to get current PATH environment variable." << std::endl;
        return false;
    }
    
    if (current_path == nullptr) {
        std::cerr << "PATH environment variable is not set." << std::endl;
        return false;
    }
    
    // Check if Java bin directory is already in PATH
    std::string current_path_str(current_path);
    if (current_path_str.find(java_bin_dir) != std::string::npos) {
        std::cout << "Java bin directory is already in PATH." << std::endl;
        free(current_path);
        return true;
    }
    
    // Prepend Java bin directory to PATH (prepend to ensure it takes priority)
    std::string new_path = java_bin_dir + ";" + current_path_str;
    
    // Set the new PATH for current process
    if (SetEnvironmentVariableA("PATH", new_path.c_str()) == 0) {
        std::cerr << "Failed to update PATH environment variable. Error: " << GetLastError() << std::endl;
        free(current_path);
        return false;
    }
    
    std::cout << "Successfully added Java bin directory to PATH." << std::endl;
    free(current_path);
    return true;
}

// Get the full path to java.exe from a Java installation that meets the required version
std::string get_java_path() {
    // First try to find java.exe in PATH
    char buffer[MAX_PATH];
    DWORD result = SearchPathA(NULL, "java.exe", NULL, MAX_PATH, buffer, NULL);
    if (result > 0 && result < MAX_PATH) {
        std::string java_path = buffer;
        
        // Verify the version by running java -version
        std::string version_cmd = "\"" + java_path + "\" -version 2>&1";
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
                        std::cout << "Found suitable java.exe in PATH: " << java_path << " (version " << version << ")" << std::endl;
                        return java_path;
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
                        std::string java_path = java_dir + "\\bin\\java.exe";
                        
                        // Check if java.exe exists
                        std::ifstream file(java_path);
                        if (file.good()) {
                            file.close();
                            
                            // Test version
                            std::string version_cmd = "\"" + java_path + "\" -version 2>&1";
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
                                            std::cout << "Found suitable java.exe: " << java_path << " (version " << version << ")" << std::endl;
                                            FindClose(hFind);
                                            return java_path;
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
    
    std::cout << "Could not find java.exe with version " << REQUIRED_JAVA_VERSION << " or newer." << std::endl;
    return ""; // Return empty string if not found
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

    // Add launcher profile for the modded install
    add_minecraft_launcher_profile(minecraft_dir, modded_install_dir, FABRIC_LOADER_VERSION, MINECRAFT_VERSION, "The Cove - Season 8 (" + MINECRAFT_VERSION + ")");

    // Wait for user to launch modded Minecraft install and close it (can skip this step, mods folder can be there before install initialization)
    // std::cout << "\n\nNow, launch your modded Minecraft install and close it!" << std::endl;
    // std::cout << "\nThen press enter." << std::endl;
    // std::cin.get();
    
    // download and unzip the modpack into the modded install
    validate_modpack_installation(MODPACK_URL);


    std::cout << "Setup script completed." << std::endl;
    return 0;
}
