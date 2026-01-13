#define UNICODE
#define _UNICODE
#include <windows.h>

#define FIRST_GEN_COUNT 7

HWND main_content = NULL;
HWND first_gen[FIRST_GEN_COUNT] = {0};
HWND second_gen_left[FIRST_GEN_COUNT] = {0};
HWND second_gen_right[FIRST_GEN_COUNT] = {0};

int scroll_pos = 0;

LRESULT CALLBACK MainContentProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GenPaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC oldMainContentProc = NULL;

//
// DPI‑aware margins
//
int GetMargin(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(12, dpi, 96);   // base margin
}

int GetSpacing(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(10, dpi, 96);   // HALF spacing between 1st_gen windows
}

int GetSecondGenSpacing(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(10, dpi, 96);   // HALF spacing between 2nd_gen siblings
}

int GetChildHeight(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    return MulDiv(120, dpi, 96);  // height of each 1st_gen
}

//
// Layout 2nd_gen windows inside each 1st_gen
//
void LayoutSecondGenWindows(HWND firstGenHwnd, int index)
{
    RECT rc;
    GetClientRect(firstGenHwnd, &rc);

    int margin = GetMargin(firstGenHwnd);
    int spacing = GetSecondGenSpacing(firstGenHwnd);

    int totalHeight = rc.bottom - rc.top;
    int squareSize = totalHeight - 2 * margin;
    if (squareSize < 0) squareSize = 0;

    // Left 2nd_gen (square)
    int leftX = margin;
    int leftY = margin;
    int leftW = squareSize;
    int leftH = squareSize;

    // Right 2nd_gen (fills remaining width)
    int rightX = leftX + leftW + spacing;
    int rightY = margin;
    int rightW = rc.right - rc.left - rightX - margin;
    if (rightW < 0) rightW = 0;
    int rightH = squareSize;

    MoveWindow(second_gen_left[index], leftX, leftY, leftW, leftH, TRUE);
    MoveWindow(second_gen_right[index], rightX, rightY, rightW, rightH, TRUE);
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

        LayoutSecondGenWindows(first_gen[i], i);

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

    COLORREF second_left_color  = RGB(0, 200, 0);   // green
    COLORREF second_right_color = RGB(60, 60, 60);  // dark gray

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

        second_gen_left[i] = CreateWindowEx(
            0, L"GenPane", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            first_gen[i], (HMENU)(300 + i),
            hInst,
            NULL
        );
        SetWindowLongPtr(second_gen_left[i], GWLP_USERDATA, (LONG_PTR)second_left_color);

        second_gen_right[i] = CreateWindowEx(
            0, L"GenPane", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            first_gen[i], (HMENU)(400 + i),
            hInst,
            NULL
        );
        SetWindowLongPtr(second_gen_right[i], GWLP_USERDATA, (LONG_PTR)second_right_color);
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
// Paint handler for all GenPane windows
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
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// Main window procedure
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            CreateContentWindows(hwnd);
            return 0;

        case WM_SIZE:
            MoveWindow(main_content, 0, 0,
                       LOWORD(lParam), HIWORD(lParam), TRUE);
            LayoutFirstGenWindows(main_content);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            DrawOutline(hwnd, hdc, main_content);

            for (int i = 0; i < FIRST_GEN_COUNT; i++)
            {
                DrawOutline(hwnd, hdc, first_gen[i]);
                DrawOutline(hwnd, hdc, second_gen_left[i]);
                DrawOutline(hwnd, hdc, second_gen_right[i]);
            }

            EndPaint(hwnd, &ps);
        }
        return 0;

        case WM_DESTROY:
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
        L"1st_gen / 2nd_gen Layout Demo",
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

