import 'package:context_menu/context_menu.dart';
import 'package:flutter/material.dart';

void main() {
  runApp(App());
}

class App extends StatefulWidget {
  const App({Key? key}) : super(key: key);

  @override
  State<App> createState() => _AppState();
}

class _AppState extends State<App> {
  String? action;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        body: ContextMenuRegion(
          onDismissed: () {
            setState(() {
              action = 'Menu was dismissed';
            });
          },
          onItemSelected: (item) {
            setState(() {
              action = '${item.title} was selected';
            });
          },
          menuItems: [
            MenuItem(
              title: 'First item',
            ),
            MenuItem(
              title: 'Second item',
            ),
            MenuItem(
              title: 'Third item with submenu',
              items: [
                MenuItem(
                  title: 'First subitem',
                ),
                MenuItem(
                  title: 'Second subitem',
                ),
                MenuItem(
                  title: 'Third subitem',
                ),
              ],
            ),
            MenuItem(
              title: 'Fourth item',
            ),
          ],
          child: Card(
            child: Center(
              child: Text(action ?? 'Right click me'),
            ),
          ),
        ),
      ),
    );
  }
}
