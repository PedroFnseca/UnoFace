#include "unoface/core/host_signals.hpp"

namespace unoface {
namespace core {

Expression infer_face_from_cables(const HubAttachedCables& cables) {
    if (cables.hdmi_connected && cables.pd_connected) {
        return Expression::DisplayAndPdConnected;
    }
    if (cables.hdmi_connected) {
        return Expression::DisplayConnected;
    }
    if (cables.pd_connected) {
        return Expression::PdConnected;
    }
    return Expression::HostConnected;
}

} // namespace core
} // namespace unoface
