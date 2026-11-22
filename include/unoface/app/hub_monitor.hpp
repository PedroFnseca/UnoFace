#pragma once

#include "unoface/core/host_signals.hpp"
#include "unoface/core/snapshot.hpp"
#include "unoface/usb/ihub_enumerator.hpp"

#include <memory>

namespace unoface {
namespace app {

class HubMonitor {
public:
    explicit HubMonitor(std::unique_ptr<usb::IHubEnumerator> enumerator);

    core::Snapshot capture(const core::HubAttachedCables& cables, bool include_descriptors);

private:
    std::unique_ptr<usb::IHubEnumerator> enumerator_;
    core::Snapshot last_;
};

} // namespace app
} // namespace unoface
