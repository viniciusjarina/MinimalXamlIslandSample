// WinUICPPXamlIslandSample.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WinUICPPXamlIslandSample.h"

#include <winrt/windows.ui.xaml.hosting.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::UI::Xaml::Controls;

#define MAX_LOADSTRING 100

static winrt::guid m_lastFocusRequestId{};
static std::vector<winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource::TakeFocusRequested_revoker> m_takeFocusEventRevokers{};
static std::vector<winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource> m_xamlSources{ };

const auto static invalidReason = static_cast<winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason>(-1);

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND _hWndXamlIsland;
HWND _hWnd;

winrt::Windows::UI::Xaml::Controls::Button _xamlButton{ nullptr };
winrt::Windows::UI::Xaml::Controls::Button _xamlButton2{ nullptr };
winrt::Windows::UI::Xaml::Controls::TextBox _text{ nullptr };
winrt::Windows::UI::Xaml::Controls::StackPanel _stack{ nullptr };

MenuBar _menu{ nullptr };
MenuBarItem _menuItem1{ nullptr };
MenuBarItem _menuItem2{ nullptr };
MenuFlyoutItem _item1{ nullptr };
MenuFlyoutItem _item2{ nullptr };

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool FilterMessage(const MSG* msg);
bool NavigateFocus(MSG* msg);
bool HandleAccessKeyMessages(const MSG* msg);

