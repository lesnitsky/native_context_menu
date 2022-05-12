import 'package:flutter/gestures.dart';
import 'package:flutter/widgets.dart' hide MenuItem;

import 'method_channel.dart';

class ContextMenuRegion extends StatefulWidget {
  const ContextMenuRegion({
    required this.child,
    required this.menuItems,
    Key? key,
    this.onItemSelected,
    this.onDismissed,
    this.menuOffset = Offset.zero,
  }) : super(key: key);

  final Widget child;
  final List<MenuItem> menuItems;
  final Offset menuOffset;
  final void Function(MenuItem item)? onItemSelected;
  final VoidCallback? onDismissed;

  @override
  _ContextMenuRegionState createState() => _ContextMenuRegionState();
}

class _ContextMenuRegionState extends State<ContextMenuRegion> {
  bool shouldReact = false;

  @override
  Widget build(BuildContext context) {
    return Listener(
      onPointerDown: (e) {
        shouldReact = e.kind == PointerDeviceKind.mouse &&
            e.buttons == kSecondaryMouseButton;
      },
      onPointerUp: (e) async {
        if (!shouldReact) return;

        shouldReact = false;

        final position = Offset(
          e.position.dx + widget.menuOffset.dx,
          e.position.dy + widget.menuOffset.dy,
        );

        final selectedItem = await showContextMenu(
          ShowMenuArgs(
            MediaQuery.of(context).devicePixelRatio,
            position,
            widget.menuItems,
          ),
        );

        if (selectedItem != null) {
          widget.onItemSelected?.call(selectedItem);
        } else {
          widget.onDismissed?.call();
        }
      },
      child: widget.child,
    );
  }
}
