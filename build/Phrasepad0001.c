#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wchar.h>
#include <stdlib.h>

#define FIRST_GEN_COUNT 7

HWND main_content = NULL;
HWND first_gen[FIRST_GEN_COUNT] = {0};
HWND content_window[FIRST_GEN_COUNT] = {0}; // single 2nd_gen per 1st_gen

int scroll_pos = 0;

LRESULT CALLBACK MainContentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GenPaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oldMainContentProc = NULL;

//
// DPI-aware geometry helpers
//
int GetMargin(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(12, dpi, 96);   // base margin
}

int GetSpacing(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(10, dpi, 96);   // spacing between 1st_gen windows
}

int GetChildHeight(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(120, dpi, 96);  // height of each 1st_gen
}

//
// Clipboard helpers
//
wchar_t* GetClipboardText(HWND hwnd)
{
    if (!OpenClipboard(hwnd))
        return NULL;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData)
    {
        CloseClipboard();
        return NULL;
    }

    wchar_t* pszText = (wchar_t*)GlobalLock(hData);
    if (!pszText)
    {
        CloseClipboard();
        return NULL;
    }

    size_t len = wcslen(pszText) + 1;
    wchar_t* copy = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!copy)
    {
        GlobalUnlock(hData);
        CloseClipboard();
        return NULL;
    }

    wcscpy(copy, pszText);

    GlobalUnlock(hData);
    CloseClipboard();

    return copy; // caller must free()
}

void SetClipboardText(HWND hwnd, const wchar_t* text)
{
    if (!text)
        return;

    if (!OpenClipboard(hwnd))
        return;

    EmptyClipboard();

    size_t len = wcslen(text) + 1;
    SIZE_T bytes = len * sizeof(wchar_t);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem)
    {
        CloseClipboard();
        return;
    }

    wchar_t* dest = (wchar_t*)GlobalLock(hMem);
    if (!dest)
    {
        GlobalFree(hMem);
        CloseClipboard();
        return;
    }

    memcpy(dest, text, bytes);
    GlobalUnlock(hMem);

    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

//
// Layout single content window inside each 1st_gen
//
void LayoutContentWindow(HWND firstGenHwnd, int index)
{
    RECT rc;
    GetClientRect(firstGenHwnd, &rc);

    int margin = GetMargin(firstGenHwnd);

    int x = margin;
    int y = margin;
    int w = rc.right - rc.left - 2 * margin;
    int h = rc.bottom - rc.top - 2 * margin;
    if (w < 0) w = 0;
    if (h < 0) h = 0;

    MoveWindow(content_window[index], x, y, w, h, TRUE);
}

//
// Layout 1st_gen windows inside main_content
//
void LayoutFirstGenWindows(HWND parent)
{
    RECT rc;
    GetClientRect(parent, &rc);

    int spacing = GetSpacing(parent);
    int h = GetChildHeight(parent);

    int y = spacing - scroll_pos;

    for (int i = 0; i < FIRST_GEN_COUNT; i++)
    {
        MoveWindow(first_gen[i],
                   spacing,
                   y,
                   rc.right - rc.left - spacing * 2,
                   h,
                   TRUE);

        LayoutContentWindow(first_gen[i], i);

        y += h + spacing;
    }

    int contentBottom = y;
    int contentHeight = contentBottom;
    if (contentHeight < rc.bottom) contentHeight = rc.bottom;

    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = contentHeight - 1;
    si.nPage = rc.bottom - rc.top;
    if (si.nPage == 0) si.nPage = 1;

    if (scroll_pos > (int)(si.nMax - si.nPage + 1))
        scroll_pos = si.nMax - si.nPage + 1;
    if (scroll_pos < 0) scroll_pos = 0;

    si.nPos = scroll_pos;
    SetScrollInfo(parent, SB_VERT, &si, TRUE);

    InvalidateRect(parent, NULL, TRUE);
}

//
// Create all windows
//
void CreateContentWindows(HWND hwnd)
{
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    main_content = CreateWindowEx(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        0, 0, 0, 0,
        hwnd, (HMENU)100,
        hInst,
        NULL
    );

    oldMainContentProc = (WNDPROC)SetWindowLongPtr(
        main_content, GWLP_WNDPROC, (LONG_PTR)MainContentProc);

    COLORREF first_gen_colors[FIRST_GEN_COUNT] = {
        RGB(255, 0, 0),
        RGB(0, 200, 0),
        RGB(0, 120, 255),
        RGB(255, 255, 0),
        RGB(180, 0, 180),
        RGB(255, 128, 0),
        RGB(0, 200, 200)
    };

    for (int i = 0; i < FIRST_GEN_COUNT; i++)
    {
        first_gen[i] = CreateWindowEx(
            0, L"GenPane", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            main_content, (HMENU)(200 + i),
            hInst,
            NULL
        );
        SetWindowLongPtr(first_gen[i], GWLP_USERDATA, (LONG_PTR)first_gen_colors[i]);

        // Single full-width content window: EDIT control, multiline, wrapping
        content_window[i] = CreateWindowEx(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_LEFT,
            0, 0, 0, 0,
            first_gen[i], (HMENU)(400 + i),
            hInst,
            NULL
        );
        // Color is handled via WM_CTLCOLOREDIT in GenPaneProc.
    }
}

//
// Draw outline around any HWND
//
void DrawOutline(HWND hwnd, HDC hdc, HWND target)
{
    RECT rc;
    GetWindowRect(target, &rc);
    MapWindowPoints(HWND_DESKTOP, hwnd, (POINT*)&rc, 2);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    HGDIOBJ old = SelectObject(hdc, pen);

    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    SelectObject(hdc, old);
    DeleteObject(pen);
}

