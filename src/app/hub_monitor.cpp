#include "unoface/app/hub_monitor.hpp"

#include <utility>

namespace unoface {
namespace app {

HubMonitor::HubMonitor(std::unique_ptr<usb::IHubEnumerator> enumerator)
    : enumerator_(std::move(enumerator)) {}

core::Snapshot HubMonitor::capture(const core::HubAttachedCables& cables, bool include_descriptors) {
    core::Snapshot snapshot;
    snapshot.hubs   = enumerator_->enumerate(include_descriptors);
    snapshot.cables = cables;

    if (!include_descriptors) {
        for (auto& hub : snapshot.hubs) {
            for (const auto& previous : last_.hubs) {
                if (previous.instance_id == hub.instance_id) {
                    hub.descriptors = previous.descriptors;
                }
            }
        }
    }

    last_ = snapshot;
    return snapshot;
}

} // namespace app
} // namespace unoface
