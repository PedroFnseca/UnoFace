#pragma once

#include "unoface/core/host_signals.hpp"
#include "unoface/core/types.hpp"

#include <string>
#include <vector>

namespace unoface {
namespace core {

struct Snapshot {
    std::vector<HubDeviceInfo> hubs;
    HubAttachedCables cables;
};

std::vector<std::string> describe_snapshot(const Snapshot& snapshot);
std::vector<std::string> diff_snapshots(const Snapshot& before, const Snapshot& after);

} // namespace core
} // namespace unoface
