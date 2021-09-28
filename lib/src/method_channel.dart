import 'dart:async';
import 'dart:io';

import 'package:flutter/services.dart' show MethodChannel, EventChannel;
import 'package:flutter/widgets.dart' show Offset, VoidCallback;

class MenuItem {
  late int _id;
  final String title;
  final List<MenuItem> items;
  final Object? action;

  final VoidCallback? onSelected;

  bool get hasSubitems => items.isNotEmpty;

  MenuItem({
    required this.title,
    this.onSelected,
    this.action,
    this.items = const <MenuItem>[],
  });

  Map<String, dynamic> toJson() {
    return {
      'id': _id,
      'title': title,
      'items': items.map((e) => e.toJson()).toList(),
    };
  }
}

class ShowMenuArgs {
  final double devicePixelRatio;
  final Offset position;
  final List<MenuItem> items;

  ShowMenuArgs(this.devicePixelRatio, this.position, this.items);

  Map<String, dynamic> toJson() {
    return {
      'devicePixelRatio': devicePixelRatio,
      'position': [position.dx, position.dy],
      'items': items.map((e) => e.toJson()).toList(),
    };
  }
}

final _channel = const MethodChannel('native_context_menu')
  ..setMethodCallHandler(
    (call) async {
      switch (call.method) {
        case 'id':
          {
            // Notify about the selected menu item's id & complete the future.
            _completer.complete(call.arguments);
            break;
          }
        default:
          break;
      }
    },
  );
Completer<int> _completer = Completer<int>();
int _menuItemId = 0;

Future<MenuItem?> showContextMenu(ShowMenuArgs args) async {
  final menu = _buildMenu(args.items);
  _menuItemId = 0;
  _channel.invokeMethod('showMenu', args.toJson());
  int id = await _completer.future;
  _completer = Completer<int>();
  if (id != -1) {
    return menu[id];
  }
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
