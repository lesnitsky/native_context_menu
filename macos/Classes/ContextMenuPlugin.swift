import Cocoa
import FlutterMacOS

class EventsStreamHandler: NSObject, FlutterStreamHandler {
    var plugin: ContextMenuPlugin?
    
    public func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
        plugin!.sink = events
        return nil
    }
    
    public func onCancel(withArguments arguments: Any?) -> FlutterError? {
        plugin!.sink = nil
        return nil
    }
    
}

public class ContextMenuPlugin: NSObject, FlutterPlugin, NSMenuDelegate {
    var contentView: NSView?
    var sink: FlutterEventSink?
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(
            name: "context_menu",
            binaryMessenger: registrar.messenger
        )
        
        let instance = ContextMenuPlugin()
        registrar.addMethodCallDelegate(instance, channel: channel)
        
        instance.contentView = NSApplication.shared.windows.first?.contentView
        let events = FlutterEventChannel(
            name: "context_menu_events",
            binaryMessenger: registrar.messenger
        )
        
        events.setStreamHandler(EventsStreamHandler())
    }
    
    @objc func onTouched(_ sender: NSMenuItem) {
        sink!(sender.representedObject)
    }
    
    public func menuDidClose(_ menu: NSMenu) {
        sink!(nil)
    }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "showMenu":
            let menu = NSMenu()
            let args = call.arguments as! NSDictionary
            let pos = args["position"] as! [Double]
            let items = args["items"] as! [NSDictionary]
            
            let menuItems = items.map { (item) -> NSMenuItem in
                let menuItem = NSMenuItem(
                    title: item["title"] as! String,
                    action: #selector(onTouched(_:)), keyEquivalent: "")
                
                menuItem.isEnabled = true
                menuItem.representedObject = item["id"] as! Int
                
                return menuItem
            }
            
            menu.delegate = self
            
            menu.popUp(
                positioning: menuItems[0],
                at: NSPoint(x: pos[0], y: pos[1]),
                in: contentView
            )

            result(nil)
        default:
            result(FlutterMethodNotImplemented)
        }
    }
}
