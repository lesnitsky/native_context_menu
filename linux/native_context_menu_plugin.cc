#include "include/native_context_menu/native_context_menu_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#define NATIVE_CONTEXT_MENU_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), native_context_menu_plugin_get_type(), \
                              NativeContextMenuPlugin))

// Platform channel name.
constexpr static auto kChannelName = "native_context_menu";
// Show menu call.
// Shows context menu at passed position or cursor position.
// Pass `devicePixelRatio` and `position` from Dart to show menu at specified
// coordinates. If it is not defined, WIN32 will use `GetCursorPos` to show the
// context menu at the cursor's position.
constexpr static auto kShowMenu = "showMenu";

// Called when an item is selected from the context menu.
constexpr static auto kOnItemSelected = "onItemSelected";
// Called when menu is dismissed without clicking any item.
constexpr static auto kOnMenuDismissed = "onMenuDismissed";

NativeContextMenuPlugin* g_plugin;

// Represents a menu item, stores its id, title & possible sub-menu items.
class MenuItem;

class MenuItem {
 public:
  auto id_ptr() const { return &id_; }
  int32_t id() const { return id_; }
  std::string& title() { return title_; }
  std::vector<std::unique_ptr<MenuItem>>& items() { return items_; }

  MenuItem(int32_t id, const char* title) : id_(id), title_(title) {}

 private:
  int32_t id_ = -1;
  std::string title_ = "";
  std::vector<std::unique_ptr<MenuItem>> items_ = {};
};

struct _NativeContextMenuPlugin {
  GObject parent_instance;
  FlPluginRegistrar* registrar;
  FlMethodChannel* channel;
  // Saving `MenuItem` objects in `std::vector` so that they do not get
  // deallocated from memory once the method call is finished (to avoid
  // SEGFAULT).
  // Previously created `MenuItem`s are automatically freed upon each call by
  // calling `std::vector::clear` before each call. Thanks to smart pointers.
  std::vector<std::unique_ptr<MenuItem>> last_menu_items = {};
  bool last_menu_item_selected = false;
  // A separated thread to send "deactivate" signal with `id` as -1 since it is
  // called before "active" on any of menu items. Freed before each subsequent
  // call.
  std::unique_ptr<std::thread> last_menu_thread = nullptr;
};

G_DEFINE_TYPE(NativeContextMenuPlugin, native_context_menu_plugin,
              g_object_get_type())

// Called when a menu item is clicked.
static inline void on_menu_item_clicked(GtkWidget* widget, gpointer data) {
  // Pressed menu item.
  g_plugin->last_menu_item_selected = true;
  auto menu_item = static_cast<MenuItem*>(data);
  fl_method_channel_invoke_method(g_plugin->channel, kOnItemSelected,
                                  fl_value_new_int(menu_item->id()), nullptr,
                                  nullptr, nullptr);
}

// Called when a menu is deactivated.
static inline void on_menu_deactivated(GtkWidget* widget, gpointer) {
  // TODO: Currently using "deactivate" signal to detect menu being dismissed by
  // user.
  // Problem: "deactivate" signal is sent regardless whether menu item was
  // clicked or not (and leads the "activate" event). To notify about "activate"
  // signal first, this thread is put to sleep (so that "activate" event is
  // notified first). A better signal in GTK might be present which can be used
  // to avoid this scenario.
  g_plugin->last_menu_thread.reset(new std::thread([=]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!g_plugin->last_menu_item_selected)
      fl_method_channel_invoke_method(g_plugin->channel, kOnMenuDismissed,
                                      fl_value_new_null(), nullptr, nullptr,
                                      nullptr);
  }));
}