bool _isInMenuMode;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINUICPPXAMLISLANDSAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINUICPPXAMLISLANDSAMPLE));

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        bool processedMessage = FilterMessage(&msg);
        if (!processedMessage)
            processedMessage = HandleAccessKeyMessages(&msg);

        if (!processedMessage && !TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg))
        {
            if (!NavigateFocus(&msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINUICPPXAMLISLANDSAMPLE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINUICPPXAMLISLANDSAMPLE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// Xaml Island desktop XAML Source
static winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _desktopWindowXamlSource{ nullptr };
// XAML Manager
static winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager _winxamlmanager{ nullptr };

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    winrt::init_apartment(winrt::apartment_type::single_threaded);

    _winxamlmanager = winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    _desktopWindowXamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource{};

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    _hWnd = hWnd;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

bool FilterMessage(const MSG* msg)
{
    // When multiple child windows are present it is needed to pre dispatch messages to all
    // DesktopWindowXamlSource instances so keyboard accelerators and
    // keyboard focus work correctly.
    for (auto& xamlSource : m_xamlSources)
    {
        BOOL xamlSourceProcessedMessage = FALSE;
        winrt::check_hresult(xamlSource.as<IDesktopWindowXamlSourceNative2>()->PreTranslateMessage(msg, &xamlSourceProcessedMessage));
        if (xamlSourceProcessedMessage != FALSE)
        {
            return true;
        }
    }

    return false;
}

static bool HandleAccessKeyOnIsland(const MSG* message, HWND islandHwnd)
{
    HWND focusWindow = ::GetFocus();
    HWND inputWindow = ::GetWindow(islandHwnd, GW_HWNDNEXT);
    const bool hasFocus = focusWindow == inputWindow;
    const bool isMenuKey = message->wParam == VK_MENU;
    bool handled = false;

    if (message->message == WM_CHAR)
    {
        if (hasFocus || !_isInMenuMode)
        {
            return false;
        }

        auto result = ::SendMessage(inputWindow, message->message, message->wParam, message->lParam);
        if (result == 0)
        {
            return true;
        }

        return false;
    }

    if (isMenuKey)
    {
        auto result = ::SendMessage(inputWindow, message->message, message->wParam, message->lParam);
        if (result == 0)
        {
            handled = true;
        }
    }

    // If the message is WM_KEYDOWN we need to call TranslateMessage to receive WM_CHAR
    if (message->message == WM_KEYDOWN && _isInMenuMode)
    {
        MSG msg = *message;
        msg.hwnd = inputWindow;

        TranslateMessage(&msg);
    }

    return false;
}

bool HandleAccessKeyMessages(const MSG* message)
{
    const bool isValidMessage =
        message->message == WM_KEYDOWN ||
        message->message == WM_KEYUP ||
        message->message == WM_SYSKEYDOWN ||
        message->message == WM_CHAR ||
        message->message == WM_SYSKEYUP;

    if (!isValidMessage)
    {
        return false;
    }

    for (auto& xamlSource : m_xamlSources)
    {
        HWND islandWnd{};
        winrt::check_hresult(xamlSource.as<IDesktopWindowXamlSourceNative>()->get_WindowHandle(&islandWnd));
        
        bool handled = HandleAccessKeyOnIsland(message, islandWnd);
        if (handled)
            return true;
    }

    return false;
}


winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason GetReasonFromKey(WPARAM key)
{
    auto reason = invalidReason;
    if (key == VK_TAB)
    {
        byte keyboardState[256] = {};
        WINRT_VERIFY(::GetKeyboardState(keyboardState));
        reason = (keyboardState[VK_SHIFT] & 0x80) ?
            winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Last :
            winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::First;
    }
    else if (key == VK_LEFT)
    {
        reason = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Left;
    }
    else if (key == VK_RIGHT)
    {
        reason = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Right;
    }
    else if (key == VK_UP)
    {
        reason = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Up;
    }
    else if (key == VK_DOWN)
    {
        reason = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Down;
    }
    return reason;
}

winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource GetNextFocusedIsland(const MSG* msg)
{
    HWND nextElement;

    if (msg->message == WM_KEYDOWN)
    {
        const auto key = msg->wParam;
        auto reason = GetReasonFromKey(key);
        if (reason != invalidReason)
        {
            const BOOL previous =
                (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::First ||
                    reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Down ||
                    reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Right) ? false : true;

            const auto currentFocusedWindow = ::GetFocus();
            nextElement = ::GetNextDlgTabItem(_hWnd, currentFocusedWindow, previous);
            for (auto& xamlSource : m_xamlSources)
            {
                HWND islandWnd{};
                winrt::check_hresult(xamlSource.as<IDesktopWindowXamlSourceNative>()->get_WindowHandle(&islandWnd));
                if (nextElement == islandWnd)
                {
                    return xamlSource;
                }
            }
        }
    }

    return nullptr;
}

winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource GetFocusedIsland()
{
    for (auto& xamlSource : m_xamlSources)
    {
        if (xamlSource.HasFocus())
        {
            return xamlSource;
        }
    }
    return nullptr;
}

bool NavigateFocus(MSG* msg)
{
    if (const auto nextFocusedIsland = GetNextFocusedIsland(msg))
    {
        WINRT_VERIFY(!nextFocusedIsland.HasFocus());
        const auto previousFocusedWindow = ::GetFocus();
        RECT rect = {};
        WINRT_VERIFY(::GetWindowRect(previousFocusedWindow, &rect));
        const auto nativeIsland = nextFocusedIsland.as<IDesktopWindowXamlSourceNative>();
        HWND islandWnd{};
        winrt::check_hresult(nativeIsland->get_WindowHandle(&islandWnd));
        POINT pt = { rect.left, rect.top };
        SIZE size = { rect.right - rect.left, rect.bottom - rect.top };
        ::ScreenToClient(islandWnd, &pt);
        const auto hintRect = winrt::Windows::Foundation::Rect({ static_cast<float>(pt.x), static_cast<float>(pt.y), static_cast<float>(size.cx), static_cast<float>(size.cy) });
        const auto reason = GetReasonFromKey(msg->wParam);
        const auto request = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationRequest(reason, hintRect);
        m_lastFocusRequestId = request.CorrelationId();
        const auto result = nextFocusedIsland.NavigateFocus(request);
        return result.WasFocusMoved();
    }
    else
    {
        const bool islandIsFocused = GetFocusedIsland() != nullptr;
        byte keyboardState[256] = {};
        WINRT_VERIFY(::GetKeyboardState(keyboardState));
        const bool isMenuModifier = keyboardState[VK_MENU] & 0x80;
        if (islandIsFocused && !isMenuModifier)
        {
            return false;
        }
        const bool isDialogMessage = !!IsDialogMessage(_hWnd, msg);
        return isDialogMessage;
    }
}

static const WPARAM invalidKey = (WPARAM)-1;

WPARAM GetKeyFromReason(winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason reason)
{
    auto key = invalidKey;
    if (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Last || reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::First)
    {
        key = VK_TAB;
    }
    else if (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Left)
    {
        key = VK_LEFT;
    }
    else if (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Right)
    {
        key = VK_RIGHT;
    }
    else if (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Up)
    {
        key = VK_UP;
    }
    else if (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Down)
    {
        key = VK_DOWN;
    }
    return key;
}

void OnTakeFocusRequested(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource const& sender, winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args)
{
    if (args.Request().CorrelationId() != m_lastFocusRequestId)
    {
        const auto reason = args.Request().Reason();
        const BOOL previous =
            (reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::First ||
                reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Down ||
                reason == winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Right) ? false : true;

        const auto nativeXamlSource = sender.as<IDesktopWindowXamlSourceNative>();
        HWND senderHwnd = nullptr;
        winrt::check_hresult(nativeXamlSource->get_WindowHandle(&senderHwnd));

        MSG msg = {};
        msg.hwnd = senderHwnd;
        msg.message = WM_KEYDOWN;
        msg.wParam = GetKeyFromReason(reason);
        if (!NavigateFocus(&msg))
        {
            const auto nextElement = ::GetNextDlgTabItem(_hWnd, senderHwnd, previous);
            ::SetFocus(nextElement);
        }
    }
    else
    {
        const auto request = winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationRequest(winrt::Windows::UI::Xaml::Hosting::XamlSourceFocusNavigationReason::Restore);
        m_lastFocusRequestId = request.CorrelationId();
        sender.NavigateFocus(request);
    }
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        HWND hwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"text",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
            280,         // x position 
            80,         // y position 
            100,       // Button width
            50,        // Button height
            hWnd,     // Parent window
            (HMENU)IDOK,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

        winrt::com_ptr<IDesktopWindowXamlSourceNative> interop = _desktopWindowXamlSource.as<IDesktopWindowXamlSourceNative>();
        // Get handle to corewindow
        // Parent the DesktopWindowXamlSource object to current window
        winrt::check_hresult(interop->AttachToWindow(hWnd));
        // This Hwnd will be the window handler for the Xaml Island: A child window that contains Xaml.

        // Get the new child window's hwnd
        interop->get_WindowHandle(&_hWndXamlIsland);

        RECT windowRect;

        ::GetWindowRect(hWnd, &windowRect);
        ::SetWindowPos(_hWndXamlIsland, NULL, 20, 20, 200, 200, SWP_SHOWWINDOW);

        LONG l = ::GetWindowLong(_hWndXamlIsland, GWL_STYLE);
        ::SetWindowLong(_hWndXamlIsland, GWL_STYLE, l | WS_TABSTOP);

        _xamlButton = Button();
        _xamlButton.AccessKey(L"B");
        _xamlButton.Content(winrt::box_value(L"Button"));

        _xamlButton2 = Button();
        _xamlButton2.AccessKey(L"X");
        _xamlButton2.Content(winrt::box_value(L"Button 2"));

        _menu = MenuBar();
        _menuItem1 = MenuBarItem();
        _menuItem1.Title(L"File");
        _menuItem1.ExitDisplayModeOnAccessKeyInvoked(true);
        _menuItem1.AccessKey(L"F");

        _menuItem2 = MenuBarItem();
        _menuItem2.Title(L"Edit");
        _menuItem2.AccessKey(L"E");

        _item1 = MenuFlyoutItem();
        _item1.Text(L"New");
        _item1.AccessKey(L"N");
        _item1.Click([](auto const& /* sender */, RoutedEventArgs const& /* args */)
            {
                OutputDebugString(L"New Click ***\n");
            });

        _item2 = MenuFlyoutItem();
        _item2.Text(L"Open");
        _item2.AccessKey(L"O");
        _item2.Click([](auto const& /* sender */, RoutedEventArgs const& /* args */)
            {
                OutputDebugString(L"Open Click ***\n");
            });

        auto fileItems = _menuItem1.Items();
        fileItems.Append(_item1);
        fileItems.Append(_item2);
        
        auto items = _menu.Items();
        items.Append(_menuItem1);
        items.Append(_menuItem2);

        _xamlButton.Click([](auto const& /* sender */, RoutedEventArgs const& /* args */)
            {
                _xamlButton.Content(box_value(L"Clicked1"));
                OutputDebugString(L"Xaml Button1 clicked ***\n");
            });

        _xamlButton2.Click([](auto const& /* sender */, RoutedEventArgs const& /* args */)
            {
                _xamlButton2.Content(box_value(L"Clicked2"));
                OutputDebugString(L"Xaml Button2 clicked ***\n");
            });

        _text = TextBox();
        _text.Text(L"Text");

        _stack = StackPanel();
        _desktopWindowXamlSource.Content(_stack);

        auto collection = _stack.Children();
        collection.Append(_xamlButton);
        collection.Append(_xamlButton2);
        collection.Append(_menu);


        _xamlButton.AccessKeyDisplayRequested([](winrt::Windows::UI::Xaml::UIElement, winrt::Windows::UI::Xaml::Input::AccessKeyDisplayRequestedEventArgs const& handler)
            {
                _isInMenuMode = true;
            });

        _xamlButton.AccessKeyDisplayDismissed([](winrt::Windows::UI::Xaml::UIElement, winrt::Windows::UI::Xaml::Input::AccessKeyDisplayDismissedEventArgs const& handler)
            {
                _isInMenuMode = false;
            });
        
        _desktopWindowXamlSource.TakeFocusRequested({ &OnTakeFocusRequested });
        m_xamlSources.push_back(_desktopWindowXamlSource);



        hwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Test",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,  // Styles 
            280,         // x position 
            180,         // y position 
            100,       // Button width
            50,        // Button height
            hWnd,     // Parent window
            (HMENU)IDCANCEL,       // No menu.
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.


    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDOK:
            OutputDebugString(L"Win32 button 1 clicked ***\n");
            break;
        case IDCANCEL:
            OutputDebugString(L"Win32 button 2 clicked **");
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        SetDCBrushColor(hdc, RGB(0, 0, 255));
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
