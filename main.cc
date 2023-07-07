#pragma warning(disable : 5039)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace {
constexpr const wchar_t kMutexName[] = L"10d92afe-e2dd-4dee-9a91-e24ad5d8c492";

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

const unsigned char KeyMap[32][2] = {
    {0x57 /* W */, VK_UP},    {0x41 /* A */, VK_LEFT},
    {0x44 /* D */, VK_RIGHT}, {0x53 /* S */, VK_DOWN},
    {0x45 /* E */, VK_END},   {0x51 /* Q */, VK_HOME},
    {0x52 /* R */, VK_PRIOR}, {0x46 /* F */, VK_NEXT},
    {0x5A /* Z */, VK_DELETE}};

bool _RightShiftDown = false;
bool _LeftShiftDown = false;

LRESULT LowLevelKeyboardCallback(int code, WPARAM wparam, LPARAM lparam) {
  if (code != HC_ACTION) {
    return ::CallNextHookEx(nullptr, code, wparam, lparam);
  }

  const auto* input_event = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);

  if (input_event->flags & LLKHF_INJECTED) {
    return ::CallNextHookEx(nullptr, code, wparam, lparam);
  }

  const bool is_key_down = (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN);

  if (input_event->vkCode == VK_RSHIFT) {
    _RightShiftDown = is_key_down;
  }

  if (input_event->vkCode == VK_LSHIFT) {
    _LeftShiftDown = is_key_down;
  }

  if (_RightShiftDown && is_key_down) {
    for (auto i = 0; i < ArrayCount(KeyMap); i++) {
      if (KeyMap[i][0] == input_event->vkCode) {
        ::keybd_event(VK_RSHIFT, 0, KEYEVENTF_KEYUP, 0);
        ::keybd_event(KeyMap[i][1], 0, 0ul, 0);
        return 1;
      }
    }
  }

  if (_LeftShiftDown && is_key_down && input_event->vkCode == VK_ESCAPE) {
    ::keybd_event(VK_OEM_3, 0, 0ul, 0);
    ::keybd_event(VK_OEM_3, 0, KEYEVENTF_KEYUP, 0);
    return 1;
  }

  return ::CallNextHookEx(nullptr, code, wparam, lparam);
}
}  // namespace

void WinMainCRTStartup() {
  HANDLE mutex = ::OpenMutexW(MUTEX_ALL_ACCESS, false, kMutexName);

  if (!mutex) {
    DWORD last_error = ::GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND) {
      mutex = ::CreateMutexW(nullptr, false, kMutexName);

      const HHOOK keyboard_hook = ::SetWindowsHookExW(
          WH_KEYBOARD_LL, &LowLevelKeyboardCallback, nullptr, 0ul);

      if (keyboard_hook != nullptr) {
        for (;;) {
          MSG msg{};
          if (::GetMessageW(&msg, 0, 0, 0)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
          } else {
            break;
          }
        }
        ::UnhookWindowsHookEx(keyboard_hook);
      }
    }
  }
  if (mutex != nullptr) ::ReleaseMutex(mutex);

  ::ExitProcess(0);
}