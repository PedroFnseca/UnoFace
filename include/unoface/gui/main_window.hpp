#pragma once

#include "unoface/app/hub_monitor.hpp"
#include "unoface/core/host_signals.hpp"
#include "unoface/core/snapshot.hpp"
#include "unoface/investigation/capture_recorder.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace unoface {
namespace gui {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(void* instance_handle, int cmd_show);
    int run_message_loop();

    intptr_t handle_message(void* hwnd, unsigned int msg, uintptr_t wparam, intptr_t lparam);

private:
    void create_controls(void* instance_handle);
    void refresh(bool include_descriptors, const std::string& trigger);
    void update_view();
    void update_buttons();
    void on_cables_changed();
    void start_recording();
    void stop_recording();
    void add_note();
    void append_log(const std::vector<std::string>& lines);
    void set_status(const std::string& message);

    app::HubMonitor monitor_;
    investigation::CaptureRecorder recorder_;
    core::HubAttachedCables cables_;
    core::Snapshot current_;

    void* hwnd_ = nullptr;
    void* font_title_ = nullptr;
    void* font_normal_ = nullptr;
    void* font_face_ = nullptr;
    void* font_log_ = nullptr;
    void* brush_screen_ = nullptr;

    void* lbl_hub_ = nullptr;
    void* lbl_descriptors_ = nullptr;
    void* box_face_ = nullptr;
    void* lbl_face_ = nullptr;
    void* lbl_face_name_ = nullptr;
    void* chk_hub_hdmi_ = nullptr;
    void* chk_hub_pd_ = nullptr;
    void* btn_start_ = nullptr;
    void* btn_stop_ = nullptr;
    void* btn_refresh_ = nullptr;
    void* btn_open_ = nullptr;
    void* edt_note_ = nullptr;
    void* btn_note_ = nullptr;
    void* edt_log_ = nullptr;
    void* lbl_status_ = nullptr;
};

} // namespace gui
} // namespace unoface
