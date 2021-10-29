# native_context_menu

Native context menu for flutter apps

[![lesnitsky.dev](https://lesnitsky.dev/shield.svg?hash=69464)](https://lesnitsky.dev?utm_source=native_context_menu)
[![GitHub stars](https://img.shields.io/github/stars/lesnitsky/native_context_menu.svg?style=social)](https://github.com/lesnitsky/native_context_menu)
[![Twitter Follow](https://img.shields.io/twitter/follow/lesnitsky_dev.svg?label=Follow%20me&style=social)](https://twitter.com/lesnitsky_dev)

![Preview](https://lesnitsky-images.s3.eu-north-1.amazonaws.com/native_context_menu.gif)

## Installation

```bash
flutter pub add native_context_menu
```

## Usage

```dart
import 'package:native_context_menu/native_context_menu.dart';
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
            child: Center(
              child: Text(action ?? 'Right click me'),
            ),
          ),
        ),
      ),
    );
  }
}
```

## Platform support

| Platform | Supported |
| -------- | --------- |
| MacOS    | ✅        |
| Linux    | ✅        |
| Windows  | ✅        |

## License

MIT

---

[![lesnitsky.dev](https://lesnitsky.dev/shield.svg?hash=69464)](https://lesnitsky.dev?utm_source=native_context_menu)
[![GitHub stars](https://img.shields.io/github/stars/lesnitsky/native_context_menu.svg?style=social)](https://github.com/lesnitsky/native_context_menu)
[![Twitter Follow](https://img.shields.io/twitter/follow/lesnitsky_dev.svg?label=Follow%20me&style=social)](https://twitter.com/lesnitsky_dev)
