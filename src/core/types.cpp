#include "unoface/core/types.hpp"

namespace unoface {
namespace core {

constexpr uint16_t KnownHardwareIds::kGenesysVendorId;
constexpr uint16_t KnownHardwareIds::kUnoUsb2HubProductId;
constexpr uint16_t KnownHardwareIds::kUnoUsb3HubProductId;

bool is_uno_hub(uint16_t vendor_id, uint16_t product_id) {
    return vendor_id == KnownHardwareIds::kGenesysVendorId &&
           (product_id == KnownHardwareIds::kUnoUsb2HubProductId ||
            product_id == KnownHardwareIds::kUnoUsb3HubProductId);
}

std::string to_string(UsbSpeed speed) {
    switch (speed) {
        case UsbSpeed::LowSpeed:   return "Low-Speed";
        case UsbSpeed::FullSpeed:  return "Full-Speed";
        case UsbSpeed::HighSpeed:  return "High-Speed";
        case UsbSpeed::SuperSpeed: return "SuperSpeed";
        case UsbSpeed::Unknown:    break;
    }
    return "Unknown";
}

} // namespace core
} // namespace unoface
