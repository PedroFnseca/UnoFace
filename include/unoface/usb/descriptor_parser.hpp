#pragma once

#include "unoface/core/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace unoface {
namespace usb {

struct DescriptorType {
    static constexpr uint8_t kDevice        = 0x01;
    static constexpr uint8_t kConfiguration = 0x02;
    static constexpr uint8_t kString        = 0x03;
    static constexpr uint8_t kInterface     = 0x04;
    static constexpr uint8_t kBos           = 0x0F;
    static constexpr uint8_t kDeviceCapability = 0x10;
};

struct CapabilityType {
    static constexpr uint8_t kUsb20Extension = 0x02;
    static constexpr uint8_t kSuperSpeedUsb  = 0x03;
    static constexpr uint8_t kContainerId    = 0x04;
    static constexpr uint8_t kPlatform       = 0x05;
    static constexpr uint8_t kBillboard      = 0x0D;
};

uint16_t read_total_length(const std::vector<uint8_t>& header);
std::vector<core::InterfaceInfo> parse_interfaces(const std::vector<uint8_t>& configuration);
std::vector<uint8_t> parse_bos_capabilities(const std::vector<uint8_t>& bos);
std::string parse_string_descriptor(const std::vector<uint8_t>& descriptor);
std::string capability_name(uint8_t capability_type);

} // namespace usb
} // namespace unoface
