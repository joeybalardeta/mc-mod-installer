#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>


std::vector<std::string> split_path(const std::string& path, char delimiter);
std::string exec(const char* cmd);
void create_directory(const std::string& path);
std::string safe_getenv(const char* var);
void download_file(const std::string& url, const std::string& output_path);
void add_minecraft_launcher_profile(const std::string& minecraft_dir, const std::string& modded_install_dir, const std::string& fabric_loader_version, const std::string& mc_version, const std::string& profile_name);
std::string get_java_version();
std::string get_javaw_path();
bool is_version_greater_or_equal(const std::string& installed_version, const std::string& required_version);

#endif