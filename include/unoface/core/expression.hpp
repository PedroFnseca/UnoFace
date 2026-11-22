#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace unoface {
namespace core {

enum class Expression : uint8_t {
    HostConnected,
    DisplayConnected,
    PdConnected,
    DisplayAndPdConnected
};

std::string to_string(Expression expr);
std::string display_name(Expression expr);
std::string ascii_art(Expression expr);
std::vector<Expression> all_expressions();

} // namespace core
} // namespace unoface
