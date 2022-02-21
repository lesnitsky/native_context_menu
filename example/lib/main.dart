import 'package:flutter/material.dart';
import 'package:native_context_menu/native_context_menu.dart';
import 'package:window_manager/window_manager.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();

  runApp(const App());
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
          onDismissed: () => setState(() => action = 'Menu was dismissed'),
          onItemSelected: (item) => setState(() {
            action = '${item.title} was selected';
          }),
          menuItems: [
            MenuItem(title: 'First item'),
            MenuItem(title: 'Second item'),
            MenuItem(
              title: 'Third item with submenu',
              items: [
                MenuItem(title: 'First subitem'),
                MenuItem(title: 'Second subitem'),
                MenuItem(title: 'Third subitem'),
              ],
            ),
            MenuItem(title: 'Fourth item'),
          ],
          child: Card(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.center,
              mainAxisAlignment: MainAxisAlignment.center,
              mainAxisSize: MainAxisSize.max,
              children: [
                DragToMoveArea(
                  child: Container(
                    width: 300,
                    height: 52,
                    alignment: Alignment.center,
                    margin: const EdgeInsets.all(20),
                    color: Colors.grey.withOpacity(0.3),
                    child: const Text('Drag to move window'),
                  ),
                ),
                Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Text('TitleBarStyle: '),
                    TextButton(
                      onPressed: () {
                        windowManager.setTitleBarStyle('default');
                      },
                      child: const Text('default'),
                    ),
                    TextButton(
                      onPressed: () {
                        windowManager.setTitleBarStyle('hidden');
                      },
                      child: const Text('hidden'),
                    ),
                  ],
                ),
                Expanded(
                  child: Center(
                    child: Text(action ?? 'Right click me'),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
