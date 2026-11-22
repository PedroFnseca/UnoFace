#pragma once

#include "unoface/core/snapshot.hpp"

#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace unoface {
namespace investigation {

class CaptureRecorder {
public:
    using Clock = std::function<std::string()>;

    explicit CaptureRecorder(Clock clock);

    std::vector<std::string> start(const std::string& file_path, const core::Snapshot& baseline);
    std::vector<std::string> observe(const core::Snapshot& snapshot, const std::string& trigger);
    std::vector<std::string> note(const std::string& text);
    std::vector<std::string> stop();

    bool is_recording() const { return recording_; }
    const std::string& file_path() const { return file_path_; }

private:
    std::string write_line(const std::string& text);

    Clock clock_;
    std::ofstream file_;
    std::string file_path_;
    core::Snapshot last_;
    bool recording_ = false;
};

} // namespace investigation
} // namespace unoface
