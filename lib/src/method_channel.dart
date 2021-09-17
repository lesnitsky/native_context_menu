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
  final Offset position;
  final List<MenuItem> items;

  ShowMenuArgs(this.position, this.items);

  Map<String, dynamic> toJson() {
    return {
      'position': [position.dx, position.dy],
      'items': items.map((e) => e.toJson()).toList(),
    };
  }
}

const _channel = MethodChannel('context_menu');
int _menuItemId = 0;

Future<MenuItem?> showContextMenu(ShowMenuArgs args) async {
  final menu = _buildMenu(args.items);
  _menuItemId = 0;

  final id = await _channel.invokeMethod('showMenu', args.toJson());

  if (id != null) {
    return menu[id];
  }
}

Map<int, MenuItem> _buildMenu(List<MenuItem> items) {
  final built = <int, MenuItem>{};

  items.forEach((item) {
    item._id = _menuItemId++;
    built[item._id] = item;

    if (item.hasSubitems) {
      final submenu = _buildMenu(item.items);
      built.addAll(submenu);
    }
  });

  return built;
}
