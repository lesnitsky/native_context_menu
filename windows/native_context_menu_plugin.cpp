#include "include/native_context_menu/native_context_menu_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <codecvt>
#include <future>
#include <map>
#include <memory>

#include <WinUser.h>

// Platform channel name.
constexpr auto kChannelName = "native_context_menu";
// Show menu call.
constexpr auto kShowMenu = "showMenu";

namespace {

class NativeContextMenuPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  NativeContextMenuPlugin(
      flutter::PluginRegistrarWindows* registrar,
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel);

  virtual ~NativeContextMenuPlugin();

 private:
  flutter::PluginRegistrarWindows* registrar_ = nullptr;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_ =
      nullptr;
  HMENU menu_handle_ = nullptr;
  int32_t window_proc_id_ = -1;
  // Windows sends `WM_EXITMENULOOP` message even if a menu item is selected
  // before `WM_COMMAND`. For responding Dart the `id` of the selected menu item
  // before the -1, this thread adds slight delay to `WM_EXITMENULOOP`. Freed
  // after each subsequent call.
  std::unique_ptr<std::thread> last_menu_thread_ = nullptr;
  bool last_menu_item_selected_ = false;
  // For safe conversion between `wchar_t[]` and `char[]`.
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter_;

  std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message,
                                          WPARAM wparam, LPARAM lparam);
  HWND GetWindow();

  void CreateMenu(HMENU menu, flutter::EncodableMap arguments);

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

void NativeContextMenuPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto plugin = std::make_unique<NativeContextMenuPlugin>(
      registrar,
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), kChannelName,
          &flutter::StandardMethodCodec::GetInstance()));
  registrar->AddPlugin(std::move(plugin));
}

NativeContextMenuPlugin::NativeContextMenuPlugin(
    flutter::PluginRegistrarWindows* registrar,
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel)
    : registrar_(registrar), channel_(std::move(channel)) {
  channel_->SetMethodCallHandler([=](const auto& call, auto result) {
    HandleMethodCall(call, std::move(result));
  });
  window_proc_id_ = registrar->RegisterTopLevelWindowProcDelegate(
      [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        return HandleWindowProc(hwnd, message, wparam, lparam);
      });
}

NativeContextMenuPlugin::~NativeContextMenuPlugin() {
  // Delete possibly created thread before exit.
  if (last_menu_thread_ != nullptr) {
    last_menu_thread_->detach();
    last_menu_thread_.reset(nullptr);
  }
  registrar_->UnregisterTopLevelWindowProcDelegate(window_proc_id_);
}

std::optional<LRESULT> NativeContextMenuPlugin::HandleWindowProc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case WM_COMMAND: {
      last_menu_item_selected_ = true;
      channel_->InvokeMethod("id", std::make_unique<flutter::EncodableValue>(
                                       static_cast<int32_t>(wparam)));
      break;
    }
    case WM_EXITMENULOOP: {
      last_menu_thread_ = std::make_unique<std::thread>([=] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!last_menu_item_selected_) {
          channel_->InvokeMethod("id",
                                 std::make_unique<flutter::EncodableValue>(-1));
        }
      });
      break;
    }
    default:
      break;
  }
  return std::nullopt;
}

HWND NativeContextMenuPlugin::GetWindow() {
  return ::GetAncestor(registrar_->GetView()->GetNativeWindow(), GA_ROOT);
}

void NativeContextMenuPlugin::CreateMenu(HMENU menu,
                                         flutter::EncodableMap arguments) {
  auto items = std::get<flutter::EncodableList>(
      arguments[flutter::EncodableValue("items")]);
  int32_t count = ::GetMenuItemCount(menu);
  for (int32_t i = 0; i < count; i++) {
    // Always remove at 0 because they shift every time.
    ::RemoveMenu(menu, 0, MF_BYPOSITION);
  }
  for (const auto& item_value : items) {
    auto item = std::get<flutter::EncodableMap>(item_value);
    int32_t id = std::get<int32_t>(item[flutter::EncodableValue("id")]);
    auto title = std::get<std::string>(item[flutter::EncodableValue("title")]);
    UINT_PTR item_id = id;
    UINT uFlags = MF_STRING;
    auto sub_items = std::get<flutter::EncodableList>(
        item[flutter::EncodableValue("items")]);
    if (sub_items.size() > 0) {
      uFlags |= MF_POPUP;
      HMENU sub_menu = ::CreatePopupMenu();
      CreateMenu(sub_menu, item);
      item_id = reinterpret_cast<UINT_PTR>(sub_menu);
    }
    ::AppendMenuW(menu, uFlags, item_id, converter_.from_bytes(title).c_str());
  }
}

void NativeContextMenuPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare(kShowMenu) == 0) {
    last_menu_item_selected_ = false;
    if (last_menu_thread_ != nullptr) {
      last_menu_thread_->detach();
      last_menu_thread_.reset(nullptr);
    }
    auto arguments = std::get<flutter::EncodableMap>(*method_call.arguments());
    menu_handle_ = CreatePopupMenu();
    CreateMenu(menu_handle_, arguments);
    HWND window = GetWindow();
    RECT rect;
    ::GetWindowRect(window, &rect);
// Define USE_FLUTTER_POSITION to use `devicePixelRatio` and `position` from
// Dart. If it is not defined, WIN32 will use `GetCursorPos` to show the context
// menu at the mouse's position.
#ifdef USE_FLUTTER_POSITION
    auto device_pixel_patio = std::get<double>(
        arguments[flutter::EncodableValue("devicePixelRatio")]);
    auto position = std::get<flutter::EncodableList>(
        arguments[flutter::EncodableValue("position")]);
    TITLEBARINFOEX title_bar_info;
    title_bar_info.cbSize = sizeof(TITLEBARINFOEX);
    ::SendMessage(window, WM_GETTITLEBARINFOEX, 0, (LPARAM)&title_bar_info);
    int32_t title_bar_height =
        title_bar_info.rcTitleBar.bottom - title_bar_info.rcTitleBar.top;
    int32_t x = static_cast<int32_t>(
        (std::get<double>(position[0]) * device_pixel_patio) + rect.left);
    int32_t y = static_cast<int32_t>(
        (std::get<double>(position[1]) * device_pixel_patio) + rect.top +
        title_bar_height);
    ::TrackPopupMenu(menu_handle_, TPM_LEFTALIGN | TPM_LEFTBUTTON, x, y, 0,
                     window, NULL);
#else
    POINT point;
    ::GetCursorPos(&point);
    ::TrackPopupMenu(menu_handle_, TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,
                     point.y, 0, window, NULL);
#endif
    result->Success(nullptr);
  } else {
    result->NotImplemented();
  }
}

}  // namespace

void NativeContextMenuPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  NativeContextMenuPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
