#include "unoface/gui/main_window.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    unoface::gui::MainWindow window;
    if (!window.create(hInstance, nCmdShow)) {
        return 1;
    }
    return window.run_message_loop();
}
#else
int main() {
    return 0;
}
#endif
