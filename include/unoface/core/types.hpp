#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace unoface {
namespace core {

struct KnownHardwareIds {
    static constexpr uint16_t kGenesysVendorId     = 0x05E3;
    static constexpr uint16_t kUnoUsb2HubProductId = 0x0610;
    static constexpr uint16_t kUnoUsb3HubProductId = 0x0625;
};

bool is_uno_hub(uint16_t vendor_id, uint16_t product_id);

enum class UsbSpeed : uint8_t {
    Unknown,
    LowSpeed,
    FullSpeed,
    HighSpeed,
    SuperSpeed
};

std::string to_string(UsbSpeed speed);

struct InterfaceInfo {
    uint8_t number    = 0;
    uint8_t klass     = 0;
    uint8_t sub_class = 0;
    uint8_t protocol  = 0;
};

struct PortStatus {
    uint32_t port_index = 0;
    bool is_connected   = false;
    bool is_hub         = false;
    uint16_t vendor_id  = 0;
    uint16_t product_id = 0;
    uint8_t device_class = 0;
    UsbSpeed speed      = UsbSpeed::Unknown;
};

struct DescriptorInfo {
    bool available       = false;
    uint16_t bcd_usb     = 0;
    uint16_t bcd_device  = 0;
    std::string manufacturer;
    std::string product;
    std::string serial;
    std::vector<InterfaceInfo> interfaces;
    std::vector<uint8_t> bos_capabilities;
};

struct HubDeviceInfo {
    uint16_t vendor_id  = 0;
    uint16_t product_id = 0;
    std::string instance_id;
    std::vector<PortStatus> ports;
    DescriptorInfo descriptors;
};

} // namespace core
} // namespace unoface
