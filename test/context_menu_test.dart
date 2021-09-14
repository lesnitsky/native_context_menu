import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:context_menu/context_menu.dart';

void main() {
  const MethodChannel channel = MethodChannel('context_menu');

  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() {
    channel.setMockMethodCallHandler((MethodCall methodCall) async {
      return '42';
    });
  });

  tearDown(() {
    channel.setMockMethodCallHandler(null);
  });

  test('getPlatformVersion', () async {
    expect(await ContextMenu.platformVersion, '42');
  });
}
