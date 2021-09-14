// You have generated a new plugin project without
// specifying the `--platforms` flag. A plugin project supports no platforms is generated.
// To add platforms, run `flutter create -t plugin --platforms <platforms> .` under the same
// directory. You can also find a detailed instruction on how to add platforms in the `pubspec.yaml` at https://flutter.dev/docs/development/packages-and-plugins/developing-packages#plugin-platforms.

import 'dart:async';
import 'dart:ui';

import 'package:flutter/services.dart';

class MenuItem {
  final int id;
  final String title;

  MenuItem(this.id, this.title);

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'title': title,
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

class ContextMenu {
  static const MethodChannel _channel = MethodChannel('context_menu');

  static Future<void> showMenu(ShowMenuArgs args) async {
    await _channel.invokeMethod('showMenu', args.toJson());
  }
}
