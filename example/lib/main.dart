import 'package:context_menu/context_menu.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';

void main() {
  runApp(App());
}

class App extends StatelessWidget {
  const App({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        body: Center(
          child: Listener(
            onPointerDown: (d) {
              print(d.buttons == kSecondaryMouseButton);
              ContextMenu.showMenu(
                ShowMenuArgs(
                  Offset(d.position.dx, d.position.dy),
                  [
                    MenuItem(0, 'First item'),
                    MenuItem(1, 'Second item'),
                    MenuItem(2, 'Third item'),
                  ],
                ),
              );
            },
            child: Container(
              padding: const EdgeInsets.symmetric(vertical: 20, horizontal: 40),
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(10),
                color: Colors.black.withAlpha(20),
              ),
              child: Text('Click me'),
            ),
          ),
        ),
      ),
    );
  }
}
