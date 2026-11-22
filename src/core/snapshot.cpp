#include "unoface/core/snapshot.hpp"

#include <cstdio>

namespace unoface {
namespace core {

namespace {

std::string hex4(uint16_t v) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%04X", static_cast<unsigned int>(v));
    return buf;
}

std::string hex2(uint8_t v) {
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned int>(v));
    return buf;
}

std::string hub_id(const HubDeviceInfo& hub) {
    return hex4(hub.vendor_id) + ":" + hex4(hub.product_id);
}

std::string port_summary(const PortStatus& port) {
    if (!port.is_connected) {
        return "empty";
    }
    std::string text = hex4(port.vendor_id) + ":" + hex4(port.product_id) +
                       " class=0x" + hex2(port.device_class) +
                       " " + to_string(port.speed);
    if (port.is_hub) {
        text += " (hub)";
    }
    return text;
}

std::string join_bytes(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return "none";
    }
    std::string text;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) text += ",";
        text += "0x" + hex2(bytes[i]);
    }
    return text;
}

std::string interfaces_summary(const std::vector<InterfaceInfo>& interfaces) {
    if (interfaces.empty()) {
        return "none";
    }
    std::string text;
    for (size_t i = 0; i < interfaces.size(); ++i) {
        if (i > 0) text += " ";
        text += "#" + std::to_string(interfaces[i].number) +
                "[class=0x" + hex2(interfaces[i].klass) +
                " sub=0x" + hex2(interfaces[i].sub_class) +
                " proto=0x" + hex2(interfaces[i].protocol) + "]";
    }
    return text;
}

std::string cables_summary(const HubAttachedCables& c) {
    return "hub_hdmi=" + std::string(c.hdmi_connected ? "yes" : "no") +
           " hub_pd=" + std::string(c.pd_connected ? "yes" : "no") +
           " face=" + to_string(infer_face_from_cables(c));
}

const HubDeviceInfo* find_hub(const std::vector<HubDeviceInfo>& hubs, const HubDeviceInfo& key) {
    for (const auto& hub : hubs) {
        if (hub.vendor_id == key.vendor_id && hub.product_id == key.product_id) {
            return &hub;
        }
    }
    return nullptr;
}

void diff_hub(const HubDeviceInfo& before, const HubDeviceInfo& after, std::vector<std::string>& out) {
    const std::string id = hub_id(after);

    const size_t port_count = before.ports.size() > after.ports.size() ? before.ports.size() : after.ports.size();
    for (size_t i = 0; i < port_count; ++i) {
        const std::string old_text = i < before.ports.size() ? port_summary(before.ports[i]) : "missing";
        const std::string new_text = i < after.ports.size() ? port_summary(after.ports[i]) : "missing";
        if (old_text != new_text) {
            out.push_back("hub " + id + " port " + std::to_string(i + 1) + ": " + old_text + " -> " + new_text);
        }
    }

    const DescriptorInfo& a = before.descriptors;
    const DescriptorInfo& b = after.descriptors;
    if (a.bcd_device != b.bcd_device) {
        out.push_back("hub " + id + " bcdDevice: " + hex4(a.bcd_device) + " -> " + hex4(b.bcd_device));
    }
    const std::string old_ifaces = interfaces_summary(a.interfaces);
    const std::string new_ifaces = interfaces_summary(b.interfaces);
    if (old_ifaces != new_ifaces) {
        out.push_back("hub " + id + " interfaces: " + old_ifaces + " -> " + new_ifaces);
    }
    if (a.bos_capabilities != b.bos_capabilities) {
        out.push_back("hub " + id + " BOS capabilities: " + join_bytes(a.bos_capabilities) +
                      " -> " + join_bytes(b.bos_capabilities));
    }
}

} // namespace

std::vector<std::string> describe_snapshot(const Snapshot& snapshot) {
    std::vector<std::string> lines;
    lines.push_back("hub cable status: " + cables_summary(snapshot.cables));

    if (snapshot.hubs.empty()) {
        lines.push_back("no UGREEN Uno hub controllers found");
    }

    for (const auto& hub : snapshot.hubs) {
        const DescriptorInfo& d = hub.descriptors;
        lines.push_back("hub " + hub_id(hub) + " instance=" + hub.instance_id);
        if (d.available) {
            lines.push_back("  bcdUSB=" + hex4(d.bcd_usb) + " bcdDevice=" + hex4(d.bcd_device) +
                            " manufacturer='" + d.manufacturer + "' product='" + d.product +
                            "' serial='" + d.serial + "'");
            lines.push_back("  interfaces: " + interfaces_summary(d.interfaces));
            lines.push_back("  BOS capabilities: " + join_bytes(d.bos_capabilities));
        } else {
            lines.push_back("  descriptors: unavailable");
        }
        for (const auto& port : hub.ports) {
            lines.push_back("  port " + std::to_string(port.port_index) + ": " + port_summary(port));
        }
    }
    return lines;
}

std::vector<std::string> diff_snapshots(const Snapshot& before, const Snapshot& after) {
    std::vector<std::string> out;

    if (before.cables.hdmi_connected != after.cables.hdmi_connected) {
        out.push_back(std::string("hub HDMI cable: ") + (before.cables.hdmi_connected ? "connected" : "disconnected") +
                      " -> " + (after.cables.hdmi_connected ? "connected" : "disconnected"));
    }
    if (before.cables.pd_connected != after.cables.pd_connected) {
        out.push_back(std::string("hub PD charger: ") + (before.cables.pd_connected ? "connected" : "disconnected") +
                      " -> " + (after.cables.pd_connected ? "connected" : "disconnected"));
    }
    const Expression old_face = infer_face_from_cables(before.cables);
    const Expression new_face = infer_face_from_cables(after.cables);
    if (old_face != new_face) {
        out.push_back("face on hub: " + to_string(old_face) + " -> " + to_string(new_face));
    }

    for (const auto& hub : before.hubs) {
        if (!find_hub(after.hubs, hub)) {
            out.push_back("hub " + hub_id(hub) + " disappeared");
        }
    }
    for (const auto& hub : after.hubs) {
        const HubDeviceInfo* previous = find_hub(before.hubs, hub);
        if (!previous) {
            out.push_back("hub " + hub_id(hub) + " appeared");
            continue;
        }
        diff_hub(*previous, hub, out);
    }

    return out;
}

} // namespace core
} // namespace unoface
