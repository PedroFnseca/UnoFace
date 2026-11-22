#include "unoface/usb/win32_hub_enumerator.hpp"
#include "unoface/usb/descriptor_parser.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace unoface {
namespace usb {

#ifdef _WIN32

namespace {

const GUID kGuidDevInterfaceUsbHub = {
    0xf18a0e88, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8}
};

const DWORD kIoctlGetNodeInformation =
    CTL_CODE(FILE_DEVICE_UNKNOWN, 258, METHOD_BUFFERED, FILE_ANY_ACCESS);
const DWORD kIoctlGetDescriptorFromNodeConnection =
    CTL_CODE(FILE_DEVICE_UNKNOWN, 260, METHOD_BUFFERED, FILE_ANY_ACCESS);
const DWORD kIoctlGetNodeConnectionDriverKeyName =
    CTL_CODE(FILE_DEVICE_UNKNOWN, 264, METHOD_BUFFERED, FILE_ANY_ACCESS);
const DWORD kIoctlGetNodeConnectionInformationEx =
    CTL_CODE(FILE_DEVICE_UNKNOWN, 274, METHOD_BUFFERED, FILE_ANY_ACCESS);

const uint16_t kLanguageEnglishUs = 0x0409;

#pragma pack(push, 1)

struct HubDescriptor {
    UCHAR  bDescriptorLength;
    UCHAR  bDescriptorType;
    UCHAR  bNumberOfPorts;
    USHORT wHubCharacteristics;
    UCHAR  bPowerOnToPowerGood;
    UCHAR  bHubControlCurrent;
    UCHAR  bRemoveAndPowerMask[64];
};

struct NodeInformation {
    ULONG NodeType;
    union {
        struct {
            HubDescriptor HubDescriptor;
            BOOLEAN       HubIsBusPowered;
        } HubInformation;
        UCHAR Reserved[76];
    } u;
};

struct DeviceDescriptor {
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    USHORT bcdUSB;
    UCHAR  bDeviceClass;
    UCHAR  bDeviceSubClass;
    UCHAR  bDeviceProtocol;
    UCHAR  bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR  iManufacturer;
    UCHAR  iProduct;
    UCHAR  iSerialNumber;
    UCHAR  bNumConfigurations;
};

struct NodeConnectionInformationEx {
    ULONG            ConnectionIndex;
    DeviceDescriptor Descriptor;
    UCHAR            CurrentConfigurationValue;
    UCHAR            Speed;
    BOOLEAN          DeviceIsHub;
    USHORT           DeviceAddress;
    ULONG            NumberOfOpenPipes;
    ULONG            ConnectionStatus;
};

struct DescriptorRequest {
    ULONG  ConnectionIndex;
    UCHAR  bmRequest;
    UCHAR  bRequest;
    USHORT wValue;
    USHORT wIndex;
    USHORT wLength;
};

struct DriverKeyNameHeader {
    ULONG ConnectionIndex;
    ULONG ActualLength;
};

#pragma pack(pop)

struct HubNode {
    std::string path;
    std::string instance_id;
    std::string driver_key;
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::vector<core::PortStatus> ports;
    std::vector<DeviceDescriptor> port_descriptors;
};

std::string to_lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool parse_vid_pid(const std::string& instance_id, uint16_t& vid, uint16_t& pid) {
    const std::string lower = to_lower(instance_id);
    const size_t v = lower.find("vid_");
    const size_t p = lower.find("pid_");
    if (v == std::string::npos || p == std::string::npos) {
        return false;
    }
    unsigned int vv = 0;
    unsigned int pp = 0;
    if (std::sscanf(lower.c_str() + v + 4, "%4x", &vv) != 1 ||
        std::sscanf(lower.c_str() + p + 4, "%4x", &pp) != 1) {
        return false;
    }
    vid = static_cast<uint16_t>(vv);
    pid = static_cast<uint16_t>(pp);
    return true;
}

core::UsbSpeed map_speed(UCHAR speed) {
    switch (speed) {
        case 0:  return core::UsbSpeed::LowSpeed;
        case 1:  return core::UsbSpeed::FullSpeed;
        case 2:  return core::UsbSpeed::HighSpeed;
        case 3:  return core::UsbSpeed::SuperSpeed;
        default: return core::UsbSpeed::Unknown;
    }
}

std::vector<HubNode> list_hub_nodes() {
    std::vector<HubNode> nodes;

    HDEVINFO dev_info = SetupDiGetClassDevsA(&kGuidDevInterfaceUsbHub, nullptr, nullptr,
                                             DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE) {
        return nodes;
    }

    SP_DEVICE_INTERFACE_DATA iface;
    iface.cbSize = sizeof(iface);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(dev_info, nullptr, &kGuidDevInterfaceUsbHub, i, &iface); ++i) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailA(dev_info, &iface, nullptr, 0, &required, nullptr);
        if (required == 0) {
            continue;
        }

