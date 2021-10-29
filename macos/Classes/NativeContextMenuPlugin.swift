import Cocoa
import FlutterMacOS

public class NativeContextMenuPlugin: NSObject, FlutterPlugin, NSMenuDelegate {
    var contentView: NSView?
    var responded = false
    var channel: FlutterMethodChannel?;
    
    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(
            name: "native_context_menu",
            binaryMessenger: registrar.messenger
        )
        
        let instance = NativeContextMenuPlugin()
        instance.contentView = NSApplication.shared.windows.first?.contentView
        instance.channel = channel;
        
        registrar.addMethodCallDelegate(instance, channel: channel)
    }
    
    func getMenuItemId(_ menuItem: NSMenuItem) -> Int {
        let menuItemData = (menuItem.representedObject as! NSDictionary)
        let id = menuItemData["id"] as! Int;
        return id;
    }
    
    @objc func onItemSelected(_ sender: NSMenuItem) {
        let id = getMenuItemId(sender)
        channel?.invokeMethod("onItemSelected", arguments: id, result: nil)
    }
    
    func onItemDismissed() {
        channel?.invokeMethod("onMenuDismissed", arguments: nil, result: nil)
    }
    
    public func menuDidClose(_ menu: NSMenu) {
        if let selectedItem = menu.highlightedItem {
            if !responded {
                onItemSelected(selectedItem)
                responded = true
            }
        } else if menu.supermenu == nil && !responded {
            onItemDismissed()
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
            
            let x = pos[0]
            var y = pos[1]
            if !contentView!.isFlipped {
                let frameHeight = Double(contentView!.frame.height)
                y = frameHeight - y
            }

            menu.popUp(
                positioning: nil,
                at: NSPoint(x: x, y: y),
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
                action: #selector(onItemSelected(_:)), keyEquivalent: "")

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
