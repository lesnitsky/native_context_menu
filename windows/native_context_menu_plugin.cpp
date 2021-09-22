#include "include/native_context_menu/native_context_menu_plugin.h"

// This must be included before many other Windows headers.
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <windows.h>

#include <codecvt>
#include <future>
#include <map>
#include <memory>
#include <sstream>

namespace
{
std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> last_show_menu_result;
bool last_show_menu_responded = false;

class NativeContextMenuPlugin : public flutter::Plugin
{
  public:
    static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

    NativeContextMenuPlugin(flutter::PluginRegistrarWindows *registrar);

    virtual ~NativeContextMenuPlugin();

  private:
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> g_converter;

    flutter::PluginRegistrarWindows *registrar;
    // The ID of the WindowProc delegate registration.
    int window_proc_id = -1;

    HMENU hMenu;

    // Called for top-level WindowProc delegation.
    std::optional<LRESULT> NativeContextMenuPlugin::HandleWindowProc(HWND hwnd, UINT message, WPARAM wparam,
                                                                     LPARAM lparam);
    HWND NativeContextMenuPlugin::GetMainWindow();
    void NativeContextMenuPlugin::CreateMenu(HMENU menu, flutter::EncodableMap args);
    void NativeContextMenuPlugin::ShowMenu(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                                           std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

    // Called when a method is called on this plugin's channel from Dart.
    void HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                          std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

// static
void NativeContextMenuPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar)
{
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
        registrar->messenger(), "native_context_menu", &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<NativeContextMenuPlugin>(registrar);

    channel->SetMethodCallHandler([plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
    });

    registrar->AddPlugin(std::move(plugin));
}

NativeContextMenuPlugin::NativeContextMenuPlugin(flutter::PluginRegistrarWindows *registrar) : registrar(registrar)
{
    window_proc_id =
        registrar->RegisterTopLevelWindowProcDelegate([this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
            return HandleWindowProc(hwnd, message, wparam, lparam);
        });
}

NativeContextMenuPlugin::~NativeContextMenuPlugin()
{
    registrar->UnregisterTopLevelWindowProcDelegate(window_proc_id);
}

void HandlePopupMenuClickItem(const int item_id)
{
    if (!last_show_menu_responded)
    {
        last_show_menu_responded = true;
        last_show_menu_result->Success(flutter::EncodableValue(item_id));
    }
}

void HandlePopupMenuDismissed(const int item_id)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!last_show_menu_responded)
    {
        last_show_menu_responded = true;
        last_show_menu_result->Success(flutter::EncodableValue(item_id));
    }
}

std::optional<LRESULT> NativeContextMenuPlugin::HandleWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::optional<LRESULT> result;
    if (message == WM_COMMAND)
    {
        int item_id = (int)wParam;
        std::thread th(HandlePopupMenuClickItem, item_id);
        th.detach();
    }
    else if (message == WM_EXITMENULOOP)
    {
        int item_id = -1;
        std::thread th(HandlePopupMenuDismissed, item_id);
        th.detach();
    }
    return result;
}

HWND NativeContextMenuPlugin::GetMainWindow()
{
    return ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
}

void NativeContextMenuPlugin::CreateMenu(HMENU menu, flutter::EncodableMap args)
{
    flutter::EncodableList items = std::get<flutter::EncodableList>(args.at(flutter::EncodableValue("items")));

    int count = GetMenuItemCount(menu);
    for (int i = 0; i < count; i++)
    {
        // always remove at 0 because they shift every time
        RemoveMenu(menu, 0, MF_BYPOSITION);
    }

    for (flutter::EncodableValue item_value : items)
    {
        flutter::EncodableMap item_map = std::get<flutter::EncodableMap>(item_value);
        int id = std::get<int>(item_map.at(flutter::EncodableValue("id")));
        std::string title = std::get<std::string>(item_map.at(flutter::EncodableValue("title")));

        UINT_PTR item_id = id;
        UINT uFlags = MF_STRING;

        flutter::EncodableList sub_items =
            std::get<flutter::EncodableList>(item_map.at(flutter::EncodableValue("items")));

        if (sub_items.size() > 0)
        {
            uFlags |= MF_POPUP;
            HMENU sub_menu = ::CreatePopupMenu();
            CreateMenu(sub_menu, item_map);
            item_id = reinterpret_cast<UINT_PTR>(sub_menu);
        }

        AppendMenuW(menu, uFlags, item_id, g_converter.from_bytes(title).c_str());
    }
}

void NativeContextMenuPlugin::ShowMenu(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                                       std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{
    const flutter::EncodableMap &args = std::get<flutter::EncodableMap>(*method_call.arguments());

    double device_pixel_patio = std::get<double>(args.at(flutter::EncodableValue("devicePixelRatio")));
    const flutter::EncodableList pos = std::get<flutter::EncodableList>(args.at(flutter::EncodableValue("position")));

    hMenu = CreatePopupMenu();
    CreateMenu(hMenu, args);

    HWND hWnd = GetMainWindow();

    RECT rect;
    GetWindowRect(hWnd, &rect);

    TITLEBARINFOEX *ptinfo = (TITLEBARINFOEX *)malloc(sizeof(TITLEBARINFOEX));
    ptinfo->cbSize = sizeof(TITLEBARINFOEX);
    SendMessage(hWnd, WM_GETTITLEBARINFOEX, 0, (LPARAM)ptinfo);
    int titleBarHeight = ptinfo->rcTitleBar.bottom - ptinfo->rcTitleBar.top;

    int x = static_cast<int>((std::get<double>(pos[0]) * device_pixel_patio) + rect.left);
    int y = static_cast<int>((std::get<double>(pos[1]) * device_pixel_patio) + rect.top + titleBarHeight);

    last_show_menu_result = std::move(result);
    last_show_menu_responded = false;

    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, static_cast<int>(x), static_cast<int>(y), 0, hWnd, NULL);
}

void NativeContextMenuPlugin::HandleMethodCall(const flutter::MethodCall<flutter::EncodableValue> &method_call,
                                               std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{
    if (method_call.method_name().compare("showMenu") == 0)
    {
        ShowMenu(method_call, std::move(result));
    }
    else
    {
        result->NotImplemented();
    }
}

} // namespace

void NativeContextMenuPluginRegisterWithRegistrar(FlutterDesktopPluginRegistrarRef registrar)
{
    NativeContextMenuPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarManager::GetInstance()->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
