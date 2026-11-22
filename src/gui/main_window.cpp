#include "unoface/gui/main_window.hpp"
#include "unoface/platform/win32_system.hpp"
#include "unoface/usb/descriptor_parser.hpp"
#include "unoface/usb/win32_hub_enumerator.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <memory>

namespace unoface {
namespace gui {

namespace {

const char kWindowClassName[] = "UnoFaceMainWindowClass";

const UINT_PTR kPollTimer         = 1;
const UINT_PTR kDeviceChangeTimer = 2;
const UINT kPollIntervalMs         = 1000;
const UINT kDeviceSettleMs         = 700;

enum ControlId {
    IdStart    = 2001,
    IdStop     = 2002,
    IdRefresh  = 2003,
    IdOpen     = 2004,
    IdNote     = 2005,
    IdChkHdmi  = 2006,
    IdChkPd    = 2007
};

const char kStepsText[] =
    "How to investigate:\r\n"
    "1. Click Start Recording.\r\n"
    "2. Toggle cables below to match what is physically plugged into the UGREEN Hub.\r\n"
    "3. Add notes about device behavior and port changes during the session.\r\n"
    "4. Click Stop Recording to save the timestamped log file.";

const char kHubNotice[] =
    "The UGREEN Uno hub's screen reacts strictly to physical signals on its own ports.\r\n"
    "Check the boxes below only when cables are plugged directly into the UGREEN Hub.";

std::string hex4(uint16_t value) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%04X", static_cast<unsigned int>(value));
    return buf;
}

std::string bcd_version(uint16_t bcd) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%X.%02X", static_cast<unsigned int>(bcd >> 8),
                  static_cast<unsigned int>(bcd & 0xFF));
    return buf;
}

std::string capabilities_text(const std::vector<uint8_t>& capabilities) {
    if (capabilities.empty()) {
        return "none";
    }
    std::string text;
    for (size_t i = 0; i < capabilities.size(); ++i) {
        if (i > 0) text += ", ";
        text += usb::capability_name(capabilities[i]);
    }
    return text;
}

std::string hub_descriptor_line(const core::HubDeviceInfo& hub) {
    const core::DescriptorInfo& d = hub.descriptors;
    std::string text = hex4(hub.product_id) + ": ";
    if (!d.available) {
        return text + "descriptors unavailable";
    }
    text += "fw " + bcd_version(d.bcd_device);
    if (!d.product.empty()) {
        text += " '" + d.product + "'";
    }
    text += ", interfaces " + std::to_string(d.interfaces.size());
    text += ", BOS: " + capabilities_text(d.bos_capabilities);
    return text;
}

} // namespace

#ifdef _WIN32

