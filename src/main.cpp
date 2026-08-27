#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <string>

namespace {

constexpr wchar_t kWindowClass[] = L"MoyuSimulatorFinalWindow";
constexpr wchar_t kHostClass[] = L"MoyuSimulatorEmbedHost";
constexpr UINT WM_ATTACH_WINDOW = WM_APP + 10;
constexpr int ID_START = 1001;
constexpr int ID_DETACH = 1002;
constexpr int ID_CLOSE = 1003;

HINSTANCE g_instance = nullptr;
HWND g_main = nullptr;
HWND g_start = nullptr;
HWND g_detach = nullptr;
HWND g_close = nullptr;
HWND g_host = nullptr;
HWINEVENTHOOK g_moveHook = nullptr;
bool g_adMode = false;

struct EmbeddedWindow {
    HWND hwnd = nullptr;
    HWND parent = nullptr;
    LONG_PTR style = 0;
    LONG_PTR exStyle = 0;
    RECT rect{};
    bool valid = false;
} g_embedded;

COLORREF kNavy = RGB(24, 31, 48);
COLORREF kBlue = RGB(43, 112, 255);
COLORREF kPale = RGB(246, 248, 252);
COLORREF kMuted = RGB(104, 116, 138);

HFONT MakeFont(int px, int weight = FW_NORMAL) {
    return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       L"Microsoft YaHei UI");
}

void FillSolid(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void DrawTextAt(HDC dc, const wchar_t* text, RECT rect, HFONT font,
                COLORREF color, UINT format) {
    HFONT old = static_cast<HFONT>(SelectObject(dc, font));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &rect, format);
    SelectObject(dc, old);
}

RECT HostScreenRect() {
    RECT rect{};
    if (!g_host || !IsWindow(g_host)) return rect;
    GetClientRect(g_host, &rect);
    POINT tl{rect.left, rect.top};
    POINT br{rect.right, rect.bottom};
    ClientToScreen(g_host, &tl);
    ClientToScreen(g_host, &br);
    return RECT{tl.x, tl.y, br.x, br.y};
}

bool IsOurWindow(HWND hwnd) {
    if (!hwnd) return true;
    if (hwnd == g_main || hwnd == g_host || hwnd == g_start ||
        hwnd == g_detach || hwnd == g_close) return true;
    return IsChild(g_main, hwnd) != FALSE;
}