        std::vector<uint8_t> buffer(required, 0);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        SP_DEVINFO_DATA dev_data;
        dev_data.cbSize = sizeof(dev_data);
        if (!SetupDiGetDeviceInterfaceDetailA(dev_info, &iface, detail, required, nullptr, &dev_data)) {
            continue;
        }

        HubNode node;
        node.path = detail->DevicePath;

        char text[512] = {0};
        if (SetupDiGetDeviceInstanceIdA(dev_info, &dev_data, text, sizeof(text), nullptr)) {
            node.instance_id = text;
        }

        std::memset(text, 0, sizeof(text));
        if (SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_data, SPDRP_DRIVER, nullptr,
                                              reinterpret_cast<PBYTE>(text), sizeof(text) - 1, nullptr)) {
            node.driver_key = to_lower(text);
        }

        nodes.push_back(node);
    }

    SetupDiDestroyDeviceInfoList(dev_info);
    return nodes;
}

void read_ports(HubNode& node) {
    NodeInformation info;
    std::memset(&info, 0, sizeof(info));
    DWORD returned = 0;

    if (!DeviceIoControl(node.handle, kIoctlGetNodeInformation,
                         &info, sizeof(info), &info, sizeof(info), &returned, nullptr)) {
        return;
    }

    const ULONG port_count = info.u.HubInformation.HubDescriptor.bNumberOfPorts;
    for (ULONG port = 1; port <= port_count; ++port) {
        std::vector<uint8_t> buffer(sizeof(NodeConnectionInformationEx) + 512, 0);
        auto* conn = reinterpret_cast<NodeConnectionInformationEx*>(buffer.data());
        conn->ConnectionIndex = port;

        core::PortStatus status;
        status.port_index = port;
        DeviceDescriptor descriptor;
        std::memset(&descriptor, 0, sizeof(descriptor));

        if (DeviceIoControl(node.handle, kIoctlGetNodeConnectionInformationEx,
                            buffer.data(), static_cast<DWORD>(buffer.size()),
                            buffer.data(), static_cast<DWORD>(buffer.size()),
                            &returned, nullptr) &&
            conn->ConnectionStatus == 1) {
            status.is_connected = true;
            status.is_hub       = conn->DeviceIsHub != 0;
            status.vendor_id    = conn->Descriptor.idVendor;
            status.product_id   = conn->Descriptor.idProduct;
            status.device_class = conn->Descriptor.bDeviceClass;
            status.speed        = map_speed(conn->Speed);
            descriptor          = conn->Descriptor;
        }

        node.ports.push_back(status);
        node.port_descriptors.push_back(descriptor);
    }
}

std::string connection_driver_key(HANDLE hub, ULONG port) {
    DriverKeyNameHeader probe;
    probe.ConnectionIndex = port;
    probe.ActualLength    = 0;
    DWORD returned = 0;

    std::vector<uint8_t> small(sizeof(DriverKeyNameHeader) + sizeof(WCHAR) * 2, 0);
    std::memcpy(small.data(), &probe, sizeof(probe));
    if (!DeviceIoControl(hub, kIoctlGetNodeConnectionDriverKeyName,
                         small.data(), static_cast<DWORD>(small.size()),
                         small.data(), static_cast<DWORD>(small.size()), &returned, nullptr)) {
        return std::string();
    }

    const ULONG needed = reinterpret_cast<DriverKeyNameHeader*>(small.data())->ActualLength;
    if (needed <= sizeof(DriverKeyNameHeader)) {
        return std::string();
    }

    std::vector<uint8_t> full(needed + sizeof(WCHAR), 0);
    std::memcpy(full.data(), &probe, sizeof(probe));
    if (!DeviceIoControl(hub, kIoctlGetNodeConnectionDriverKeyName,
                         full.data(), static_cast<DWORD>(full.size()),
                         full.data(), static_cast<DWORD>(full.size()), &returned, nullptr)) {
        return std::string();
    }

    const auto* wide = reinterpret_cast<const WCHAR*>(full.data() + sizeof(DriverKeyNameHeader));
    char narrow[512] = {0};
    WideCharToMultiByte(CP_ACP, 0, wide, -1, narrow, sizeof(narrow) - 1, nullptr, nullptr);
    return to_lower(narrow);
}