namespace {

HWND as_hwnd(void* handle) {
    return reinterpret_cast<HWND>(handle);
}

LRESULT CALLBACK window_proc_thunk(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTA*>(lparam);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }

    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (self) {
        return static_cast<LRESULT>(
            self->handle_message(hwnd, msg, static_cast<uintptr_t>(wparam), static_cast<intptr_t>(lparam)));
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

HFONT make_font(int height, int weight, DWORD pitch_family, const char* face) {
    return CreateFontA(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, pitch_family, face);
}

void* make_control(HWND parent, HINSTANCE inst, DWORD ex_style, const char* klass, const char* text,
                   DWORD style, int x, int y, int w, int h, int id, void* font) {
    HWND control = CreateWindowExA(ex_style, klass, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                                   parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
    SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

} // namespace

MainWindow::MainWindow()
    : monitor_(std::unique_ptr<usb::IHubEnumerator>(new usb::Win32HubEnumerator()))
    , recorder_(platform::local_timestamp) {}

MainWindow::~MainWindow() {
    if (font_title_)   DeleteObject(reinterpret_cast<HFONT>(font_title_));
    if (font_normal_)  DeleteObject(reinterpret_cast<HFONT>(font_normal_));
    if (font_face_)    DeleteObject(reinterpret_cast<HFONT>(font_face_));
    if (font_log_)     DeleteObject(reinterpret_cast<HFONT>(font_log_));
    if (brush_screen_) DeleteObject(reinterpret_cast<HBRUSH>(brush_screen_));
}

bool MainWindow::create(void* instance_handle, int cmd_show) {
    auto inst = reinterpret_cast<HINSTANCE>(instance_handle);

    WNDCLASSEXA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = window_proc_thunk;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kWindowClassName;
    RegisterClassExA(&wc);

    RECT rect = {0, 0, 720, 690};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, style, FALSE);
    const int width  = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    HWND window = CreateWindowExA(0, kWindowClassName, "UnoFace - UGREEN Uno Hub Monitor & Investigation",
                                  style,
                                  (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
                                  (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
                                  width, height, nullptr, nullptr, inst, this);
    if (!window) {
        return false;
    }
    hwnd_ = window;

    create_controls(inst);
    refresh(true, "startup");
    update_buttons();
    SetTimer(window, kPollTimer, kPollIntervalMs, nullptr);

    ShowWindow(window, cmd_show);
    UpdateWindow(window);
    return true;
}

void MainWindow::create_controls(void* instance_handle) {
    auto inst   = reinterpret_cast<HINSTANCE>(instance_handle);
    HWND parent = as_hwnd(hwnd_);

    font_title_   = make_font(22, FW_BOLD, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    font_normal_  = make_font(15, FW_NORMAL, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    font_face_    = make_font(28, FW_BOLD, FIXED_PITCH | FF_MODERN, "Consolas");
    font_log_     = make_font(14, FW_NORMAL, FIXED_PITCH | FF_MODERN, "Consolas");
    brush_screen_ = CreateSolidBrush(RGB(18, 20, 28));

    make_control(parent, inst, 0, "STATIC", "UGREEN Uno 6-in-1 (35998) Monitor", 0,
                 20, 12, 680, 28, 0, font_title_);
    lbl_hub_ = make_control(parent, inst, 0, "STATIC", "", 0, 20, 42, 680, 20, 0, font_normal_);
    lbl_descriptors_ = make_control(parent, inst, 0, "STATIC", "", 0, 20, 64, 680, 36, 0, font_normal_);

    box_face_ = make_control(parent, inst, 0, "STATIC", "", SS_BLACKRECT, 20, 106, 230, 124, 0, font_normal_);
    lbl_face_ = make_control(parent, inst, 0, "STATIC", "", SS_CENTER, 24, 126, 222, 84, 0, font_face_);

    lbl_face_name_ = make_control(parent, inst, 0, "STATIC", "", 0, 268, 106, 432, 22, 0, font_title_);
    make_control(parent, inst, 0, "STATIC", kHubNotice, 0, 268, 132, 432, 36, 0, font_normal_);

    chk_hub_hdmi_ = make_control(parent, inst, 0, "BUTTON", "HDMI Monitor plugged into UGREEN Hub",
                                 BS_AUTOCHECKBOX, 268, 172, 432, 24, IdChkHdmi, font_normal_);
    chk_hub_pd_   = make_control(parent, inst, 0, "BUTTON", "USB-C PD Charger plugged into UGREEN Hub",
                                 BS_AUTOCHECKBOX, 268, 200, 432, 24, IdChkPd, font_normal_);

    make_control(parent, inst, 0, "STATIC", kStepsText, 0, 20, 244, 680, 96, 0, font_normal_);

    btn_start_   = make_control(parent, inst, 0, "BUTTON", "Start Recording", BS_PUSHBUTTON,
                                20, 348, 160, 32, IdStart, font_normal_);
    btn_stop_    = make_control(parent, inst, 0, "BUTTON", "Stop Recording", BS_PUSHBUTTON,
                                190, 348, 160, 32, IdStop, font_normal_);
    btn_refresh_ = make_control(parent, inst, 0, "BUTTON", "Refresh", BS_PUSHBUTTON,
                                360, 348, 150, 32, IdRefresh, font_normal_);
    btn_open_    = make_control(parent, inst, 0, "BUTTON", "Open Captures Folder", BS_PUSHBUTTON,
                                520, 348, 180, 32, IdOpen, font_normal_);

    edt_note_ = make_control(parent, inst, WS_EX_CLIENTEDGE, "EDIT", "", ES_AUTOHSCROLL,
                             20, 390, 560, 28, 0, font_normal_);
    btn_note_ = make_control(parent, inst, 0, "BUTTON", "Add Note", BS_PUSHBUTTON,
                             590, 388, 110, 32, IdNote, font_normal_);

    edt_log_ = make_control(parent, inst, WS_EX_CLIENTEDGE, "EDIT", "",
                            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL,
                            20, 428, 680, 228, 0, font_log_);
    SendMessageA(as_hwnd(edt_log_), EM_SETLIMITTEXT, 0, 0);

    lbl_status_ = make_control(parent, inst, WS_EX_STATICEDGE, "STATIC", " Ready.", 0,
                               0, 666, 720, 24, 0, font_normal_);
}

int MainWindow::run_message_loop() {
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageA(as_hwnd(hwnd_), &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

void MainWindow::refresh(bool include_descriptors, const std::string& trigger) {
    current_ = monitor_.capture(cables_, include_descriptors);
    update_view();
    if (recorder_.is_recording()) {
        append_log(recorder_.observe(current_, trigger));
    }
}

void MainWindow::on_cables_changed() {
    cables_.hdmi_connected = (SendMessageA(as_hwnd(chk_hub_hdmi_), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cables_.pd_connected   = (SendMessageA(as_hwnd(chk_hub_pd_), BM_GETCHECK, 0, 0) == BST_CHECKED);
    refresh(false, "cables_toggled");
}

void MainWindow::update_view() {
    std::string hub_text;
    std::string descriptor_text;

    if (current_.hubs.empty()) {
        hub_text = "Hub: not detected (looking for Genesys GL3523 05E3:0610 / 05E3:0625)";
    } else {
        hub_text = "Hub: detected";
        for (const auto& hub : current_.hubs) {
            hub_text += "  |  " + hex4(hub.vendor_id) + ":" + hex4(hub.product_id);
            if (!descriptor_text.empty()) descriptor_text += "\r\n";
            descriptor_text += hub_descriptor_line(hub);
        }
    }

    const core::Expression face = core::infer_face_from_cables(current_.cables);

    SetWindowTextA(as_hwnd(lbl_hub_), hub_text.c_str());
    SetWindowTextA(as_hwnd(lbl_descriptors_), descriptor_text.c_str());
    SetWindowTextA(as_hwnd(lbl_face_), core::ascii_art(face).c_str());
    SetWindowTextA(as_hwnd(lbl_face_name_), ("Current face: " + core::display_name(face)).c_str());
}

void MainWindow::update_buttons() {
    const bool recording = recorder_.is_recording();
    EnableWindow(as_hwnd(btn_start_), !recording);
    EnableWindow(as_hwnd(btn_stop_), recording);
    EnableWindow(as_hwnd(btn_note_), recording);
    EnableWindow(as_hwnd(edt_note_), recording);
}

void MainWindow::start_recording() {
    const std::string directory = platform::executable_directory() + "\\captures";
    if (!platform::ensure_directory(directory)) {
        set_status("Could not create folder: " + directory);
        return;
    }

    const std::string path = directory + "\\capture-" + platform::file_timestamp() + ".log";
    current_ = monitor_.capture(cables_, true);
    update_view();

    const auto lines = recorder_.start(path, current_);
    if (lines.empty()) {
        set_status("Could not open log file: " + path);
        return;
    }
    SetWindowTextA(as_hwnd(edt_log_), "");
    append_log(lines);
    update_buttons();
    set_status("Recording to " + path);
}

void MainWindow::stop_recording() {
    append_log(recorder_.stop());
    update_buttons();
    set_status("Saved: " + recorder_.file_path());
}

void MainWindow::add_note() {
    char text[512] = {0};
    GetWindowTextA(as_hwnd(edt_note_), text, sizeof(text));
    const std::string note(text);
    if (note.empty()) {
        return;
    }
    refresh(true, "before-note");
    append_log(recorder_.note(note));
    SetWindowTextA(as_hwnd(edt_note_), "");
}

void MainWindow::append_log(const std::vector<std::string>& lines) {
    HWND log = as_hwnd(edt_log_);
    for (const auto& line : lines) {
        const std::string text = line + "\r\n";
        const int length = GetWindowTextLengthA(log);
        SendMessageA(log, EM_SETSEL, static_cast<WPARAM>(length), static_cast<LPARAM>(length));
        SendMessageA(log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    }
}

void MainWindow::set_status(const std::string& message) {
    SetWindowTextA(as_hwnd(lbl_status_), (" " + message).c_str());
}

intptr_t MainWindow::handle_message(void* hwnd, unsigned int msg, uintptr_t wparam, intptr_t lparam) {
    HWND handle = as_hwnd(hwnd);

    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case IdStart:     start_recording();                                   return 0;
                case IdStop:      stop_recording();                                    return 0;
                case IdRefresh:   refresh(true, "manual"); set_status("Refreshed.");   return 0;
                case IdChkHdmi:
                case IdChkPd:     on_cables_changed();                                 return 0;
                case IdOpen: {
                    const std::string directory = platform::executable_directory() + "\\captures";
                    platform::ensure_directory(directory);
                    platform::open_in_explorer(directory);
                    return 0;
                }
                case IdNote:      add_note();                                          return 0;
                default: break;
            }
            break;

        case WM_TIMER:
            if (wparam == kPollTimer) {
                refresh(false, "poll");
                return 0;
            }
            if (wparam == kDeviceChangeTimer) {
                KillTimer(handle, kDeviceChangeTimer);
                refresh(true, "device-change");
                return 0;
            }
            break;

        case WM_DEVICECHANGE:
            SetTimer(handle, kDeviceChangeTimer, kDeviceSettleMs, nullptr);
            return TRUE;

        case WM_CTLCOLORSTATIC: {
            HWND target = reinterpret_cast<HWND>(lparam);
            if (target == as_hwnd(lbl_face_) || target == as_hwnd(box_face_)) {
                HDC dc = reinterpret_cast<HDC>(wparam);
                SetTextColor(dc, RGB(0, 240, 210));
                SetBkColor(dc, RGB(18, 20, 28));
                return reinterpret_cast<intptr_t>(brush_screen_);
            }
            break;
        }

        case WM_DESTROY:
            KillTimer(handle, kPollTimer);
            KillTimer(handle, kDeviceChangeTimer);
            if (recorder_.is_recording()) {
                recorder_.stop();
            }
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProcA(handle, msg, static_cast<WPARAM>(wparam), static_cast<LPARAM>(lparam));
}

#else

MainWindow::MainWindow()
    : monitor_(std::unique_ptr<usb::IHubEnumerator>(new usb::Win32HubEnumerator()))
    , recorder_(platform::local_timestamp) {}
MainWindow::~MainWindow() = default;
bool MainWindow::create(void*, int) { return false; }
int MainWindow::run_message_loop() { return 0; }
intptr_t MainWindow::handle_message(void*, unsigned int, uintptr_t, intptr_t) { return 0; }

#endif

} // namespace gui
} // namespace unoface
