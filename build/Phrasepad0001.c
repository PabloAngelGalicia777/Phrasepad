#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT r;
        GetClientRect(hwnd, &r);

        // Window dimensions
        int width  = r.right  - r.left;
        int height = r.bottom - r.top;

        // Fill background
        FillRect(hdc, &r, (HBRUSH)(COLOR_GRAYTEXT + 1));

        // Compute scalable layout
        int marginX = width  / 10;     // 10% from left
        int marginY = height / 15;     // 15% from top
        int spacing = height / 15;     // vertical spacing scales with height

        // Draw numbers 1–10
        for (int i = 0; i < 10; i++) {
            char buffer[16];
            wsprintf(buffer, "%d", i + 1);

            int x = marginX;
            int y = marginY + i * spacing;

            TextOut(hdc, x, y, buffer, lstrlen(buffer));
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {

    const char CLASS_NAME[] = "PhrasePadWindow";

    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_GRAYTEXT + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "PhrasePad",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}