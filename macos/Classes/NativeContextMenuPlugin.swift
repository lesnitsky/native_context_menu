import Cocoa
import FlutterMacOS

public class NativeContextMenuPlugin: NSObject, FlutterPlugin, NSMenuDelegate {
    var contentView: NSView?
    var result: FlutterResult?
    var responded = false
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(
            name: "native_context_menu",
            binaryMessenger: registrar.messenger
        )
        
        let instance = NativeContextMenuPlugin()
        instance.contentView = NSApplication.shared.windows.first?.contentView
        
        registrar.addMethodCallDelegate(instance, channel: channel)
    }
    
    @objc func onTouched(_ sender: NSMenuItem) {
        result!((sender.representedObject as! NSDictionary)["id"])
    }
    
    public func menuDidClose(_ menu: NSMenu) {
        if let selectedItem = menu.highlightedItem {
            if !responded {
                result!((selectedItem.representedObject as! NSDictionary)["id"])
                responded = true
            }
        } else if menu.supermenu == nil && !responded {
            result!(nil)
            responded = true
        }
    }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "showMenu":
            responded = false
            let args = call.arguments as! NSDictionary
            let pos = args["position"] as! [Double]
            let items = args["items"] as! [NSDictionary]
            
            let menu = createMenu(items)
            
            self.result = result
            
            menu.popUp(
                positioning: nil,
                at: NSPoint(x: pos[0], y: pos[1]),
                in: contentView
            )
        default:
            result(FlutterMethodNotImplemented)
        }
    }
    
    func createMenu(_ items: [NSDictionary]) -> NSMenu {
        let menuItems = items.map { (item) -> NSMenuItem in
            let menuItem = NSMenuItem(
                title: item["title"] as! String,
                action: #selector(onTouched(_:)), keyEquivalent: "")

            menuItem.representedObject = item
            
            return menuItem
        }
        
        let menu = NSMenu()

        menu.autoenablesItems = false
        menu.delegate = self
        
        menuItems.forEach { (item) -> Void in
            menu.addItem(item)
            
            let children = (item.representedObject as! NSDictionary)["items"] as! [NSDictionary]
            
            if !children.isEmpty {
                let submenu = createMenu(children)
                menu.setSubmenu(submenu, for: item)
            }
        }
        
        return menu
    }
}
