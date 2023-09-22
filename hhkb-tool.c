#pragma warning(disable : 5039)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define APPLICATION_MUTEX_NAME L"10d92afe-e2dd-4dee-9a91-e24ad5d8c492"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

static const unsigned char G_KeyMap[32][2] = {
    {0x57 /* W */, VK_UP},    {0x41 /* A */, VK_LEFT},
    {0x44 /* D */, VK_RIGHT}, {0x53 /* S */, VK_DOWN},
    {0x45 /* E */, VK_END},   {0x51 /* Q */, VK_HOME},
    {0x52 /* R */, VK_PRIOR}, {0x46 /* F */, VK_NEXT},
    {0x5A /* Z */, VK_DELETE}};

static BOOL G_RightShiftDown = FALSE;
static BOOL G_LeftShiftDown = FALSE;

static LRESULT LowLevelKeyboardCallback(int code, WPARAM wparam, LPARAM lparam) {
  if (code != HC_ACTION) {
    return CallNextHookEx(NULL, code, wparam, lparam);
  }

  KBDLLHOOKSTRUCT* input_event = (KBDLLHOOKSTRUCT*)lparam;

  if (input_event != NULL) {
    if (input_event->flags & LLKHF_INJECTED) {
      return CallNextHookEx(NULL, code, wparam, lparam);
    }

    BOOL key_down = (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN);

    switch (input_event->vkCode) {
      case VK_RSHIFT: {
        G_RightShiftDown = key_down;
      } break;
      case VK_LSHIFT: {
        G_LeftShiftDown = key_down;
      } break;
      case VK_ESCAPE: {
        if (G_LeftShiftDown && key_down) {
          keybd_event(VK_OEM_3, 0, 0ul, 0);
          keybd_event(VK_OEM_3, 0, KEYEVENTF_KEYUP, 0);
          return 1;
        }
      } break;
      default:
        break;
    }

    if (G_RightShiftDown && key_down) {
      for (int i = 0; i < ArrayCount(G_KeyMap); i++) {
        if (G_KeyMap[i][0] == input_event->vkCode) {
          keybd_event(VK_RSHIFT, 0, KEYEVENTF_KEYUP, 0);
          keybd_event(G_KeyMap[i][1], 0, 0ul, 0);
          return 1;
        }
      }
    }
  }

  return CallNextHookEx(NULL, code, wparam, lparam);
}

void WinMainCRTStartup() {
  HANDLE mutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, APPLICATION_MUTEX_NAME);

  if (!mutex) {
    DWORD last_error = GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND) {
      mutex = CreateMutexW(NULL, FALSE, APPLICATION_MUTEX_NAME);

      HHOOK keyboard_hook = SetWindowsHookExW(
          WH_KEYBOARD_LL, &LowLevelKeyboardCallback, NULL, 0ul);

      if (keyboard_hook != NULL) {
        for (;;) {
          MSG msg = {0};
          if (GetMessageW(&msg, 0, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
          } else {
            break;
          }
        }
        UnhookWindowsHookEx(keyboard_hook);
      }
    }
  }
  if (mutex != NULL) ReleaseMutex(mutex);

  ExitProcess(0);
}