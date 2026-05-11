#include "global.h"
#include <windows.h>


static DWORD origMode = 0;


void resetTerminalMode()
{
    if (origMode) {
        HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(stdinHandle, origMode);
    }
}


void setTerminalMode(bool enableEcho)
{
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(stdinHandle, &mode);

    if (!origMode) {
        origMode = mode;
    }

    mode &= ~ENABLE_LINE_INPUT;
    if (enableEcho) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(stdinHandle, mode);
}


QByteArray readStdInput()
{
    QByteArray input;
    DWORD count = 0;

    if (!GetNumberOfConsoleInputEvents(stdinHandle, &count) || count == 0) {
        return input;
    }

    INPUT_RECORD records[64];
    DWORD readCount = 0;

    if (!ReadConsoleInputW(stdinHandle, records, 64, &readCount)) {
        return input;
    }

    for (DWORD i = 0; i < readCount; ++i) {
        const INPUT_RECORD &rec = records[i];

        if (rec.EventType != KEY_EVENT) {
            continue;
        }

        const KEY_EVENT_RECORD &key = rec.Event.KeyEvent;

        if (!key.bKeyDown) {
            continue;
        }

        wchar_t ch = key.uChar.UnicodeChar;
        if (ch == 0) {
            continue;
        }

        if (ch == L'\r') {
            input += "\r\n";
        } else if (key.wVirtualKeyCode == VK_BACK) {
            input += (char)0x7f;
        } else {
            input += QString::fromWCharArray(&ch, 1).toUtf8();
        }
    }

    return input;
}
