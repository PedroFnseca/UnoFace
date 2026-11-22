#pragma once

#include "unoface/core/expression.hpp"

namespace unoface {
namespace core {

struct HubAttachedCables {
    bool hdmi_connected = false;
    bool pd_connected   = false;
};

Expression infer_face_from_cables(const HubAttachedCables& cables);

} // namespace core
} // namespace unoface
