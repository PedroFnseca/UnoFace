#include "unoface/core/expression.hpp"
#include "unoface/core/host_signals.hpp"
#include "unoface/core/snapshot.hpp"
#include "unoface/core/types.hpp"
#include "unoface/investigation/capture_recorder.hpp"
#include "unoface/usb/descriptor_parser.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        ++g_failures;
    } else {
        std::cout << "[PASS] " << message << "\n";
    }
}

void test_expression_and_signals() {
    using namespace unoface::core;

    expect(all_expressions().size() == 4, "Four official expressions registered");

    HubAttachedCables c1{false, false};
    expect(infer_face_from_cables(c1) == Expression::HostConnected, "Nothing attached to hub -> HostConnected");

    HubAttachedCables c2{true, false};
    expect(infer_face_from_cables(c2) == Expression::DisplayConnected, "Hub HDMI attached -> DisplayConnected");

    HubAttachedCables c3{false, true};
    expect(infer_face_from_cables(c3) == Expression::PdConnected, "Hub PD attached -> PdConnected");

    HubAttachedCables c4{true, true};
    expect(infer_face_from_cables(c4) == Expression::DisplayAndPdConnected, "Both attached -> DisplayAndPdConnected");

    expect(ascii_art(Expression::HostConnected).find("-       -") != std::string::npos, "HostConnected ascii art has eyes");
    expect(ascii_art(Expression::DisplayConnected).find("<") != std::string::npos, "DisplayConnected ascii art has mouth");
}

void test_snapshot_diff() {
    using namespace unoface::core;

    Snapshot before;
    before.cables = {false, false};

    HubDeviceInfo hub1;
    hub1.vendor_id = 0x05E3;
    hub1.product_id = 0x0610;
    hub1.instance_id = "USB\\VID_05E3&PID_0610\\1";
    PortStatus p1;
    p1.port_index = 1;
    p1.is_connected = false;
    hub1.ports.push_back(p1);
    before.hubs.push_back(hub1);

    Snapshot after = before;
    after.cables.pd_connected = true;
    after.hubs[0].ports[0].is_connected = true;
    after.hubs[0].ports[0].vendor_id = 0x1234;
    after.hubs[0].ports[0].product_id = 0x5678;

    auto diffs = diff_snapshots(before, after);
    expect(!diffs.empty(), "Diff detected changes");

    bool found_cable_diff = false;
    bool found_port_diff = false;
    for (const auto& d : diffs) {
        if (d.find("hub PD charger") != std::string::npos) found_cable_diff = true;
        if (d.find("port 1") != std::string::npos) found_port_diff = true;
    }
    expect(found_cable_diff, "Diff tracks hub PD charger change");
    expect(found_port_diff, "Diff tracks port connection change");
}

void test_descriptor_parser() {
    using namespace unoface::usb;

    std::vector<uint8_t> config = {
        0x09, 0x02, 0x19, 0x00, 0x01, 0x01, 0x00, 0xE0, 0x32,
        0x09, 0x04, 0x00, 0x00, 0x01, 0x09, 0x00, 0x01, 0x00
    };

    expect(read_total_length(config) == 25, "read_total_length parses total length");
    auto ifaces = parse_interfaces(config);
    expect(ifaces.size() == 1, "parse_interfaces found 1 interface");
    if (!ifaces.empty()) {
        expect(ifaces[0].number == 0, "Interface number is 0");
        expect(ifaces[0].klass == 0x09, "Interface class is 0x09 (Hub)");
    }

    std::vector<uint8_t> bos = {
        0x05, 0x0F, 0x16, 0x00, 0x02,
        0x07, 0x10, 0x02, 0x02, 0x00, 0x00, 0x00,
        0x0A, 0x10, 0x03, 0x00, 0x0E, 0x00, 0x03, 0x00, 0x00, 0x00
    };

    auto caps = parse_bos_capabilities(bos);
    expect(caps.size() == 2, "parse_bos_capabilities found 2 capabilities");
    if (caps.size() == 2) {
        expect(caps[0] == 0x02, "First cap is USB 2.0 Extension");
        expect(caps[1] == 0x03, "Second cap is SuperSpeed USB");
    }

    std::vector<uint8_t> str_desc = {
        0x0E, 0x03, 'U', 0x00, 'G', 0x00, 'R', 0x00, 'E', 0x00, 'E', 0x00, 'N', 0x00
    };
    expect(parse_string_descriptor(str_desc) == "UGREEN", "parse_string_descriptor parses UTF-16LE string");
}

void test_capture_recorder() {
    using namespace unoface::core;
    using namespace unoface::investigation;

    CaptureRecorder recorder([]() { return "2026-11-22 00:20:00.000"; });

    Snapshot baseline;
    baseline.cables = {false, false};

    auto start_lines = recorder.start("test_capture.log", baseline);
    expect(recorder.is_recording(), "Recorder is active");
    expect(!start_lines.empty(), "Start emitted baseline lines");

    auto note_lines = recorder.note("Hub plugged into host");
    expect(!note_lines.empty(), "Note written");

    Snapshot next = baseline;
    next.cables.hdmi_connected = true;
    auto obs_lines = recorder.observe(next, "cables_toggled");
    expect(!obs_lines.empty(), "Observed cable change");

    auto stop_lines = recorder.stop();
    expect(!recorder.is_recording(), "Recorder stopped");
    expect(!stop_lines.empty(), "Stop emitted end marker");

    std::remove("test_capture.log");
}

} // namespace

int main() {
    std::cout << "=== Running UnoFace Tests ===\n";
    test_expression_and_signals();
    test_snapshot_diff();
    test_descriptor_parser();
    test_capture_recorder();

    if (g_failures == 0) {
        std::cout << "=== All tests passed! ===\n";
        return 0;
    }
    std::cerr << "=== Failures: " << g_failures << " ===\n";
    return 1;
}
