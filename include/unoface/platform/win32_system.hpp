#pragma once

#include <string>

namespace unoface {
namespace platform {

std::string local_timestamp();
std::string file_timestamp();
std::string executable_directory();
bool ensure_directory(const std::string& path);
void open_in_explorer(const std::string& path);

} // namespace platform
} // namespace unoface