//
// Subclass procedure for main_content
//
LRESULT CALLBACK MainContentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
            LayoutFirstGenWindows(hwnd);
            return 0;

        case WM_VSCROLL:
        {
            SCROLLINFO si = { sizeof(si), SIF_ALL };
            GetScrollInfo(hwnd, SB_VERT, &si);

            int newPos = scroll_pos;

            switch (LOWORD(wParam))
            {
                case SB_LINEUP:     newPos -= 20; break;
                case SB_LINEDOWN:   newPos += 20; break;
                case SB_PAGEUP:     newPos -= si.nPage; break;
                case SB_PAGEDOWN:   newPos += si.nPage; break;
                case SB_THUMBTRACK: newPos = si.nTrackPos; break;
            }

            if (newPos < 0) newPos = 0;
            if (newPos > (int)(si.nMax - si.nPage + 1))
                newPos = si.nMax - si.nPage + 1;

            scroll_pos = newPos;
            LayoutFirstGenWindows(hwnd);
            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            scroll_pos -= delta / 2;

            SCROLLINFO si = { sizeof(si), SIF_ALL };
            GetScrollInfo(hwnd, SB_VERT, &si);

            if (scroll_pos < 0) scroll_pos = 0;
            if (scroll_pos > (int)(si.nMax - si.nPage + 1))
                scroll_pos = si.nMax - si.nPage + 1;

            LayoutFirstGenWindows(hwnd);
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH grey = CreateSolidBrush(RGB(200,200,200));
            FillRect(hdc, &rc, grey);
            DeleteObject(grey);

            EndPaint(hwnd, &ps);
        }
        return 0;
    }

    return CallWindowProc(oldMainContentProc, hwnd, msg, wParam, lParam);
}

//
// Paint handler and color handler for GenPane windows
//
LRESULT CALLBACK GenPaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            COLORREF color = (COLORREF)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);

            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            HWND hEdit = (HWND)lParam;

            // If this edit is one of our content windows, style it
            for (int i = 0; i < FIRST_GEN_COUNT; i++)
            {
                if (hEdit == content_window[i])
                {
                    static HBRUSH darkBrush = NULL;
                    if (!darkBrush)
                        darkBrush = CreateSolidBrush(RGB(60, 60, 60));

                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(60, 60, 60));
                    return (LRESULT)darkBrush;
                }
            }
        }
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// Slot operations
//
void FillSlotFromClipboard(int index, HWND hwnd)
{
    if (index < 0 || index >= FIRST_GEN_COUNT)
        return;

    wchar_t* text = GetClipboardText(hwnd);
    if (!text)
        return;

    SetWindowText(content_window[index], text);

    free(text);
}

void SetClipboardFromSlot(int index, HWND hwnd)
{
    if (index < 0 || index >= FIRST_GEN_COUNT)
        return;

    int len = GetWindowTextLength(content_window[index]);
    if (len <= 0)
        return;

    wchar_t* buffer = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (!buffer)
        return;

    GetWindowText(content_window[index], buffer, len + 1);
    SetClipboardText(hwnd, buffer);

    free(buffer);
}

//
// Main window procedure
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            CreateContentWindows(hwnd);

            // Register hotkeys for slots 1–7
            // Ctrl + Alt + N : fill slot
            // Ctrl + Shift + N : override clipboard from slot
            for (int i = 0; i < FIRST_GEN_COUNT; i++)
            {
                RegisterHotKey(hwnd,
                               100 + i, // id for fill
                               MOD_CONTROL | MOD_ALT,
                               '1' + i);

                RegisterHotKey(hwnd,
                               200 + i, // id for override
                               MOD_CONTROL | MOD_SHIFT,
                               '1' + i);
            }
            return 0;
        }

        case WM_SIZE:
            MoveWindow(main_content, 0, 0,
                       LOWORD(lParam), HIWORD(lParam), TRUE);
            LayoutFirstGenWindows(main_content);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_HOTKEY:
        {
            int id = (int)wParam;

            if (id >= 100 && id < 100 + FIRST_GEN_COUNT)
            {
                // Fill slot (Ctrl+Alt+N)
                int index = id - 100;
                FillSlotFromClipboard(index, hwnd);
            }
            else if (id >= 200 && id < 200 + FIRST_GEN_COUNT)
            {
                // Override clipboard from slot (Ctrl+Shift+N)
                int index = id - 200;
                SetClipboardFromSlot(index, hwnd);
            }
            return 0;
        }

        case WM_MOUSEWHEEL:
            if (main_content)
                SendMessage(main_content, WM_MOUSEWHEEL, wParam, lParam);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            DrawOutline(hwnd, hdc, main_content);

            for (int i = 0; i < FIRST_GEN_COUNT; i++)
            {
                DrawOutline(hwnd, hdc, first_gen[i]);
                DrawOutline(hwnd, hdc, content_window[i]);
            }

            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_DESTROY:
            // Unregister hotkeys; controls are destroyed, contents cleared
            for (int i = 0; i < FIRST_GEN_COUNT; i++)
            {
                UnregisterHotKey(hwnd, 100 + i);
                UnregisterHotKey(hwnd, 200 + i);
            }

            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// Entry point
//
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE prev, PWSTR cmd, int show)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"ScrollableContentDemo";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    WNDCLASS wcPane = {0};
    wcPane.lpfnWndProc   = GenPaneProc;
    wcPane.hInstance     = hInst;
    wcPane.lpszClassName = L"GenPane";
    wcPane.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wcPane);

    HWND hwnd = CreateWindowEx(
        0,
        L"ScrollableContentDemo",
        L"PhrasePad Layout Demo (Single Content Window)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, show);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

