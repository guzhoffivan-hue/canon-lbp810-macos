//
// CanonLBP810Progress — menu-bar progress and status monitor for Canon LBP-810 on macOS
//
import Cocoa

let QUEUE = "Canon_LBP810"
let POLL_SECONDS = 1.0

final class ProgressWatcher: NSObject {
    let statusItem = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
    let statusLine = NSMenuItem(title: "Ожидание", action: nil, keyEquivalent: "")
    let printerHeader = NSMenuItem(title: "Canon LBP-810", action: nil, keyEquivalent: "")
    var timer: Timer?
    let testFile: String

    override init() {
        testFile = NSTemporaryDirectory() + "lbp810-progress.test"
        let ipp = """
        { OPERATION Get-Jobs
          GROUP operation-attributes-tag
          ATTR charset attributes-charset utf-8
          ATTR naturalLanguage attributes-natural-language en
          ATTR uri printer-uri $uri
          ATTR keyword which-jobs not-completed
          ATTR keyword requested-attributes job-id,job-name,job-media-sheets-completed,job-impressions
        }
        """
        try? ipp.write(toFile: testFile, atomically: true, encoding: .utf8)
        super.init()

        statusItem.button?.title = "🖨"
        statusItem.isVisible = false // Hidden while idle

        let menu = NSMenu()
        printerHeader.isEnabled = false
        menu.addItem(printerHeader)
        menu.addItem(statusLine)
        menu.addItem(.separator())

        let openQueue = NSMenuItem(title: "Открыть очередь печати…", action: #selector(openPrintQueue), keyEquivalent: "o")
        openQueue.target = self
        menu.addItem(openQueue)

        menu.addItem(.separator())
        let quit = NSMenuItem(title: "Выход", action: #selector(quitApp), keyEquivalent: "q")
        quit.target = self
        menu.addItem(quit)
        statusItem.menu = menu

        timer = Timer.scheduledTimer(timeInterval: POLL_SECONDS, target: self,
                                     selector: #selector(poll), userInfo: nil, repeats: true)
        RunLoop.main.add(timer!, forMode: .common)
        poll()
    }

    @objc func quitApp() {
        NSApp.terminate(nil)
    }

    @objc func openPrintQueue() {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/open")
        task.arguments = ["/System/Library/PreferencePanes/PrintAndScan.prefPane"]
        try? task.run()
    }

    func query() -> (String?, Int?, Int?)? {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/ipptool")
        task.arguments = ["-tv", "ipp://localhost/printers/\(QUEUE)", testFile]
        let pipe = Pipe()
        task.standardOutput = pipe
        task.standardError = Pipe()
        do { try task.run() } catch { return nil }
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        task.waitUntilExit()
        guard let out = String(data: data, encoding: .utf8) else { return nil }

        var haveJob = false
        var name: String? = nil
        var current: Int? = nil
        var total: Int? = nil
        for line in out.split(separator: "\n") {
            guard let eq = line.range(of: " = ") else { continue }
            let value = String(line[eq.upperBound...])
            if line.contains("job-id (integer)") { haveJob = true }
            else if line.contains("job-name (name") { name = value }
            else if line.contains("job-media-sheets-completed (integer)") { current = Int(value) }
            else if line.contains("job-impressions (integer)") { total = Int(value) }
        }
        return haveJob ? (name, current, total) : nil
    }

    @objc func poll() {
        DispatchQueue.global(qos: .background).async {
            let r = self.query()
            DispatchQueue.main.async { self.render(r) }
        }
    }

    func render(_ r: (String?, Int?, Int?)?) {
        guard let (name, current, total) = r else {
            statusItem.isVisible = false // Hide when idle
            statusLine.title = "Ожидание"
            return
        }
        statusItem.isVisible = true // Show in menu bar when printing
        let cur = current ?? 0
        var title: String
        var line: String
        if cur == 0 {
            title = "🖨 Готовится…"
            line = "Подготовка к печати…"
        } else if let t = total, cur <= t {
            title = "🖨 \(cur)/\(t)"
            line = "Печать страницы \(cur) из \(t)"
        } else {
            title = "🖨 Стр. \(cur)"
            line = "Печать страницы \(cur)"
        }
        if let n = name, !n.isEmpty {
            line += " — \(n)"
        }
        statusItem.button?.title = title
        statusLine.title = line
    }
}

let app = NSApplication.shared
app.setActivationPolicy(.accessory) // Accessory: no Dock icon
let watcher = ProgressWatcher()
app.run()
