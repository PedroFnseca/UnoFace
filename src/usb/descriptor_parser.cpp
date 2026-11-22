#include "unoface/usb/descriptor_parser.hpp"

namespace unoface {
namespace usb {

constexpr uint8_t DescriptorType::kDevice;
constexpr uint8_t DescriptorType::kConfiguration;
constexpr uint8_t DescriptorType::kString;
constexpr uint8_t DescriptorType::kInterface;
constexpr uint8_t DescriptorType::kBos;
constexpr uint8_t DescriptorType::kDeviceCapability;
constexpr uint8_t CapabilityType::kUsb20Extension;
constexpr uint8_t CapabilityType::kSuperSpeedUsb;
constexpr uint8_t CapabilityType::kContainerId;
constexpr uint8_t CapabilityType::kPlatform;
constexpr uint8_t CapabilityType::kBillboard;

uint16_t read_total_length(const std::vector<uint8_t>& header) {
    if (header.size() < 4) {
        return 0;
    }
    return static_cast<uint16_t>(header[2] | (header[3] << 8));
}

std::vector<core::InterfaceInfo> parse_interfaces(const std::vector<uint8_t>& configuration) {
    std::vector<core::InterfaceInfo> result;
    size_t offset = 0;

    while (offset + 2 <= configuration.size()) {
        const uint8_t length = configuration[offset];
        const uint8_t type   = configuration[offset + 1];
        if (length < 2 || offset + length > configuration.size()) {
            break;
        }
        if (type == DescriptorType::kInterface && length >= 9) {
            core::InterfaceInfo info;
            info.number    = configuration[offset + 2];
            info.klass     = configuration[offset + 5];
            info.sub_class = configuration[offset + 6];
            info.protocol  = configuration[offset + 7];
            result.push_back(info);
        }
        offset += length;
    }
    return result;
}

std::vector<uint8_t> parse_bos_capabilities(const std::vector<uint8_t>& bos) {
    std::vector<uint8_t> result;
    if (bos.size() < 5 || bos[1] != DescriptorType::kBos) {
        return result;
    }

    size_t offset = bos[0];
    while (offset + 3 <= bos.size()) {
        const uint8_t length = bos[offset];
        const uint8_t type   = bos[offset + 1];
        if (length < 3 || offset + length > bos.size()) {
            break;
        }
        if (type == DescriptorType::kDeviceCapability) {
            result.push_back(bos[offset + 2]);
        }
        offset += length;
    }
    return result;
}

std::string parse_string_descriptor(const std::vector<uint8_t>& descriptor) {
    std::string text;
    if (descriptor.size() < 2 || descriptor[1] != DescriptorType::kString) {
        return text;
    }

    const size_t end = descriptor[0] < descriptor.size() ? descriptor[0] : descriptor.size();
    for (size_t i = 2; i + 1 < end; i += 2) {
        const uint16_t code = static_cast<uint16_t>(descriptor[i] | (descriptor[i + 1] << 8));
        text.push_back(code >= 0x20 && code < 0x7F ? static_cast<char>(code) : '?');
    }
    return text;
}

std::string capability_name(uint8_t capability_type) {
    switch (capability_type) {
        case CapabilityType::kUsb20Extension: return "USB 2.0 Extension";
        case CapabilityType::kSuperSpeedUsb:  return "SuperSpeed USB";
        case CapabilityType::kContainerId:    return "Container ID";
        case CapabilityType::kPlatform:       return "Platform";
        case CapabilityType::kBillboard:      return "Billboard";
        default:                              return "Other";
    }
}

} // namespace usb
} // namespace unoface
