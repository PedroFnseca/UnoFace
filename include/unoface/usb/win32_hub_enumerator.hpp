#pragma once

#include "unoface/usb/ihub_enumerator.hpp"

namespace unoface {
namespace usb {

class Win32HubEnumerator final : public IHubEnumerator {
public:
    std::vector<core::HubDeviceInfo> enumerate(bool include_descriptors) override;
};

} // namespace usb
} // namespace unoface