// Gets the parent `GdkWindow` to show the context menu in it.
static inline GdkWindow* get_window(NativeContextMenuPlugin* self) {
  FlView* view = fl_plugin_registrar_get_view(self->registrar);
  if (view == nullptr) return nullptr;

  return gtk_widget_get_window(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

static void native_context_menu_plugin_handle_method_call(
    NativeContextMenuPlugin* self, FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method = fl_method_call_get_name(method_call);
  if (strcmp(method, kShowMenu) == 0) {
    // Clear previously saved object instances.
    self->last_menu_items.clear();
    self->last_menu_item_selected = false;
    if (self->last_menu_thread != nullptr) {
      self->last_menu_thread->detach();
      self->last_menu_thread.reset(nullptr);
    }
    auto arguments = fl_method_call_get_args(method_call);
    auto items = fl_value_lookup_string(arguments, "items");
    auto device_pixel_ratio =
        fl_value_lookup_string(arguments, "devicePixelRatio");
    auto position = fl_value_lookup_string(arguments, "position");
    GdkWindow* window = get_window(self);
    GtkWidget* menu = gtk_menu_new();
    for (int32_t i = 0; i < fl_value_get_length(items); i++) {
      int32_t id = fl_value_get_int(
          fl_value_lookup_string(fl_value_get_list_value(items, i), "id"));
      const char* title = fl_value_get_string(
          fl_value_lookup_string(fl_value_get_list_value(items, i), "title"));
      auto sub_items =
          fl_value_lookup_string(fl_value_get_list_value(items, i), "items");
      self->last_menu_items.emplace_back(std::make_unique<MenuItem>(id, title));
      GtkWidget* menu_item = gtk_menu_item_new_with_label(title);
      // Check for sub-items & create a sub-menu.
      if (fl_value_get_length(sub_items) > 0) {
        GtkWidget* sub_menu = gtk_menu_new();
        for (int32_t j = 0; j < fl_value_get_length(sub_items); j++) {
          int32_t sub_id = fl_value_get_int(fl_value_lookup_string(
              fl_value_get_list_value(sub_items, j), "id"));
          const char* sub_title = fl_value_get_string(fl_value_lookup_string(
              fl_value_get_list_value(sub_items, j), "title"));
          GtkWidget* sub_menu_item = gtk_menu_item_new_with_label(sub_title);
          gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), sub_menu_item);
          gtk_widget_show(sub_menu_item);
          self->last_menu_items.back()->items().emplace_back(
              std::make_unique<MenuItem>(sub_id, sub_title));
          g_signal_connect(
              G_OBJECT(sub_menu_item), "activate",
              G_CALLBACK(on_menu_item_clicked),
              (gpointer)self->last_menu_items.back()->items().at(j).get());
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);
      } else {
        // Avoid "activate" event for the menu item containing a sub-menu.
        g_signal_connect(G_OBJECT(menu_item), "activate",
                         G_CALLBACK(on_menu_item_clicked),
                         (gpointer)self->last_menu_items.back().get());
      }
      gtk_widget_show(menu_item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
    GdkRectangle rectangle;
    // Pass `devicePixelRatio` and `position` from Dart to show menu at
    // specified coordinates. If it is not defined, WIN32 will use
    // `GetCursorPos` to show the context menu at the cursor's position.
    if (device_pixel_ratio != nullptr && position != nullptr) {
      rectangle.x = fl_value_get_float(fl_value_get_list_value(position, 0)) *
                    fl_value_get_float(device_pixel_ratio);
      rectangle.y = fl_value_get_float(fl_value_get_list_value(position, 1)) *
                    fl_value_get_float(device_pixel_ratio);
    } else {
      GdkDevice* mouse_device;
      int x, y;
      // Legacy support.
#if GTK_CHECK_VERSION(3, 20, 0)
      GdkSeat* seat = gdk_display_get_default_seat(gdk_display_get_default());
      mouse_device = gdk_seat_get_pointer(seat);
#else
      GdkDeviceManager* devman =
          gdk_display_get_device_manager(gdk_display_get_default());
      mouse_device = gdk_device_manager_get_client_pointer(devman);
#endif
      gdk_window_get_device_position(window, mouse_device, &x, &y, NULL);
      rectangle.x = x;
      rectangle.y = y;
    }
    g_signal_connect(G_OBJECT(menu), "deactivate",
                     G_CALLBACK(on_menu_deactivated), nullptr);
    // `gtk_menu_popup_at_rect` is used since `gtk_menu_popup_at_pointer` will
    // require event box creation & another callback will be involved. This way
    // is straight forward & easy to work with.
    // NOTE: GDK_GRAVITY_NORTH_WEST is hard-coded by default since no analog is
    // present for it inside the Dart platform channel code (as of now). In
    // summary, this will create a menu whose body is in bottom-right to the
    // position of the mouse pointer.
    gtk_menu_popup_at_rect(GTK_MENU(menu), window, &rectangle,
                           GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_NORTH_WEST,
                           NULL);

    // Responding with `null`, click event & respective `id` of the `MenuItem`
    // is notified through callback. Otherwise the GUI will become unresponsive.
    // To keep the API same, a `Completer` is used in the Dart.
    response =
        FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_null()));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }
  fl_method_call_respond(method_call, response, nullptr);
}

static void native_context_menu_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(native_context_menu_plugin_parent_class)->dispose(object);
}

static void native_context_menu_plugin_class_init(
    NativeContextMenuPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = native_context_menu_plugin_dispose;
}

static void native_context_menu_plugin_init(NativeContextMenuPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  NativeContextMenuPlugin* plugin = NATIVE_CONTEXT_MENU_PLUGIN(user_data);
  native_context_menu_plugin_handle_method_call(plugin, method_call);
}
NativeContextMenuPlugin* native_context_menu_plugin_new(
    FlPluginRegistrar* registrar) {
  NativeContextMenuPlugin* self = NATIVE_CONTEXT_MENU_PLUGIN(
      g_object_new(native_context_menu_plugin_get_type(), nullptr));
  self->registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));
  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  self->channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            kChannelName, FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(self->channel, method_call_cb,
                                            g_object_ref(self), g_object_unref);
  return self;
}

void native_context_menu_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  g_plugin = native_context_menu_plugin_new(registrar);
  g_object_unref(g_plugin);
}