void ResizeEmbedded() {
    if (!g_embedded.valid || !IsWindow(g_embedded.hwnd) || !g_host) return;
    RECT area{};
    GetClientRect(g_host, &area);
    SetWindowPos(g_embedded.hwnd, nullptr, 0, 0,
                 area.right - area.left, area.bottom - area.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void SetStatus(const std::wstring& text) {
    SetWindowTextW(g_host, text.c_str());
    InvalidateRect(g_host, nullptr, TRUE);
}

void DetachEmbedded(bool bringToFront) {
    if (!g_embedded.valid) return;

    HWND target = g_embedded.hwnd;
    EmbeddedWindow saved = g_embedded;
    g_embedded = EmbeddedWindow{};

    if (IsWindow(target)) {
        ShowWindow(target, SW_HIDE);
        SetParent(target, saved.parent);
        SetWindowLongPtrW(target, GWL_STYLE, saved.style);
        SetWindowLongPtrW(target, GWL_EXSTYLE, saved.exStyle);
        int width = saved.rect.right - saved.rect.left;
        int height = saved.rect.bottom - saved.rect.top;
        SetWindowPos(target, bringToFront ? HWND_TOP : nullptr,
                     saved.rect.left, saved.rect.top, width, height,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW |
                         (bringToFront ? 0 : SWP_NOZORDER));
        if (bringToFront) {
            ShowWindow(target, SW_RESTORE);
            SetForegroundWindow(target);
        }
    }
    ShowWindow(g_detach, SW_HIDE);
    SetStatus(L"把任意软件窗口拖到这里，松手即可嵌入");
}

bool AttachWindow(HWND candidate) {
    if (!candidate || !IsWindow(candidate) || IsOurWindow(candidate)) return false;

    HWND top = GetAncestor(candidate, GA_ROOT);
    if (top) candidate = top;
    if (IsOurWindow(candidate) || !IsWindowVisible(candidate)) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(candidate, &pid);
    if (pid == GetCurrentProcessId()) return false;

    wchar_t title[256]{};
    GetWindowTextW(candidate, title, 255);

    DetachEmbedded(false);

    g_embedded.hwnd = candidate;
    g_embedded.parent = GetParent(candidate);
    g_embedded.style = GetWindowLongPtrW(candidate, GWL_STYLE);
    g_embedded.exStyle = GetWindowLongPtrW(candidate, GWL_EXSTYLE);
    GetWindowRect(candidate, &g_embedded.rect);

    ShowWindow(candidate, SW_HIDE);
    SetLastError(ERROR_SUCCESS);
    HWND result = SetParent(candidate, g_host);
    DWORD error = GetLastError();
    if ((!result && error != ERROR_SUCCESS) || GetParent(candidate) != g_host) {
        SetParent(candidate, g_embedded.parent);
        SetWindowLongPtrW(candidate, GWL_STYLE, g_embedded.style);
        SetWindowLongPtrW(candidate, GWL_EXSTYLE, g_embedded.exStyle);
        ShowWindow(candidate, SW_SHOW);
        g_embedded = EmbeddedWindow{};
        SetStatus(L"该窗口受系统权限保护，无法嵌入；请以相同权限运行两个程序");
        return false;
    }

    LONG_PTR childStyle = g_embedded.style;
    childStyle &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU |
                    WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    childStyle |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtrW(candidate, GWL_STYLE, childStyle);
    SetWindowLongPtrW(candidate, GWL_EXSTYLE,
                      g_embedded.exStyle & ~(WS_EX_APPWINDOW | WS_EX_TOPMOST));
    g_embedded.valid = true;
    ResizeEmbedded();
    ShowWindow(candidate, SW_SHOW);
    if (GetParent(candidate) != g_host || !IsWindowVisible(candidate)) {
        DetachEmbedded(true);
        SetStatus(L"窗口未成功进入广告区域，已自动还原，请重新拖入");
        return false;
    }
    ShowWindow(g_detach, SW_SHOW);

    std::wstring status = title[0] ? std::wstring(L"已嵌入：") + title
                                   : L"窗口已嵌入，可直接操作";
    SetStatus(status);
    SetForegroundWindow(g_main);
    SetFocus(candidate);
    return true;
}

bool CandidateTouchesHost(HWND candidate) {
    if (!g_adMode || !g_host || !IsWindowVisible(g_main) || IsOurWindow(candidate)) {
        return false;
    }

    RECT host = HostScreenRect();
    POINT cursor{};
    GetCursorPos(&cursor);
    if (PtInRect(&host, cursor)) return true;

    RECT window{};
    if (!GetWindowRect(candidate, &window)) return false;
    RECT overlap{};
    if (!IntersectRect(&overlap, &host, &window)) return false;
    long long overlapArea = static_cast<long long>(overlap.right - overlap.left) *
                            (overlap.bottom - overlap.top);
    long long hostArea = static_cast<long long>(host.right - host.left) *
                         (host.bottom - host.top);
    return hostArea > 0 && overlapArea * 5 >= hostArea;
}

void CALLBACK MoveEventCallback(HWINEVENTHOOK, DWORD event, HWND hwnd,
                                LONG objectId, LONG childId, DWORD, DWORD) {
    if (event != EVENT_SYSTEM_MOVESIZEEND || objectId != OBJID_WINDOW ||
        childId != CHILDID_SELF || !hwnd || IsOurWindow(hwnd)) return;
    if (CandidateTouchesHost(hwnd)) {
        PostMessageW(g_main, WM_ATTACH_WINDOW, reinterpret_cast<WPARAM>(hwnd), 0);
    }
}

void EnterAdMode() {
    if (g_adMode) return;
    g_adMode = true;
    ShowWindow(g_start, SW_HIDE);

    LONG_PTR style = GetWindowLongPtrW(g_main, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW);
    style |= WS_POPUP | WS_BORDER | WS_CLIPCHILDREN;
    SetWindowLongPtrW(g_main, GWL_STYLE, style);

    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoW(MonitorFromWindow(g_main, MONITOR_DEFAULTTONEAREST), &monitor);
    constexpr int width = 620;
    constexpr int height = 460;
    int x = monitor.rcWork.right - width - 18;
    int y = monitor.rcWork.bottom - height - 18;
    SetWindowPos(g_main, HWND_TOP, x, y, width, height,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    ShowWindow(g_host, SW_SHOW);
    ShowWindow(g_close, SW_SHOW);
    SetStatus(L"把任意软件窗口拖到这里，松手即可嵌入");

    if (!g_moveHook) {
        g_moveHook = SetWinEventHook(EVENT_SYSTEM_MOVESIZEEND,
                                     EVENT_SYSTEM_MOVESIZEEND, nullptr,
                                     MoveEventCallback, 0, 0,
                                     WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }
    InvalidateRect(g_main, nullptr, TRUE);
}

LRESULT CALLBACK HostProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT client{};
            GetClientRect(hwnd, &client);
            FillSolid(dc, client, RGB(255, 255, 255));
            if (!g_embedded.valid) {
                HPEN pen = CreatePen(PS_DASH, 2, RGB(160, 178, 209));
                HGDIOBJ oldPen = SelectObject(dc, pen);
                HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
                Rectangle(dc, 10, 10, client.right - 10, client.bottom - 10);
                SelectObject(dc, oldBrush);
                SelectObject(dc, oldPen);
                DeleteObject(pen);

                HFONT iconFont = MakeFont(46, FW_BOLD);
                HFONT mainFont = MakeFont(19, FW_BOLD);
                HFONT subFont = MakeFont(13);
                RECT iconRect{0, client.bottom / 2 - 80, client.right,
                              client.bottom / 2 - 22};
                DrawTextAt(dc, L"＋", iconRect, iconFont, kBlue,
                           DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                RECT textRect{30, client.bottom / 2 - 20, client.right - 30,
                              client.bottom / 2 + 18};
                DrawTextAt(dc, L"拖入一个窗口，立即伪装成广告内容", textRect,
                           mainFont, kNavy, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                wchar_t status[300]{};
                GetWindowTextW(hwnd, status, 299);
                RECT subRect{30, client.bottom / 2 + 20, client.right - 30,
                             client.bottom / 2 + 70};
                DrawTextAt(dc, status, subRect, subFont, kMuted,
                           DT_CENTER | DT_WORDBREAK);
                DeleteObject(iconFont);
                DeleteObject(mainFont);
                DeleteObject(subFont);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SIZE:
            ResizeEmbedded();
            return 0;
        case WM_LBUTTONDOWN:
            if (g_embedded.valid) SetFocus(g_embedded.hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void LayoutChildren(HWND hwnd) {
    RECT client{};
    GetClientRect(hwnd, &client);
    if (!g_adMode) {
        SetWindowPos(g_start, nullptr, (client.right - 188) / 2,
                     client.bottom - 94, 188, 48, SWP_NOZORDER);
        return;
    }

    SetWindowPos(g_close, nullptr, client.right - 45, 9, 34, 28, SWP_NOZORDER);
    SetWindowPos(g_detach, nullptr, client.right - 116, 50, 100, 28, SWP_NOZORDER);
    SetWindowPos(g_host, nullptr, 14, 90, client.right - 28,
                 client.bottom - 104, SWP_NOZORDER);
}

void PaintMain(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);

    if (!g_adMode) {
        FillSolid(dc, client, kPale);
        RECT hero{0, 0, client.right, 105};
        FillSolid(dc, hero, kBlue);
        HFONT badge = MakeFont(13, FW_BOLD);
        HFONT title = MakeFont(29, FW_BOLD);
        HFONT body = MakeFont(15);
        RECT badgeRect{24, 17, client.right - 24, 46};
        DrawTextAt(dc, L"摸鱼生产力研究所 · PRO", badgeRect, badge,
                   RGB(222, 233, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT titleRect{20, 44, client.right - 20, 91};
        DrawTextAt(dc, L"摸鱼模拟器最终版", titleRect, title, RGB(255, 255, 255),
                   DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT bodyRect{44, 130, client.right - 44, 220};
        DrawTextAt(dc,
                   L"一键切换为右下角广告弹窗\n把其他软件拖进去，窗口仍可照常操作",
                   bodyRect, body, kNavy, DT_CENTER | DT_WORDBREAK);
        RECT tipRect{40, client.bottom - 127, client.right - 40, client.bottom - 101};
        HFONT tip = MakeFont(12);
        DrawTextAt(dc, L"支持常见桌面软件与视频播放器 · 关闭时自动还原", tipRect,
                   tip, kMuted, DT_CENTER | DT_SINGLELINE);
        DeleteObject(badge);
        DeleteObject(title);
        DeleteObject(body);
        DeleteObject(tip);
    } else {
        FillSolid(dc, client, RGB(239, 243, 250));
        RECT header{0, 0, client.right, 44};
        FillSolid(dc, header, kBlue);
        HFONT mini = MakeFont(11, FW_BOLD);
        HFONT headline = MakeFont(18, FW_BOLD);
        HFONT copy = MakeFont(12);
        RECT brand{15, 0, client.right - 55, 44};
        DrawTextAt(dc, L"今日摸鱼特惠 · 仅剩 00:59:59", brand, mini,
                   RGB(255, 255, 255), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT headlineRect{15, 46, client.right - 130, 76};
        DrawTextAt(dc, L"恭喜！你获得了一份限时快乐", headlineRect, headline,
                   RGB(235, 76, 62), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT copyRect{15, 70, client.right - 130, 89};
        DrawTextAt(dc, L"内容由摸鱼模拟器赞助 · 广告位招租", copyRect, copy,
                   kMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(mini);
        DeleteObject(headline);
        DeleteObject(copy);
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            g_main = hwnd;
            g_start = CreateWindowW(L"BUTTON", L"开始摸鱼", WS_CHILD | WS_VISIBLE |
                                        WS_TABSTOP | BS_PUSHBUTTON,
                                    0, 0, 0, 0, hwnd,
                                    reinterpret_cast<HMENU>(ID_START), g_instance, nullptr);
            g_detach = CreateWindowW(L"BUTTON", L"还原窗口", WS_CHILD | BS_PUSHBUTTON,
                                     0, 0, 0, 0, hwnd,
                                     reinterpret_cast<HMENU>(ID_DETACH), g_instance, nullptr);
            g_close = CreateWindowW(L"BUTTON", L"×", WS_CHILD | BS_PUSHBUTTON,
                                    0, 0, 0, 0, hwnd,
                                    reinterpret_cast<HMENU>(ID_CLOSE), g_instance, nullptr);
            g_host = CreateWindowExW(WS_EX_CLIENTEDGE, kHostClass,
                                     L"把任意软件窗口拖到这里，松手即可嵌入",
                                     WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     0, 0, 0, 0, hwnd, nullptr, g_instance, nullptr);
            HFONT buttonFont = MakeFont(15, FW_BOLD);
            SendMessageW(g_start, WM_SETFONT, reinterpret_cast<WPARAM>(buttonFont), TRUE);
            SendMessageW(g_detach, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            SendMessageW(g_close, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_START:
                    EnterAdMode();
                    return 0;
                case ID_DETACH:
                    DetachEmbedded(true);
                    return 0;
                case ID_CLOSE:
                    SendMessageW(hwnd, WM_CLOSE, 0, 0);
                    return 0;
            }
            break;
        case WM_ATTACH_WINDOW:
            AttachWindow(reinterpret_cast<HWND>(wParam));
            return 0;
        case WM_SIZE:
            LayoutChildren(hwnd);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            PaintMain(hwnd);
            return 0;
        case WM_CLOSE:
            DetachEmbedded(true);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (g_moveHook) {
                UnhookWinEvent(g_moveHook);
                g_moveHook = nullptr;
            }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    g_instance = instance;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW hostClass{};
    hostClass.cbSize = sizeof(hostClass);
    hostClass.style = CS_HREDRAW | CS_VREDRAW;
    hostClass.lpfnWndProc = HostProc;
    hostClass.hInstance = instance;
    hostClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    hostClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    hostClass.lpszClassName = kHostClass;
    if (!RegisterClassExW(&hostClass)) return 1;

    WNDCLASSEXW mainClass{};
    mainClass.cbSize = sizeof(mainClass);
    mainClass.style = CS_HREDRAW | CS_VREDRAW;
    mainClass.lpfnWndProc = MainProc;
    mainClass.hInstance = instance;
    mainClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    mainClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    mainClass.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&mainClass)) return 1;

    constexpr int width = 540;
    constexpr int height = 350;
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    HWND window = CreateWindowExW(
        0, kWindowClass, L"摸鱼模拟器最终版",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        x, y, width, height, nullptr, nullptr, instance, nullptr);
    if (!window) return 1;

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return static_cast<int>(message.wParam);
}