bool request_descriptor(HANDLE hub, ULONG port, uint8_t type, uint8_t index,
                        uint16_t language, uint16_t length, std::vector<uint8_t>& out) {
    std::vector<uint8_t> buffer(sizeof(DescriptorRequest) + length, 0);
    auto* request = reinterpret_cast<DescriptorRequest*>(buffer.data());
    request->ConnectionIndex = port;
    request->bmRequest       = 0x80;
    request->bRequest        = 0x06;
    request->wValue          = static_cast<USHORT>((type << 8) | index);
    request->wIndex          = language;
    request->wLength         = length;

    DWORD returned = 0;
    if (!DeviceIoControl(hub, kIoctlGetDescriptorFromNodeConnection,
                         buffer.data(), static_cast<DWORD>(buffer.size()),
                         buffer.data(), static_cast<DWORD>(buffer.size()), &returned, nullptr) ||
        returned <= sizeof(DescriptorRequest)) {
        return false;
    }

    out.assign(buffer.begin() + sizeof(DescriptorRequest), buffer.begin() + returned);
    return true;
}

bool request_full_descriptor(HANDLE hub, ULONG port, uint8_t type, uint16_t header_length,
                             std::vector<uint8_t>& out) {
    std::vector<uint8_t> header;
    if (!request_descriptor(hub, port, type, 0, 0, header_length, header)) {
        return false;
    }
    const uint16_t total = read_total_length(header);
    if (total <= header.size()) {
        out = header;
        return true;
    }
    return request_descriptor(hub, port, type, 0, 0, total, out);
}

std::string request_string(HANDLE hub, ULONG port, uint8_t index) {
    if (index == 0) {
        return std::string();
    }
    std::vector<uint8_t> raw;
    if (!request_descriptor(hub, port, DescriptorType::kString, index, kLanguageEnglishUs, 255, raw)) {
        return std::string();
    }
    return parse_string_descriptor(raw);
}

core::DescriptorInfo read_descriptors(HANDLE hub, ULONG port, const DeviceDescriptor& device) {
    core::DescriptorInfo info;
    info.available    = true;
    info.bcd_usb      = device.bcdUSB;
    info.bcd_device   = device.bcdDevice;
    info.manufacturer = request_string(hub, port, device.iManufacturer);
    info.product      = request_string(hub, port, device.iProduct);
    info.serial       = request_string(hub, port, device.iSerialNumber);

    std::vector<uint8_t> configuration;
    if (request_full_descriptor(hub, port, DescriptorType::kConfiguration, 9, configuration)) {
        info.interfaces = parse_interfaces(configuration);
    }

    if (device.bcdUSB >= 0x0201) {
        std::vector<uint8_t> bos;
        if (request_full_descriptor(hub, port, DescriptorType::kBos, 5, bos)) {
            info.bos_capabilities = parse_bos_capabilities(bos);
        }
    }
    return info;
}

} // namespace

std::vector<core::HubDeviceInfo> Win32HubEnumerator::enumerate(bool include_descriptors) {
    std::vector<HubNode> nodes = list_hub_nodes();

    for (auto& node : nodes) {
        node.handle = CreateFileA(node.path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, 0, nullptr);
        if (node.handle != INVALID_HANDLE_VALUE) {
            read_ports(node);
        }
    }

    std::vector<core::HubDeviceInfo> hubs;
    std::vector<std::string> hub_driver_keys;

    for (const auto& node : nodes) {
        uint16_t vid = 0;
        uint16_t pid = 0;
        if (!parse_vid_pid(node.instance_id, vid, pid) || !core::is_uno_hub(vid, pid)) {
            continue;
        }
        core::HubDeviceInfo hub;
        hub.vendor_id   = vid;
        hub.product_id  = pid;
        hub.instance_id = node.instance_id;
        hub.ports       = node.ports;
        hubs.push_back(hub);
        hub_driver_keys.push_back(node.driver_key);
    }

    if (include_descriptors) {
        for (const auto& parent : nodes) {
            if (parent.handle == INVALID_HANDLE_VALUE) {
                continue;
            }
            for (size_t i = 0; i < parent.ports.size(); ++i) {
                const core::PortStatus& port = parent.ports[i];
                if (!port.is_connected || !core::is_uno_hub(port.vendor_id, port.product_id)) {
                    continue;
                }
                const std::string key = connection_driver_key(parent.handle, port.port_index);
                for (size_t h = 0; h < hubs.size(); ++h) {
                    if (!key.empty() && key == hub_driver_keys[h]) {
                        hubs[h].descriptors = read_descriptors(parent.handle, port.port_index,
                                                               parent.port_descriptors[i]);
                    }
                }
            }
        }
    }

    for (auto& node : nodes) {
        if (node.handle != INVALID_HANDLE_VALUE) {
            CloseHandle(node.handle);
        }
    }

    return hubs;
}

#else

std::vector<core::HubDeviceInfo> Win32HubEnumerator::enumerate(bool) {
    return {};
}

#endif

} // namespace usb
} // namespace unoface
