#include <windows.h>

static int Scale(int value, int dpi) {
    return MulDiv(value, dpi, 96);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        int width  = rc.right  - rc.left;
        int height = rc.bottom - rc.top;
        int dpi    = GetDpiForWindow(hwnd);

        // Background
        HBRUSH bg = CreateSolidBrush(RGB(220, 220, 220));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        // DPI‑scaled font
        int fontPx = Scale(32, dpi);
        HFONT font = CreateFont(
            -fontPx, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            "Segoe UI"
        );
        HFONT oldFont = SelectObject(hdc, font);

        // Layout constants
        const int count = 10;
        int padding = Scale(8, dpi);

        // Spacing that respects both window height and font height
        int minSpacing   = fontPx + padding;
        int idealSpacing = height / (count + 1);
        int spacing      = max(minSpacing, idealSpacing);

        // Column positions
        int colNumX = Scale(60, dpi);
        int colLetX = colNumX + Scale(120, dpi);

        // Draw rows
        for (int i = 0; i < count; i++) {

            int y = spacing * (i + 1);

            // Number
            char num[4];
            wsprintf(num, "%d", i + 1);
            TextOut(hdc, colNumX, y, num, lstrlen(num));

            // Letter
            char letter[2] = { (char)('A' + i), '\0' };
            TextOut(hdc, colLetX, y, letter, 1);

            // Divider line
            int lineY = y + fontPx + padding / 2;
            MoveToEx(hdc, Scale(20, dpi), lineY, NULL);
            LineTo(hdc, width - Scale(20, dpi), lineY);
        }

        SelectObject(hdc, oldFont);
        DeleteObject(font);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {

    const char CLASS_NAME[] = "PhrasePadWindow";

    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "PhrasePad",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 600,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

