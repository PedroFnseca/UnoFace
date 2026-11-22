#include "unoface/investigation/capture_recorder.hpp"

#include <utility>

namespace unoface {
namespace investigation {

CaptureRecorder::CaptureRecorder(Clock clock)
    : clock_(std::move(clock)) {}

std::string CaptureRecorder::write_line(const std::string& text) {
    const std::string line = clock_() + "  " + text;
    file_ << line << "\n";
    file_.flush();
    return line;
}

std::vector<std::string> CaptureRecorder::start(const std::string& file_path, const core::Snapshot& baseline) {
    std::vector<std::string> lines;
    if (recording_) {
        return lines;
    }

    file_.open(file_path.c_str(), std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        return lines;
    }

    file_path_ = file_path;
    recording_ = true;
    last_      = baseline;

    lines.push_back(write_line("[session] capture started"));
    for (const auto& text : core::describe_snapshot(baseline)) {
        lines.push_back(write_line("[baseline] " + text));
    }
    return lines;
}

std::vector<std::string> CaptureRecorder::observe(const core::Snapshot& snapshot, const std::string& trigger) {
    std::vector<std::string> lines;
    if (!recording_) {
        return lines;
    }

    for (const auto& text : core::diff_snapshots(last_, snapshot)) {
        lines.push_back(write_line("[" + trigger + "] " + text));
    }
    last_ = snapshot;
    return lines;
}

std::vector<std::string> CaptureRecorder::note(const std::string& text) {
    std::vector<std::string> lines;
    if (recording_ && !text.empty()) {
        lines.push_back(write_line("[note] " + text));
    }
    return lines;
}

std::vector<std::string> CaptureRecorder::stop() {
    std::vector<std::string> lines;
    if (!recording_) {
        return lines;
    }
    lines.push_back(write_line("[session] capture stopped"));
    file_.close();
    recording_ = false;
    return lines;
}

} // namespace investigation
} // namespace unoface
