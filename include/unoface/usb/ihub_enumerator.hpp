#pragma once

#include "unoface/core/types.hpp"

#include <string>
#include <vector>

namespace unoface {
namespace usb {

class IHubEnumerator {
public:
    virtual ~IHubEnumerator() = default;
    virtual std::vector<core::HubDeviceInfo> enumerate(bool include_descriptors) = 0;
};

} // namespace usb
} // namespace unoface
