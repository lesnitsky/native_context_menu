import 'dart:async';

import 'package:flutter/services.dart' show MethodChannel;
import 'package:flutter/widgets.dart' show Offset, VoidCallback;

/// Method channel name of the plugin.
const String _kChannelName = 'native_context_menu';

/// Show menu call.
/// Shows context menu at passed position or cursor position.
/// Pass `devicePixelRatio` and `position` from Dart to show menu at specified position.
/// If it is not defined, native code will show the context menu at the cursor's position.
const String _kShowMenu = "showMenu";

/// Called when an item is selected from the context menu.
const String _kOnItemSelected = "onItemSelected";

/// Called when menu is dismissed without clicking any item.
const String _kOnMenuDismissed = "onMenuDismissed";

class MenuItem {
  MenuItem({
    required this.title,
    this.onSelected,
    this.action,
    this.items = const <MenuItem>[],
  });

  late int _id;
  final String title;
  final List<MenuItem> items;
  final Object? action;

  /// Allows a callback to be registered on an item-by-item basis (as opposed to the single
  /// `ContextMenuRegion.onItemSelected` callback that is registered at the higher level.
  /// If this value is not set, the menu item will be disabled. This is the case even for
  /// nested menu items - if you want to disable a whole menu tree, you would set `onSelected`
  /// for the root menu item to `null`. If you want to enable the menu tree, `onSelected`
  /// must be non-`null` - an empty function works just fine. eg. `() {}`.  This function
  /// will be called back if the user explicitly clicks on the root item of the sub-menu.
  final VoidCallback? onSelected;

  bool get hasSubitems => items.isNotEmpty;

  Map<String, dynamic> toJson() {
    return {
      'id': _id,
      'title': title,
      'items': items.map((e) => e.toJson()).toList(),
      'enabled': onSelected != null,
    };
  }
}

class ShowMenuArgs {
  ShowMenuArgs(
    this.devicePixelRatio,
    this.position,
    this.items,
  );

  final double devicePixelRatio;
  final Offset position;
  final List<MenuItem> items;

  Map<String, dynamic> toJson() {
    return {
      'devicePixelRatio': devicePixelRatio,
      'position': <double>[position.dx, position.dy],
      'items': items.map((e) => e.toJson()).toList(),
    };
  }
}

final _channel = const MethodChannel(_kChannelName)
  ..setMethodCallHandler(
    (call) async {
      switch (call.method) {
        case _kOnItemSelected:
          {
            _contextMenuCompleter.complete(call.arguments);
            break;
          }
        case _kOnMenuDismissed:
          {
            _contextMenuCompleter.complete(null);
            break;
          }
        default:
          {
            _contextMenuCompleter.completeError(
              Exception('$_kChannelName: Invalid method call received.'),
            );
          }
      }
    },
  );

Completer<int?> _contextMenuCompleter = Completer<int?>();

int _menuItemId = 0;

Future<MenuItem?> showContextMenu(ShowMenuArgs args) async {
  final menu = _buildMenu(args.items);
  _menuItemId = 0;

  _channel.invokeMethod(_kShowMenu, args.toJson());

  final id = await _contextMenuCompleter.future;
  _contextMenuCompleter = Completer<int?>();

  return menu[id];
}

Map<int, MenuItem> _buildMenu(List<MenuItem> items) {
  final built = <int, MenuItem>{};

  for (var item in items) {
    item._id = _menuItemId++;
    built[item._id] = item;
    if (item.hasSubitems) {
      final submenu = _buildMenu(item.items);
      built.addAll(submenu);
    }
  }

  return built;
}
