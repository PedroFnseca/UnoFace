#include "unoface/core/expression.hpp"

namespace unoface {
namespace core {

std::string to_string(Expression expr) {
    switch (expr) {
        case Expression::HostConnected:         return "host";
        case Expression::DisplayConnected:      return "display";
        case Expression::PdConnected:           return "pd";
        case Expression::DisplayAndPdConnected: return "display_pd";
    }
    return "unknown";
}

std::string display_name(Expression expr) {
    switch (expr) {
        case Expression::HostConnected:         return "Connect to Host";
        case Expression::DisplayConnected:      return "Connect to External Display";
        case Expression::PdConnected:           return "Connect to PD Charger";
        case Expression::DisplayAndPdConnected: return "External Display + PD Charging";
    }
    return "Unknown";
}

std::string ascii_art(Expression expr) {
    switch (expr) {
        case Expression::HostConnected:
            return "-       -\r\n\r\n         ";
        case Expression::DisplayConnected:
            return "-       -\r\n\r\n    <    ";
        case Expression::PdConnected:
            return "^       ^\r\n\r\n         ";
        case Expression::DisplayAndPdConnected:
            return "^       ^\r\n\r\n    <    ";
    }
    return "?       ?\r\n\r\n         ";
}

std::vector<Expression> all_expressions() {
    return {
        Expression::HostConnected,
        Expression::DisplayConnected,
        Expression::PdConnected,
        Expression::DisplayAndPdConnected
    };
}

} // namespace core
} // namespace unoface
