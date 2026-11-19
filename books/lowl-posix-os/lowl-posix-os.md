# LOWL POSIX OS

**By Anthony Matarazzo (c) 2026**

*175-page technical reference, translated to Markdown from `LOWL POSIX OS - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

## License

```text
MIT License

Copyright (c) 2026 Anthony Matarazzo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Table of Contents

- [Part 1: LOWL Boot Controller (nebula_boot.lowl)](#part-1-lowl-boot-controller-nebulabootlowl)
- [Part 1: LOWL Boot Controller (nebula_boot.lowl)](#part-1-lowl-boot-controller-nebulabootlowl-1)
- [Part 2: LOWLUX Document Object Model (lowlux/document.lowl)](#part-2-lowlux-document-object-model-lowluxdocumentlowl)
- [Part 3: LOWLUX Container System (lowlux/container.lowl)](#part-3-lowlux-container-system-lowluxcontainerlowl)
- [Part 4: LOWLUX Desktop Manager (lowlux/desktop.lowl)](#part-4-lowlux-desktop-manager-lowluxdesktoplowl)
- [Part 1: System Overview](#part-1-system-overview)
- [Part 2: Process Control Block (PCB)](#part-2-process-control-block-pcb)
- [Part 3: Scheduler Core](#part-3-scheduler-core)
- [Part 4: Process Management](#part-4-process-management)
- [Part 5: Memory Management](#part-5-memory-management)
- [Part 6: File System](#part-6-file-system)
- [Part 7: System Calls](#part-7-system-calls)
- [Part 8: Context Switching](#part-8-context-switching)
- [Part 9: Signal Handling](#part-9-signal-handling)
- [Part 10: Complete Boot and Initialization](#part-10-complete-boot-and-initialization)
- [Part 11: Complete Example - Userspace Program](#part-11-complete-example---userspace-program)
- [Part 12: Build Configuration](#part-12-build-configuration)

---

NEBULA OS - Complete LOWL Script Implementation

Pure LOWL Script System Architecture

```text
The entire NEBULA OS is implemented in LOWL script, with no C++ components. The system consists of the POSIX kernel (written in lowlPython), the CPPUX Document Object Model for the desktop, and the boot controller that orchestrates everything. All objects are created, managed, and executed within the LOWL JIT environment.
```

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                    NEBULA OS - PURE LOWL SCRIPT ARCHITECTURE                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    LOWL PLATFORM BOOT CONTROLLER                      │    │
│  │                    (nebula_boot.lowl)                                 │    │
│  │  - JIT compilation                                                   │    │
│  │  - Script execution                                                  │    │
│  │  - Configuration loading                                             │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    POSIX KERNEL (lowlPython)                          │    │
│  │  - Process management (fork, exec, wait, exit)                       │    │
│  │  - Memory management (mmap, brk, demand paging)                      │    │
│  │  - Scheduling (round-robin, priority queues)                         │    │
│  │  - File system (inodes, directories, file descriptors)               │    │
│  │  - System calls (60+ POSIX syscalls)                                 │    │
│  │  - Signal handling                                                   │    │
│  │  - IPC (pipes, message queues, shared memory)                        │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    CPPUX DOCUMENT OBJECT MODEL                        │    │
│  │  - Document (root UI container)                                      │    │
│  │  - Element (base visual class)                                       │    │
│  │  - VisualContainer (window container)                                │    │
│  │  - ShapeContainer (polygon, oval, rectangle)                         │    │
│  │  - LayoutEngine (CSS box model)                                      │    │
│  │  - StyleResolver (style cascade)                                     │    │
│  │  - RenderBackend (software rasterizer)                               │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    DESKTOP ENVIRONMENT                                │    │
│  │  - DesktopManager                                                    │    │
│  │  - Dock                                                              │    │
│  │  - StartMenu                                                         │    │
│  │  - SystemTray                                                        │    │
│  │  - NotificationCenter                                                │    │
│  │  - Workspace                                                         │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Part 1: LOWL Boot Controller (nebula_boot.lowl)

```lua
lowl_script nebula_boot version 5.0.0

import lowl.system
import lowl.system.hardware
import lowl.system.power
import lowl.security
import lowl.security.audit
import lowl.security.keyring
import lowl.security.tpm
import lowl.jit.JITCompiler
import lowl.jit.MultiLanguageJIT
import lowl.filesystem.File
import lowl.filesystem.Directory
import lowl.database.MapDB

// Import POSIX kernel (lowlPython)
import posix_kernel
// Import CPPUX DOM components
rename cppux to lowlux
```

NEBULA OS - Complete LOWL Script Implementation with LOWLUX

Pure LOWL Script System Architecture

```lua
The entire NEBULA OS is implemented in LOWL script, with no C++ components. The system consists of the POSIX kernel (written in lowlPython), the LOWLUX Document Object Model for the desktop, and the boot controller that orchestrates everything. All objects are created, managed, and executed within the LOWL JIT environment.
```

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                    NEBULA OS - PURE LOWL SCRIPT ARCHITECTURE                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    LOWL PLATFORM BOOT CONTROLLER                      │    │
│  │                    (nebula_boot.lowl)                                 │    │
│  │  - JIT compilation                                                   │    │
│  │  - Script execution                                                  │    │
│  │  - Configuration loading                                             │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    POSIX KERNEL (lowlPython)                          │    │
│  │  - Process management (fork, exec, wait, exit)                       │    │
│  │  - Memory management (mmap, brk, demand paging)                      │    │
│  │  - Scheduling (round-robin, priority queues)                         │    │
│  │  - File system (inodes, directories, file descriptors)               │    │
│  │  - System calls (60+ POSIX syscalls)                                 │    │
│  │  - Signal handling                                                   │    │
│  │  - IPC (pipes, message queues, shared memory)                        │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    LOWLUX DOCUMENT OBJECT MODEL                       │    │
│  │  - Document (root UI container)                                      │    │
│  │  - Element (base visual class)                                       │    │
│  │  - VisualContainer (window container)                                │    │
│  │  - ShapeContainer (polygon, oval, rectangle)                         │    │
│  │  - LayoutEngine (CSS box model)                                      │    │
│  │  - StyleResolver (style cascade)                                     │    │
│  │  - RenderBackend (software rasterizer)                               │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                      │                                       │
│                                      ▼                                       │
```

│

```text
┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    DESKTOP ENVIRONMENT                                │    │
│  │  - DesktopManager                                                    │    │
│  │  - Dock                                                              │    │
│  │  - StartMenu                                                         │    │
│  │  - SystemTray                                                        │    │
│  │  - NotificationCenter                                                │    │
│  │  - Workspace                                                         │    │
```

│

```text
└─────────────────────────────────────────────────────────────────────┘    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Part 1: LOWL Boot Controller (nebula_boot.lowl)

```lua
lowl_script nebula_boot version 5.0.0

import lowl.system
import lowl.system.hardware
import lowl.system.power
import lowl.security
import lowl.security.audit
import lowl.security.keyring
import lowl.security.tpm
import lowl.jit.JITCompiler
import lowl.jit.MultiLanguageJIT
import lowl.filesystem.File
import lowl.filesystem.Directory
import lowl.database.MapDB

// Import POSIX kernel (lowlPython)
import posix_kernel

// Import LOWLUX DOM components
import lowlux.document.Document
import lowlux.element.Element
import lowlux.container.VisualContainer
import lowlux.container.ShapeContainer
import lowlux.layout.LayoutEngine
import lowlux.style.StyleResolver
import lowlux.style.ComputedStyle
import lowlux.render.RenderBackend
import lowlux.render.SoftwareBackend
import lowlux.desktop.DesktopManager
import lowlux.desktop.Dock
import lowlux.desktop.StartMenu
import lowlux.desktop.SystemTray
import lowlux.desktop.NotificationCenter
import lowlux.desktop.Workspace
import lowlux.animation.Animation
import lowlux.animation.Tween
import lowlux.graphics.Canvas
import lowlux.graphics.Texture
import lowlux.graphics.Shader

namespace nebula

class NebulaPlatform
    property name = "NEBULA OS Platform"
    property version = "5.0.0"
    property boot_time = 0
    property kernel_type = "posix"
    property desktop_enabled = true
    property security_level = "standard"
    property boot_mode = "normal"

    // Core LOWL components
    property jit_compiler = null
    property script_engine = null
    property config = null
    // POSIX kernel
    property posix_kernel = null
    property windows_driver_adapter = null

    // LOWLUX Desktop components
    property desktop_document = null
    property desktop_root = null
    property desktop_manager = null
    property layout_engine = null
    property style_resolver = null
    property render_backend = null

    // UI components
    property dock = null
    property start_menu = null
    property system_tray = null
    property notification_center = null
    property workspaces = {}
    property current_workspace = 0

    // System services
    property service_manager = null
    property power_manager = null
    property network_manager = null
    property audio_manager = null
    property input_manager = null
    property security_manager = null

    method initialize()
        self.boot_time = lowl.system.get_time_ms()
        lowl.system.console.log("NEBULA OS Platform v" + self.version)
```

```text
        lowl.system.console.log("================================")
```

```lua
        self.load_configuration()
        self.initialize_security()
        self.initialize_hardware()
        self.initialize_kernel_layer()
        self.initialize_service_manager()
        self.initialize_lowlux_desktop()
        self.initialize_ui_components()
        self.start_desktop()
        self.start_system_services()

        let boot_time_ms = lowl.system.get_time_ms() - self.boot_time
        lowl.system.console.log("Boot completed in " + boot_time_ms + " ms")
```

```end
    method load_configuration()
        self.config = lowl.system.config.load("/boot/nebula.lowl")

        if self.config.has_key("kernel_type")
            self.kernel_type = self.config.get("kernel_type")
```

```end
        if self.config.has_key("desktop_enabled")
            self.desktop_enabled = self.config.get("desktop_enabled")
```

```end
        if self.config.has_key("security_level")
            self.security_level = self.config.get("security_level")
```

```end
        if self.config.has_key("boot_mode")
            self.boot_mode = self.config.get("boot_mode")
```

```end
        lowl.system.console.log("Configuration loaded:")
        lowl.system.console.log("  Kernel: " + self.kernel_type)
        lowl.system.console.log("  Security: " + self.security_level)
        lowl.system.console.log("  Mode: " + self.boot_mode)
```

```end
    method initialize_security()
        lowl.system.console.log("Initializing security subsystem...")

        self.security_manager = lowl.security.SecurityManager.new()

        if lowl.hardware.tpm.is_available()
            let tpm = lowl.security.tpm.TPM.get_instance()
            tpm.initialize()
            lowl.system.console.log("  TPM " + tpm.get_version() + " initialized")
```

```end
        self.audit = lowl.security.audit.Audit.get_instance()
        self.audit.start()
        self.audit.log("System boot started", "info")

        self.security_manager.set_policy(self.security_level)
        self.security_manager.enable_secure_boot()
        self.security_manager.load_security_profiles()
        self.keyring = lowl.security.keyring.Keyring.get_instance()
        if lowl.filesystem.File.exists("/system/keys/master.key")
            self.keyring.load("/system/keys/master.key")
```

```else
            self.keyring.generate_master_key()
            self.keyring.save("/system/keys/master.key")
```

```end
        lowl.system.console.log("  Security level: " + self.security_level)
        self.audit.log("Security subsystem initialized", "info")
```

```end
    method initialize_hardware()
        lowl.system.console.log("Detecting hardware...")

        let cpu = lowl.hardware.cpu.get_info()
        lowl.system.console.log("  CPU: " + cpu.vendor + " " + cpu.model)
        lowl.system.console.log("    Cores: " + cpu.cores + ", Threads: " + cpu.threads)
        lowl.system.console.log("    Frequency: " + cpu.frequency_mhz + " MHz")

        let memory = lowl.hardware.memory.get_info()
        lowl.system.console.log("  Memory: " + memory.total_gb + " GB (" + memory.type + ")")

        let storage = lowl.hardware.storage.get_devices()
```

for each device in storage

```end
            lowl.system.console.log("  Storage: " + device.model)
            lowl.system.console.log("    Capacity: " + device.capacity_gb + " GB")
            lowl.system.console.log("    Type: " + device.interface_type)
```

```end
        let gpu = lowl.hardware.gpu.get_devices()
```

for each device in gpu

```end
            lowl.system.console.log("  GPU: " + device.vendor + " " + device.model)
            lowl.system.console.log("    VRAM: " + device.vram_mb + " MB")
```

```end
        let network = lowl.hardware.network.get_adapters()
```

for each adapter in network

```end
            lowl.system.console.log("  Network: " + adapter.name + " (" + adapter.type + ")")
```

```end
        self.audit.log("Hardware detection complete", "info")
```

```end
    method initialize_kernel_layer()
        lowl.system.console.log("Initializing kernel layer...")

        if self.kernel_type == "posix"
            lowl.system.console.log("  Loading POSIX kernel (lowlPython)...")
            self.posix_kernel = posix_kernel.POSIXKernel.new()
            self.posix_kernel.initialize()
            lowl.system.console.log("  POSIX kernel loaded successfully")
```

```end
        self.windows_driver_adapter = lowl.kernel.WindowsDriverAdapter.new()
        self.windows_driver_adapter.initialize()

        lowl.system.console.log("Kernel layer initialized")
        self.audit.log("Kernel layer initialized: " + self.kernel_type, "info")
```

```end
    method initialize_service_manager()
        lowl.system.console.log("Initializing system services...")

        self.service_manager = lowl.system.ServiceManager.new()
        self.power_manager = lowl.system.power.PowerManager.new()
        self.network_manager = lowl.network.NetworkManager.new()
        self.audio_manager = lowl.audio.AudioManager.new()
        self.input_manager = lowl.input.InputManager.new()

        self.service_manager.register("power", self.power_manager)
        self.service_manager.register("network", self.network_manager)
        self.service_manager.register("audio", self.audio_manager)
        self.service_manager.register("input", self.input_manager)

        self.network_manager.initialize()
        self.audio_manager.initialize()
        self.input_manager.initialize()

        lowl.system.console.log("  Registered " + self.service_manager.get_service_count() + " services")
        self.audit.log("System services initialized", "info")
```

```end
    //
```

```text
========================================================================
```

```end
    // LOWLUX DESKTOP INITIALIZATION
    //
```

```text
========================================================================
```

```end
    method initialize_lowlux_desktop()
        lowl.system.console.log("Initializing LOWLUX desktop environment...")
        self.render_backend = lowlux.render.SoftwareBackend.new()
        self.render_backend.initialize(1920, 1080)

        self.layout_engine = lowlux.layout.LayoutEngine.new()
        self.style_resolver = lowlux.style.StyleResolver.new()

        self.desktop_document = lowlux.document.Document.new()
        self.desktop_document.set_title("NEBULA OS Desktop")
        self.desktop_document.set_viewport(viewport.device_width, 1.0)

        self.desktop_root = self.desktop_document.get_document_element()
        self.desktop_root.set_style("display", "flex")
        self.desktop_root.set_style("flex_direction", "column")
        self.desktop_root.set_style("height", "100vh")
        self.desktop_root.set_style("width", "100vw")
        self.desktop_root.set_style("background", "linear_gradient(135deg, #1a1a2e, #16213e)")
        self.desktop_root.set_style("margin", "0")
        self.desktop_root.set_style("padding", "0")
        self.desktop_root.set_style("overflow", "hidden")
        self.desktop_root.set_style("font_family", "system-ui, sans-serif")

        let wallpaper = self.desktop_document.create_element("div")
        wallpaper.set_style("position", "absolute")
        wallpaper.set_style("top", "0")
        wallpaper.set_style("left", "0")
        wallpaper.set_style("width", "100%")
        wallpaper.set_style("height", "100%")
        wallpaper.set_style("background_image", "url(/system/wallpapers/nebula_default.jpg)")
        wallpaper.set_style("background_size", "cover")
        wallpaper.set_style("background_position", "center")
        wallpaper.set_style("z_index", "0")
        self.desktop_root.append_child(wallpaper)

        let overlay = self.desktop_document.create_element("div")
        overlay.set_style("position", "absolute")
        overlay.set_style("top", "0")
        overlay.set_style("left", "0")
        overlay.set_style("width", "100%")
        overlay.set_style("height", "100%")

        overlay.set_style("background", "radial_gradient(ellipse at 50% 30%, rgba(0,0,0,0.3), rgba(0,0,0,0.6))")
        overlay.set_style("z_index", "1")
        self.desktop_root.append_child(overlay)

        self.desktop_manager = lowlux.desktop.DesktopManager.new()
        self.desktop_manager.set_document(self.desktop_document)
        self.desktop_manager.set_render_backend(self.render_backend)
        self.desktop_manager.set_layout_engine(self.layout_engine)
        self.desktop_manager.initialize()

        lowl.system.console.log("  LOWLUX desktop initialized")
        self.audit.log("LOWLUX desktop initialized", "info")
```

```end
    method initialize_ui_components()
        lowl.system.console.log("Creating UI components...")

        // Create dock (macOS style at bottom)
        self.dock = lowlux.desktop.Dock.new()
        self.dock.set_position("bottom")
        self.dock.set_height(72)
        self.dock.set_alignment("center")
        self.dock.set_magnification(true)
        self.dock.set_magnification_size(96)
        self.dock.set_transparency(0.85)
        self.dock.set_backdrop_filter("blur(20px)")
        self.dock.set_border_radius(20)
        self.dock.set_z_index(100)

        self.dock.add_item("launcher", "/system/icons/launcher.svg", self.show_launcher)
        self.dock.add_item("terminal", "/system/icons/terminal.svg", self.launch_terminal)
        self.dock.add_item("browser", "/system/icons/browser.svg", self.launch_browser)
        self.dock.add_item("files", "/system/icons/files.svg", self.launch_file_manager)
        self.dock.add_item("settings", "/system/icons/settings.svg", self.launch_settings)

        self.desktop_manager.add_component(self.dock)

        // Create system tray (top right)
        self.system_tray = lowlux.desktop.SystemTray.new()
        self.system_tray.set_position("top-right")
        self.system_tray.set_height(40)
        self.system_tray.set_width(200)
        self.system_tray.set_background("rgba(0,0,0,0.5)")
        self.system_tray.set_backdrop_filter("blur(10px)")
        self.system_tray.set_border_radius(0, 0, 12, 12)
        self.system_tray.set_z_index(99)

        self.system_tray.add_icon("network", self.get_network_icon(), self.show_network_panel)
        self.system_tray.add_icon("audio", self.get_audio_icon(), self.show_audio_panel)
        self.system_tray.add_clock(self.update_clock)

        self.desktop_manager.add_component(self.system_tray)

        // Create notification center
        self.notification_center = lowlux.desktop.NotificationCenter.new()
        self.notification_center.set_position("top-right")
        self.notification_center.set_width(380)
        self.notification_center.set_max_notifications(10)
        self.notification_center.set_timeout(5)
        self.notification_center.set_background("rgba(0,0,0,0.85)")
        self.notification_center.set_backdrop_filter("blur(20px)")
        self.notification_center.set_border_radius(16)
        self.notification_center.set_z_index(200)
        self.notification_center.set_visible(false)

        self.desktop_manager.add_component(self.notification_center)

        // Create start menu
        self.start_menu = lowlux.desktop.StartMenu.new()
        self.start_menu.set_size(560, 700)
        self.start_menu.set_position("bottom-left")
        self.start_menu.set_animation("slide_up")
        self.start_menu.set_animation_duration(200)
        self.start_menu.set_background("rgba(30,30,40,0.95)")
        self.start_menu.set_backdrop_filter("blur(30px)")
        self.start_menu.set_border_radius(16)
        self.start_menu.set_z_index(150)
        self.start_menu.set_visible(false)

        self.start_menu.set_user_image("/home/user/.face")
        self.start_menu.set_user_name("NEBULA User")
        self.start_menu.set_user_email("user@nebula.local")

        let apps = {
            "Terminal": {icon: "/system/icons/terminal.svg", action: self.launch_terminal},
            "Browser": {icon: "/system/icons/browser.svg", action: self.launch_browser},
            "Files": {icon: "/system/icons/files.svg", action: self.launch_file_manager},
            "Settings": {icon: "/system/icons/settings.svg", action: self.launch_settings},
            "Calculator": {icon: "/system/icons/calculator.svg", action: self.launch_calculator},
            "Text Editor": {icon: "/system/icons/editor.svg", action: self.launch_editor},
            "Media Player": {icon: "/system/icons/media.svg", action: self.launch_media},
            "System Monitor": {icon: "/system/icons/monitor.svg", action: self.launch_monitor}
        }

        self.start_menu.set_applications(apps)

        let power_options = {
            "Sleep": self.power_manager.sleep,
            "Restart": self.power_manager.restart,
            "Shut Down": self.power_manager.shutdown,
            "Lock": self.lock_screen
        }

        self.start_menu.set_power_options(power_options)

        self.desktop_manager.add_component(self.start_menu)

        // Create workspaces
```

for i from 1 to 4

```end
            let workspace = lowlux.desktop.Workspace.new()
            workspace.set_name("Workspace " + i)
            workspace.set_index(i - 1)
            workspace.set_background_color(0.1, 0.1, 0.2, 1.0)
            self.workspaces[i - 1] = workspace
            self.desktop_manager.add_workspace(workspace)
```

```end
        lowl.system.console.log("  UI components created")
```

```end
    method start_desktop()
        lowl.system.console.log("Starting desktop environment...")

        self.desktop_manager.set_active_workspace(0)
        self.desktop_manager.render()

        self.show_notification("NEBULA OS", "Welcome to NEBULA OS v" + self.version, "info")

        lowl.system.console.log("  Desktop running")
        self.audit.log("Desktop environment started", "info")
```

```end
    method start_system_services()
        lowl.system.console.log("Starting system services...")

        self.service_manager.start_all()

        if self.network_manager.is_available()
            self.network_manager.connect()
            lowl.system.console.log("  Network connected")
```

```end
        self.audio_manager.set_master_volume(75)
        lowl.system.console.log("  Audio initialized")

        lowl.system.console.log("All services started")
        self.audit.log("System services started", "info")
```

```end
    //
```

```text
========================================================================
```

```end
    // APPLICATION LAUNCHERS
    //
```

```text
========================================================================
```

```end
    method launch_terminal()
        let terminal = lowlux.container.VisualContainer.new()
        terminal.set_shape_rectangle(200, 200, 800, 500)
        terminal.set_background("rgba(0,0,0,0.9)")
        terminal.set_border_radius(12)
        terminal.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let terminal_doc = lowl.dom.Document.new()
        let terminal_root = terminal_doc.get_document_element()
        terminal_root.set_style("display", "flex")
        terminal_root.set_style("height", "100%")
        terminal_root.set_style("padding", "12px")

        let terminal_content = terminal_root.create_element("div")
        terminal_content.set_style("background", "#0a0a0f")
        terminal_content.set_style("border_radius", "8px")
        terminal_content.set_style("padding", "12px")
        terminal_content.set_style("color", "#00ff00")
        terminal_content.set_style("font_family", "monospace")
        terminal_content.set_inner_text("$ ")

        terminal_root.append_child(terminal_content)
        terminal.set_content_document(terminal_doc)
        self.desktop_manager.add_container(terminal)

        self.audit.log("Terminal launched", "info")
```

```end
    method launch_browser()
        let browser = lowlux.container.VisualContainer.new()
        browser.set_shape_rectangle(100, 100, 1200, 800)
        browser.set_background("white")
        browser.set_border_radius(12)
        browser.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let browser_doc = lowl.dom.Document.new()
        let browser_root = browser_doc.get_document_element()

        let toolbar = browser_root.create_element("div")
        toolbar.set_style("display", "flex")
        toolbar.set_style("padding", "8px")
        toolbar.set_style("background", "#f0f0f0")
        toolbar.set_style("border_bottom", "1px solid #ddd")
        browser_root.append_child(toolbar)

        let url_bar = toolbar.create_element("input")
        url_bar.set_attribute("type", "text")
        url_bar.set_attribute("placeholder", "Enter URL...")
        url_bar.set_style("flex", "1")
        url_bar.set_style("padding", "8px")
        url_bar.set_style("border_radius", "8px")
        url_bar.set_style("border", "1px solid #ccc")
        toolbar.append_child(url_bar)
        let go_btn = toolbar.create_element("button")
        go_btn.set_text("Go")
        go_btn.set_style("margin_left", "8px")
        go_btn.set_style("padding", "8px 16px")
        go_btn.set_style("background", "#0078d4")
        go_btn.set_style("color", "white")
        go_btn.set_style("border", "none")
        go_btn.set_style("border_radius", "8px")
        go_btn.set_style("cursor", "pointer")
        toolbar.append_child(go_btn)

        let content = browser_root.create_element("iframe")
        content.set_attribute("src", "about:blank")
        content.set_style("width", "100%")
        content.set_style("height", "calc(100% - 50px)")
        content.set_style("border", "none")
        browser_root.append_child(content)

        go_btn.on("click", func()
            content.set_attribute("src", url_bar.get_value())
        end)

        browser.set_content_document(browser_doc)
        self.desktop_manager.add_container(browser)
```

```end
    method launch_file_manager()
        let file_manager = lowlux.container.VisualContainer.new()
        file_manager.set_shape_rectangle(150, 150, 1000, 600)
        file_manager.set_background("white")
        file_manager.set_border_radius(12)
        file_manager.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        self.desktop_manager.add_container(file_manager)
```

```end
    method launch_settings()
        let settings = lowlux.container.VisualContainer.new()
        settings.set_shape_rectangle(300, 200, 800, 700)
        settings.set_background("white")
        settings.set_border_radius(12)
        settings.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let settings_doc = lowl.dom.Document.new()
        let settings_root = settings_doc.get_document_element()
        settings_root.set_style("padding", "24px")

        let title = settings_root.create_element("h1")
        title.set_text("Settings")
        title.set_style("margin_bottom", "20px")
        settings_root.append_child(title)

        let categories = {"General", "Display", "Network", "Security", "Audio", "Applications"}
```

for each cat in categories

```end
            let cat_item = settings_root.create_element("div")
            cat_item.set_text(cat)
            cat_item.set_style("padding", "12px")
            cat_item.set_style("border_bottom", "1px solid #eee")
            cat_item.set_style("cursor", "pointer")
            cat_item.on("click", func() self.show_settings_category(cat) end)
            settings_root.append_child(cat_item)
```

```end
        settings.set_content_document(settings_doc)
        self.desktop_manager.add_container(settings)
```

```end
    method launch_calculator()
        let calculator = lowlux.container.VisualContainer.new()
        calculator.set_shape_rectangle(500, 300, 300, 400)
        calculator.set_background("#2c2c2e")
        calculator.set_border_radius(16)
        calculator.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let calc_doc = lowl.dom.Document.new()
        let calc_root = calc_doc.get_document_element()

        let display = calc_root.create_element("div")
        display.set_style("background", "#1c1c1e")
        display.set_style("color", "white")
        display.set_style("font_size", "48px")
        display.set_style("padding", "20px")
        display.set_style("text_align", "right")
        display.set_style("border_radius", "12px")
        display.set_style("margin", "16px")
        display.set_text("0")
        calc_root.append_child(display)

        let buttons = {"7", "8", "9", "/", "4", "5", "6", "*", "1", "2", "3", "-", "0", "C", "=", "+"}
        let button_grid = calc_root.create_element("div")
        button_grid.set_style("display", "grid")
        button_grid.set_style("grid_template_columns", "repeat(4, 1fr)")
        button_grid.set_style("gap", "8px")
        button_grid.set_style("padding", "16px")
        calc_root.append_child(button_grid)

        let current = "0"
```

for each btn in buttons

```end
            let button = button_grid.create_element("button")
            button.set_text(btn)
            button.set_style("padding", "16px")
            button.set_style("font_size", "20px")
            button.set_style("background", "#3a3a3c")
            button.set_style("color", "white")
            button.set_style("border", "none")
            button.set_style("border_radius", "8px")
            button.set_style("cursor", "pointer")

            if btn == "C"
                button.on("click", func()
                    current = "0"
                    display.set_text(current)
                end)
            else if btn == "="
                button.on("click", func()
                    current = eval(current)
                    display.set_text(current)
                end)
```

```else
                button.on("click", func()
                    if current == "0" and btn != "."
                        current = btn
```

```else
                        current = current + btn
```

```end
                    display.set_text(current)
                end)
```

```end
            button_grid.append_child(button)
```

```end
        calculator.set_content_document(calc_doc)
        self.desktop_manager.add_container(calculator)
```

```end
    method launch_editor()
        let editor = lowlux.container.VisualContainer.new()
        editor.set_shape_rectangle(100, 100, 800, 600)
        editor.set_background("#1e1e1e")
        editor.set_border_radius(12)
        editor.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let editor_doc = lowl.dom.Document.new()
        let editor_root = editor_doc.get_document_element()

        let text_area = editor_root.create_element("textarea")
        text_area.set_style("width", "100%")
        text_area.set_style("height", "100%")
        text_area.set_style("background", "#1e1e1e")
        text_area.set_style("color", "#d4d4d4")
        text_area.set_style("border", "none")
        text_area.set_style("padding", "16px")
        text_area.set_style("font_family", "monospace")
        text_area.set_style("font_size", "14px")
        text_area.set_attribute("placeholder", "Start typing...")

        editor_root.append_child(text_area)
        editor.set_content_document(editor_doc)

        self.desktop_manager.add_container(editor)
```

```end
    method launch_media()
        let media = lowlux.container.VisualContainer.new()
        media.set_shape_rectangle(200, 200, 600, 400)
        media.set_background("#1a1a2e")
        media.set_border_radius(12)
        media.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let media_doc = lowl.dom.Document.new()
        let media_root = media_doc.get_document_element()
        media_root.set_style("display", "flex")
        media_root.set_style("flex_direction", "column")
        media_root.set_style("height", "100%")

        let player = media_root.create_element("video")
        player.set_attribute("controls", "true")
        player.set_style("width", "100%")
        player.set_style("height", "calc(100% - 60px)")
        player.set_style("background", "#000")

        let playlist = media_root.create_element("div")
        playlist.set_style("height", "60px")
        playlist.set_style("background", "rgba(0,0,0,0.8)")
        playlist.set_style("display", "flex")
        playlist.set_style("justify_content", "center")
        playlist.set_style("align_items", "center")
        playlist.set_style("gap", "16px")

        let prev_btn = playlist.create_element("button")
        prev_btn.set_text("⏮")
        prev_btn.set_style("background", "transparent")
        prev_btn.set_style("color", "white")
        prev_btn.set_style("border", "none")
        prev_btn.set_style("cursor", "pointer")
        playlist.append_child(prev_btn)

        let play_btn = playlist.create_element("button")
        play_btn.set_text("▶")
        play_btn.set_style("background", "transparent")
        play_btn.set_style("color", "white")
        play_btn.set_style("border", "none")
        play_btn.set_style("cursor", "pointer")
        playlist.append_child(play_btn)

        let next_btn = playlist.create_element("button")
        next_btn.set_text("⏭")
        next_btn.set_style("background", "transparent")
        next_btn.set_style("color", "white")
        next_btn.set_style("border", "none")
        next_btn.set_style("cursor", "pointer")
        playlist.append_child(next_btn)

        play_btn.on("click", func()
            if player.get_attribute("paused") == "true"
                player.play()
                play_btn.set_text("⏸")
```

```else
                player.pause()
                play_btn.set_text("▶")
```

```end
        end)

        media_root.append_child(player)
        media_root.append_child(playlist)

        media.set_content_document(media_doc)
        self.desktop_manager.add_container(media)
```

```end
    method launch_monitor()
        let monitor = lowlux.container.VisualContainer.new()
        monitor.set_shape_rectangle(300, 100, 400, 400)
        monitor.set_background("rgba(0,0,0,0.85)")
        monitor.set_backdrop_filter("blur(10px)")
        monitor.set_border_radius(12)
        monitor.set_box_shadow(0, 4, 20, 0, 0, 0, 0, 0.3)

        let monitor_doc = lowl.dom.Document.new()
        let monitor_root = monitor_doc.get_document_element()
        monitor_root.set_style("padding", "16px")

        let title = monitor_root.create_element("h3")
        title.set_text("System Monitor")
        title.set_style("color", "white")
        title.set_style("margin_bottom", "16px")
        monitor_root.append_child(title)

        let cpu_label = monitor_root.create_element("div")
        cpu_label.set_text("CPU Usage: --%")
        cpu_label.set_style("color", "#a0a0b0")
        monitor_root.append_child(cpu_label)

        let cpu_bar = monitor_root.create_element("div")
        cpu_bar.set_style("width", "100%")
        cpu_bar.set_style("height", "8px")
        cpu_bar.set_style("background", "rgba(255,255,255,0.2)")
        cpu_bar.set_style("border_radius", "4px")
        cpu_bar.set_style("margin_bottom", "16px")
        monitor_root.append_child(cpu_bar)

        let cpu_fill = cpu_bar.create_element("div")
        cpu_fill.set_style("width", "0%")
        cpu_fill.set_style("height", "100%")
        cpu_fill.set_style("background", "#0078d4")
        cpu_fill.set_style("border_radius", "4px")

        let mem_label = monitor_root.create_element("div")
        mem_label.set_text("Memory Usage: --%")
        mem_label.set_style("color", "#a0a0b0")
        monitor_root.append_child(mem_label)

        let mem_bar = monitor_root.create_element("div")
        mem_bar.set_style("width", "100%")
        mem_bar.set_style("height", "8px")
        mem_bar.set_style("background", "rgba(255,255,255,0.2)")
        mem_bar.set_style("border_radius", "4px")
        mem_bar.set_style("margin_bottom", "16px")
        monitor_root.append_child(mem_bar)

        let mem_fill = mem_bar.create_element("div")
        mem_fill.set_style("width", "0%")
        mem_fill.set_style("height", "100%")
        mem_fill.set_style("background", "#34c759")
        mem_fill.set_style("border_radius", "4px")

        let update_timer = lowl.system.timer.interval(1, func()
            let cpu_usage = lowl.system.get_cpu_usage()
            let mem_usage = lowl.system.get_memory_usage()
            cpu_label.set_text("CPU Usage: " + cpu_usage + "%")
            mem_label.set_text("Memory Usage: " + mem_usage + "%")
            cpu_fill.set_style("width", cpu_usage + "%")
            mem_fill.set_style("width", mem_usage + "%")
        end)

        monitor.set_content_document(monitor_doc)
        self.desktop_manager.add_container(monitor)
```

```end
    //
```

```text
========================================================================
```

```end
    // UI HELPER METHODS
    //
```

```text
========================================================================
```

```end
    method show_launcher()
        if self.start_menu.is_visible()
            self.start_menu.hide()
```

```else
            self.start_menu.show()
```

```end
    method lock_screen()
        self.desktop_manager.lock_screen()
        self.audit.log("Screen locked", "security")
```

```end
    method show_network_panel()
        let panel = lowlux.container.VisualContainer.new()
        panel.set_shape_rectangle(1700, 50, 200, 300)
        panel.set_background("rgba(0,0,0,0.85)")
        panel.set_backdrop_filter("blur(20px)")
        panel.set_border_radius(12)
        panel.set_z_index(200)

        let panel_doc = lowl.dom.Document.new()
        let panel_root = panel_doc.get_document_element()
        panel_root.set_style("padding", "16px")

        let wifi_status = panel_root.create_element("div")
        wifi_status.set_text("WiFi: Connected")
        wifi_status.set_style("color", "white")
        panel_root.append_child(wifi_status)

        let ssid = panel_root.create_element("div")
        ssid.set_text("Network: " + self.network_manager.get_ssid())
        ssid.set_style("color", "#a0a0b0")
        ssid.set_style("font_size", "12px")
        panel_root.append_child(ssid)

        let signal = panel_root.create_element("div")
        signal.set_text("Signal: " + self.network_manager.get_signal_strength() + "%")
        signal.set_style("color", "#a0a0b0")
        signal.set_style("font_size", "12px")
        panel_root.append_child(signal)

        panel.set_content_document(panel_doc)
        self.desktop_manager.add_container(panel)

        lowl.system.timer.delay(5000, func() panel.destroy() end)
```

```end
    method show_audio_panel()
        let panel = lowlux.container.VisualContainer.new()
        panel.set_shape_rectangle(1700, 50, 200, 200)
        panel.set_background("rgba(0,0,0,0.85)")
        panel.set_backdrop_filter("blur(20px)")
        panel.set_border_radius(12)
        panel.set_z_index(200)

        let panel_doc = lowl.dom.Document.new()
        let panel_root = panel_doc.get_document_element()
        panel_root.set_style("padding", "16px")

        let volume_label = panel_root.create_element("div")
        volume_label.set_text("Volume: 75%")
        volume_label.set_style("color", "white")
        panel_root.append_child(volume_label)
        let volume_slider = panel_root.create_element("input")
        volume_slider.set_attribute("type", "range")
        volume_slider.set_attribute("min", "0")
        volume_slider.set_attribute("max", "100")
        volume_slider.set_value("75")
        volume_slider.set_style("width", "100%")
        volume_slider.on("input", func()
            let vol = volume_slider.get_value()
            volume_label.set_text("Volume: " + vol + "%")
            self.audio_manager.set_master_volume(vol)
        end)
        panel_root.append_child(volume_slider)

        let mute_btn = panel_root.create_element("button")
        mute_btn.set_text("Mute")
        mute_btn.set_style("margin_top", "12px")
        mute_btn.set_style("padding", "8px")
        mute_btn.set_style("width", "100%")
        mute_btn.set_style("background", "#ff3b30")
        mute_btn.set_style("color", "white")
        mute_btn.set_style("border", "none")
        mute_btn.set_style("border_radius", "8px")
        mute_btn.set_style("cursor", "pointer")
        mute_btn.on("click", func()
            self.audio_manager.set_muted(true)
            panel.destroy()
        end)
        panel_root.append_child(mute_btn)

        panel.set_content_document(panel_doc)
        self.desktop_manager.add_container(panel)

        lowl.system.timer.delay(5000, func() panel.destroy() end)
```

```end
    method show_notification(title, message, type)
        let notif = self.notification_center.add_notification(title, message, type)
        if not self.notification_center.is_visible()
            self.notification_center.set_visible(true)
            lowl.system.timer.delay(5000, func()
                self.notification_center.set_visible(false)
            end)
```

```end
        self.audit.log("Notification: " + title + " - " + message, "info")
```

```end
    method update_clock(clock_element)
        lowl.system.timer.interval(1, func()
            let now = lowl.system.get_time()
            clock_element.set_text(now.format("%I:%M %p"))
        end)
```

```end
    method get_network_icon()
        if self.network_manager.is_connected()
            let strength = self.network_manager.get_signal_strength()
            if strength > 70
                return "📶"
            else if strength > 30
                return "📶"
```

```else
                return "📶"
```

```else
            return "❌"
```

```end
    method get_audio_icon()
        let volume = self.audio_manager.get_master_volume()
        if volume == 0
            return "🔇"
        else if volume < 30
            return "🔈"
        else if volume < 70
            return "🔉"
```

```else
            return "🔊"
```

```end
    method show_settings_category(category)
        lowl.system.console.log("Opening settings: " + category)
```

```end
    //
```

```text
========================================================================
```

```end
    // MAIN ENTRY POINT
    //
```

```text
========================================================================
```

```end
    method run()
        self.initialize()

        lowl.system.console.log("NEBULA OS is running")
        lowl.system.console.log("Press Ctrl+Alt+Del for options")

        while true
            lowl.system.process_events()
            self.desktop_manager.render()
            lowl.system.scheduler.yield()
```

```end
// Create and run the platform
let nebula = NebulaPlatform.new()
nebula.run()
```

## Part 2: LOWLUX Document Object Model (lowlux/document.lowl)

```lua
lowl_script lowlux_document version 1.0.0

namespace lowlux

class Document
    property document_element = null
    property title = ""
    property viewport_width = 0
    property viewport_scale = 1.0
    property style_sheets = []
    property event_listeners = {}

    method new()
        self.document_element = Element.new("html")
        return self
```

```end
    method set_title(new_title)
        self.title = new_title
        lowl.system.console.log("Document title: " + new_title)
```

```end
    method get_title()
        return self.title
```

```end
    method set_viewport(width, scale)
        self.viewport_width = width
        self.viewport_scale = scale
```

```end
    method get_document_element()
        return self.document_element
```

```end
    method create_element(tag_name)
        return Element.new(tag_name)
```

```end
    method create_text_node(text)
        let element = Element.new("text")
        element.set_inner_text(text)
        return element
```

```end
    method get_element_by_id(id)
        return self.document_element.find_by_id(id)
```

```end
    method get_elements_by_class_name(class_name)
        return self.document_element.find_by_class(class_name)
```

```end
    method get_elements_by_tag_name(tag_name)
        return self.document_element.find_by_tag(tag_name)
```

```end
    method query_selector(selector)
        return self.document_element.query_selector(selector)
```

```end
    method query_selector_all(selector)
        return self.document_element.query_selector_all(selector)
```

```end
    method append_child(element)
        self.document_element.append_child(element)
```

```end
    method remove_child(element)
        self.document_element.remove_child(element)
```

```end
    method add_style_sheet(css)
        let sheet = StyleSheet.new(css)
        self.style_sheets.append(sheet)
        self.style_resolver.add_sheet(sheet)
```

```end
    method add_event_listener(type, listener)
        if not self.event_listeners.has_key(type)
            self.event_listeners[type] = []
```

```end
        self.event_listeners[type].append(listener)
```

```end
    method dispatch_event(event)
        let type = event.get_type()
        if self.event_listeners.has_key(type)
```

for each listener in self.event_listeners[type]

```end
                listener(event)
```

```end
        self.document_element.dispatch_event(event)
```

```end
    method serialize()
        return "<!DOCTYPE html>\n" + self.document_element.serialize()
```

```end
    method deserialize(html)
        // Parse HTML and build DOM tree
        let parser = HTMLParser.new()
        let parsed = parser.parse(html)
        self.document_element = parsed
```

```end
class Element
    property tag_name = ""
    property attributes = {}
    property styles = {}
    property computed_style = null
    property parent = null
    property children = []
    property event_listeners = {}
    property inner_text = ""

    method new(tag)
        self.tag_name = tag
        self.computed_style = ComputedStyle.new()
        return self
```

```end
    method get_attribute(name)
        if self.attributes.has_key(name)
            return self.attributes[name]
```

```end
        return null
```

```end
    method set_attribute(name, value)
        self.attributes[name] = value
```

```end
    method remove_attribute(name)
        if self.attributes.has_key(name)
            self.attributes.remove(name)
```

```end
    method get_style(property)
        if self.styles.has_key(property)
            return self.styles[property]
```

```end
        return ""
```

```end
    method set_style(property, value)
        self.styles[property] = value
        self.mark_dirty()
```

```end
    method get_computed_style()
        return self.computed_style
```

```end
    method get_class_name()
        return self.get_attribute("class")
```

```end
    method set_class_name(class_name)
        self.set_attribute("class", class_name)
```

```end
    method add_class(class_name)
        let current = self.get_class_name()
        if current == null
            self.set_class_name(class_name)
        else if current.find(class_name) == null
            self.set_class_name(current + " " + class_name)
```

```end
    method remove_class(class_name)
        let current = self.get_class_name()
        if current != null
            let new_classes = current.split(" ").filter(func(c) c != class_name end).join(" ")
            self.set_class_name(new_classes)
```

```end
    method has_class(class_name)
        let current = self.get_class_name()
        if current != null
            return current.split(" ").contains(class_name)
```

```end
        return false
```

```end
    method get_id()
        return self.get_attribute("id")
```

```end
    method set_id(id)
        self.set_attribute("id", id)
```

```end
    method get_inner_text()
        return self.inner_text
```

```end
    method set_inner_text(text)
        self.inner_text = text
        self.children.clear()
        self.mark_dirty()
```

```end
    method get_outer_text()
        return self.inner_text
```

```end
    method set_outer_text(text)
        self.set_inner_text(text)
```

```end
    method get_inner_html()
        let html = ""
```

for each child in self.children

```end
            html = html + child.serialize()
```

```end
        return html
```

```end
    method set_inner_html(html)
        self.children.clear()
        let parser = HTMLParser.new()
        let parsed = parser.parse_fragment(html)
```

for each child in parsed

```end
            self.append_child(child)
```

```end
        self.mark_dirty()
```

```end
    method get_outer_html()
        return self.serialize()
```

```end
    method set_outer_html(html)
        let parser = HTMLParser.new()
        let new_element = parser.parse_fragment(html)[0]
        if self.parent != null
            self.parent.replace_child(self, new_element)
```

```end
    method append_child(child)
        child.parent = self
        self.children.append(child)
        self.mark_dirty()
```

```end
    method insert_before(new_child, ref_child)
        let index = self.children.index_of(ref_child)
        if index >= 0
            new_child.parent = self
            self.children.insert(index, new_child)
            self.mark_dirty()
```

```end
    method remove_child(child)
        let index = self.children.index_of(child)
        if index >= 0
            child.parent = null
            self.children.remove_at(index)
            self.mark_dirty()
```

```end
    method replace_child(old_child, new_child)
        let index = self.children.index_of(old_child)
        if index >= 0
            old_child.parent = null
            new_child.parent = self
            self.children[index] = new_child
            self.mark_dirty()
```

```end
    method get_parent()
        return self.parent
```

```end
    method get_children()
        return self.children
```

```end
    method get_first_child()
        if self.children.size() > 0
            return self.children[0]
```

```end
        return null
```

```end
    method get_last_child()
        if self.children.size() > 0
            return self.children[self.children.size() - 1]
```

```end
        return null
```

```end
    method get_next_sibling()
        if self.parent != null
            let siblings = self.parent.get_children()
            let index = siblings.index_of(self)
```

if index >= 0 and index + 1 < siblings.size()

```end
                return siblings[index + 1]
```

```end
        return null
```

```end
    method get_previous_sibling()
        if self.parent != null
            let siblings = self.parent.get_children()
            let index = siblings.index_of(self)
            if index > 0
                return siblings[index - 1]
```

```end
        return null
```

```end
    method add_event_listener(type, listener)
        if not self.event_listeners.has_key(type)
            self.event_listeners[type] = []
```

```end
        self.event_listeners[type].append(listener)
```

```end
    method remove_event_listener(type, listener)
        if self.event_listeners.has_key(type)
            self.event_listeners[type] = self.event_listeners[type].filter(func(l) l != listener end)
```

```end
    method dispatch_event(event)
        let type = event.get_type()
        if self.event_listeners.has_key(type)
```

for each listener in self.event_listeners[type]

```end
                listener(event)
```

if event.get_bubbles() and not event.get_propagation_stopped()

```end
            if self.parent != null
                self.parent.dispatch_event(event)
```

```end
    method get_bounding_client_rect()
        return self.layout_rect
```

```end
    method get_client_rects()
        return [self.layout_rect]
```

```end
    method scroll_into_view()
        // Scroll element into viewport
        lowl.system.console.log("Scroll into view: " + self.tag_name)
```

```end
    method get_scroll_top()
        return 0
```

```end
    method set_scroll_top(value)
        // Set scroll position
```

```end
    method get_scroll_left()
        return 0
```

```end
    method set_scroll_left(value)
        // Set scroll position
```

```end
    method get_scroll_width()
        return self.layout_rect.width
```

```end
    method get_scroll_height()
        return self.layout_rect.height
```

```end
    method get_client_width()
        return self.layout_rect.width
```

```end
    method get_client_height()
        return self.layout_rect.height
```

```end
    method focus()
        self.set_attribute("focused", "true")
        let event = FocusEvent.new("focus")
        self.dispatch_event(event)
```

```end
    method blur()
        self.remove_attribute("focused")
        let event = FocusEvent.new("blur")
        self.dispatch_event(event)
```

```end
    method click()
        let event = MouseEvent.new("click")
        event.set_client_x(0)
        event.set_client_y(0)
        self.dispatch_event(event)
```

```end
    method get_tab_index()
        let idx = self.get_attribute("tabindex")
        if idx != null
            return idx.to_int()
```

```end
        return -1
```

```end
    method set_tab_index(index)
        self.set_attribute("tabindex", index)
```

```end
    method get_draggable()
        return self.get_attribute("draggable") == "true"
```

```end
    method set_draggable(draggable)
        self.set_attribute("draggable", draggable ? "true" : "false")
```

```end
    method get_hidden()
        return self.has_class("hidden") or self.get_attribute("hidden") == "hidden"
```

```end
    method set_hidden(hidden)
        if hidden
            self.add_class("hidden")
            self.set_attribute("hidden", "hidden")
```

```else
            self.remove_class("hidden")
            self.remove_attribute("hidden")
```

```end
    method get_content_editable()
        return self.get_attribute("contenteditable") == "true"
```

```end
    method set_content_editable(editable)
        self.set_attribute("contenteditable", editable ? "true" : "false")
```

```end
    method get_spellcheck()
        return self.get_attribute("spellcheck") == "true"
```

```end
    method set_spellcheck(check)
        self.set_attribute("spellcheck", check ? "true" : "false")
```

```end
    method get_lang()
        return self.get_attribute("lang")
```

```end
    method set_lang(lang)
        self.set_attribute("lang", lang)
```

```end
    method get_dir()
        return self.get_attribute("dir")
```

```end
    method set_dir(dir)
        self.set_attribute("dir", dir)
```

```end
    method get_title()
        return self.get_attribute("title")
```

```end
    method set_title(title)
        self.set_attribute("title", title)
```

```end
    method clone_node(deep)
        let clone = Element.new(self.tag_name)
        clone.attributes = self.attributes.copy()
        clone.styles = self.styles.copy()
        clone.inner_text = self.inner_text

        if deep
```

for each child in self.children

```end
                clone.append_child(child.clone_node(true))
```

```end
        return clone
```

```end
    method serialize()
        let html = "<" + self.tag_name
```

for each name, value in self.attributes

```end
            html = html + " " + name + "=\"" + value + "\""
```

```end
        if self.styles.size() > 0
            let style_str = ""
```

for each prop, value in self.styles

```end
                style_str = style_str + prop + ": " + value + "; "
```

```end
            html = html + " style=\"" + style_str + "\""
```

```end
        html = html + ">"
        html = html + self.inner_text
```

for each child in self.children

```end
            html = html + child.serialize()
```

```end
        html = html + "</" + self.tag_name + ">"
        return html
```

```end
    method mark_dirty()
        // Mark element for re-layout
        if self.parent != null
            self.parent.mark_dirty()
```

```end
    method find_by_id(id)
        if self.get_id() == id
            return self
```

for each child in self.children

```end
            let found = child.find_by_id(id)
            if found != null
                return found
```

```end
        return null
```

```end
    method find_by_class(class_name)
        let results = []
        if self.has_class(class_name)
            results.append(self)
```

for each child in self.children

```end
            results.extend(child.find_by_class(class_name))
```

```end
        return results
```

```end
    method find_by_tag(tag_name)
        let results = []
        if self.tag_name == tag_name
            results.append(self)
```

for each child in self.children

```end
            results.extend(child.find_by_tag(tag_name))
```

```end
        return results
```

```end
    method query_selector(selector)
        // Simple selector implementation
        if selector.starts_with("#")
            return self.find_by_id(selector.substring(1))
        else if selector.starts_with(".")
            let results = self.find_by_class(selector.substring(1))
            if results.size() > 0
                return results[0]
```

```else
            let results = self.find_by_tag(selector)
            if results.size() > 0
                return results[0]
```

```end
        return null
```

```end
    method query_selector_all(selector)
        if selector.starts_with("#")
            let result = self.find_by_id(selector.substring(1))
            return result != null ? [result] : []
        else if selector.starts_with(".")
            return self.find_by_class(selector.substring(1))
```

```else
            return self.find_by_tag(selector)
```

## Part 3: LOWLUX Container System (lowlux/container.lowl)

```lua
lowl_script lowlux_container version 1.0.0

namespace lowlux

class VisualContainer
    property id = ""
    property x = 0
    property y = 0
    property width = 800
    property height = 600
    property shape_type = "rectangle"  // rectangle, oval, polygon, rounded_rectangle
    property shape_params = {}
    property visible = true
    property active = false
    property modal = false
    property resizable = true
    property movable = true
    property closable = true
    property z_order = 0
    property workspace = 0
    property opacity = 1.0
    property background_color = "rgba(255,255,255,1.0)"
    property background_blur = 0
    property backdrop_filter = ""
    property border_width = 0
    property border_color = "rgba(0,0,0,1.0)"
    property border_radius = 0
    property box_shadow = ""
    property transform = ""
    property animation = null
    property content_document = null
    property parent = null
    property event_listeners = {}

    method new()
        self.id = lowl.utils.uuid.generate_v4()
        return self
```

```end
    method set_shape_rectangle(x, y, width, height)
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.shape_type = "rectangle"
        self.shape_params = {x: x, y: y, w: width, h: height}
```

```end
    method set_shape_rounded_rectangle(x, y, width, height, radius)
        self.set_shape_rectangle(x, y, width, height)
        self.shape_type = "rounded_rectangle"
        self.shape_params.radius = radius
        self.border_radius = radius
```

```end
    method set_shape_oval(cx, cy, rx, ry)
        self.x = cx - rx
        self.y = cy - ry
        self.width = rx * 2
        self.height = ry * 2
        self.shape_type = "oval"
        self.shape_params = {cx: cx, cy: cy, rx: rx, ry: ry}
```

```end
    method set_shape_polygon(points)
        self.shape_type = "polygon"
        self.shape_params = {points: points}
        // Calculate bounds
        let min_x = 999999, min_y = 999999, max_x = -999999, max_y = -999999
```

for each point in points

```end
            if point.x < min_x, min_x = point.x end
            if point.y < min_y, min_y = point.y end
            if point.x > max_x, max_x = point.x end
            if point.y > max_y, max_y = point.y end
```

```end
        self.x = min_x
        self.y = min_y
        self.width = max_x - min_x
        self.height = max_y - min_y
```

```end
    method set_content_document(doc)
        self.content_document = doc
```

```end
    method get_content_document()
        return self.content_document
```

```end
    method set_background_color(color)
        self.background_color = color
```

```end
    method set_background_blur(radius)
        self.background_blur = radius
        if radius > 0
            self.backdrop_filter = "blur(" + radius + "px)"
```

```end
    method set_backdrop_filter(filter)
        self.backdrop_filter = filter
```

```end
    method set_border(width, color)
        self.border_width = width
        self.border_color = color
```

```end
    method set_border_radius(radius)
        self.border_radius = radius
```

method set_box_shadow(offset_x, offset_y, blur, spread, r, g, b, a)

```end
        self.box_shadow = offset_x + "px " + offset_y + "px " + blur + "px " + spread + "px rgba(" + r * 255 + "," + g * 255 + "," + b * 255 + "," + a + ")"
```

```end
    method set_opacity(opacity)
        self.opacity = opacity
```

```end
    method set_transform(transform)
        self.transform = transform
```

```end
    method set_position(x, y)
        self.x = x
        self.y = y
        if self.shape_type == "rectangle"
            self.shape_params.x = x
            self.shape_params.y = y
        else if self.shape_type == "oval"
            self.shape_params.cx = x + self.width / 2
            self.shape_params.cy = y + self.height / 2
```

```end
    method set_size(width, height)
        self.width = width
        self.height = height
        if self.shape_type == "oval"
            self.shape_params.rx = width / 2
            self.shape_params.ry = height / 2
```

```end
    method set_z_order(z)
        self.z_order = z
```

```end
    method get_z_order()
        return self.z_order
```

```end
    method set_workspace(workspace)
        self.workspace = workspace
```

```end
    method get_workspace()
        return self.workspace
```

```end
    method set_animation(anim)
        self.animation = anim
```

```end
    method show()
        self.visible = true
        self.dispatch_event(ContainerEvent.new("show"))
```

```end
    method hide()
        self.visible = false
        self.dispatch_event(ContainerEvent.new("hide"))
```

```end
    method is_visible()
        return self.visible
```

```end
    method set_active(active)
        self.active = active
```

```end
    method is_active()
        return self.active
```

```end
    method set_modal(modal)
        self.modal = modal
```

```end
    method is_modal()
        return self.modal
```

```end
    method set_resizable(resizable)
        self.resizable = resizable
```

```end
    method is_resizable()
        return self.resizable
```

```end
    method set_movable(movable)
        self.movable = movable
```

```end
    method is_movable()
        return self.movable
```

```end
    method set_closable(closable)
        self.closable = closable
```

```end
    method is_closable()
        return self.closable
```

```end
    method contains_point(px, py)
        if self.shape_type == "rectangle"
```

return px >= self.x and px <= self.x + self.width and py >= self.y and py <= self.y + self.height

```end
        else if self.shape_type == "oval"
            let dx = (px - self.shape_params.cx) / self.shape_params.rx
            let dy = (py - self.shape_params.cy) / self.shape_params.ry
            return dx * dx + dy * dy <= 1.0
        else if self.shape_type == "polygon"
            // Ray casting algorithm
            let inside = false
            let points = self.shape_params.points
            let n = points.size()
```

for i from 0 to n - 1

```end
                let j = (i + 1) % n
                if ((points[i].y > py) != (points[j].y > py)) and

                   (px < (points[j].x - points[i].x) * (py - points[i].y) / (points[j].y - points[i].y) + points[i].x)
                    inside = not inside
```

```end
            return inside
```

```end
        return false
```

```end
    method raise()
        self.dispatch_event(ContainerEvent.new("raise"))
```

```end
    method lower()
        self.dispatch_event(ContainerEvent.new("lower"))
```

```end
    method close()
        self.dispatch_event(ContainerEvent.new("close"))
        if self.parent != null
            self.parent.remove_container(self)
```

```end
    method destroy()
        self.close()
```

```end
    method add_event_listener(type, listener)
        if not self.event_listeners.has_key(type)
            self.event_listeners[type] = []
```

```end
        self.event_listeners[type].append(listener)
```

```end
    method remove_event_listener(type, listener)
        if self.event_listeners.has_key(type)
            self.event_listeners[type] = self.event_listeners[type].filter(func(l) l != listener end)
```

```end
    method dispatch_event(event)
        let type = event.get_type()
        if self.event_listeners.has_key(type)
```

for each listener in self.event_listeners[type]

```end
                listener(event)
```

```end
    method render(renderer)
        if not self.visible
```

```end
        renderer.save()
        renderer.set_opacity(self.opacity)
        renderer.set_transform(self.transform)

        // Draw background
```

if self.background_blur > 0 or self.backdrop_filter != ""

```end
            renderer.begin_backdrop_filter(self.backdrop_filter)
```

```end
        renderer.fill_rect(self.x, self.y, self.width, self.height, self.background_color)

        if self.background_blur > 0
            renderer.end_backdrop_filter()
```

```end
        // Draw border
        if self.border_width > 0

            renderer.stroke_rect(self.x, self.y, self.width, self.height, self.border_width, self.border_color)
```

```end
        // Draw shadow
        if self.box_shadow != ""
            renderer.draw_shadow(self.x, self.y, self.width, self.height, self.box_shadow)
```

```end
        // Draw content
        if self.content_document != null
            renderer.render_document(self.content_document, self.x, self.y)
```

```end
        renderer.restore()
```

```end
class ShapeContainer extends VisualContainer
    method set_shape_from_definition(shape_def)
        self.shape_type = shape_def.get_type()
        self.shape_params = shape_def.get_params()
        let bounds = shape_def.get_bounds()
        self.x = bounds.x
        self.y = bounds.y
        self.width = bounds.width
        self.height = bounds.height
```

```end
class ShapeDefinition
    property type = "rectangle"
    property params = {}

    method create_rectangle(x, y, w, h)
        self.type = "rectangle"
        self.params = {x: x, y: y, w: w, h: h}
        return self
```

```end
    method create_rounded_rectangle(x, y, w, h, r)
        self.type = "rounded_rectangle"
        self.params = {x: x, y: y, w: w, h: h, r: r}
        return self
```

```end
    method create_oval(cx, cy, rx, ry)
        self.type = "oval"
        self.params = {cx: cx, cy: cy, rx: rx, ry: ry}
        return self
```

```end
    method create_polygon(points)
        self.type = "polygon"
        self.params = {points: points}
        return self
```

```end
    method create_star(cx, cy, points, inner_r, outer_r)
        let vertices = []
```

for i from 0 to points * 2 - 1

```end
            let angle = i * math.pi / points
            let r = (i % 2 == 0) ? outer_r : inner_r
            let x = cx + r * math.cos(angle)
            let y = cy + r * math.sin(angle)
            vertices.append({x: x, y: y})
```

```end
        return self.create_polygon(vertices)
```

```end
    method get_type()
        return self.type
```

```end
    method get_params()
        return self.params
```

```end
    method get_bounds()
        if self.type == "rectangle"
            return {x: self.params.x, y: self.params.y, width: self.params.w, height: self.params.h}
        else if self.type == "oval"

            return {x: self.params.cx - self.params.rx, y: self.params.cy - self.params.ry, width: self.params.rx * 2, height: self.params.ry * 2}
        else if self.type == "polygon"
            let min_x = 999999, min_y = 999999, max_x = -999999, max_y = -999999
```

for each point in self.params.points

```end
                if point.x < min_x, min_x = point.x end
                if point.y < min_y, min_y = point.y end
                if point.x > max_x, max_x = point.x end
                if point.y > max_y, max_y = point.y end
```

```end
            return {x: min_x, y: min_y, width: max_x - min_x, height: max_y - min_y}
```

```end
        return {x: 0, y: 0, width: 0, height: 0}
```

```end
class ContainerEvent
    property type = ""
    property target = null
    property bubbles = true
    property cancelable = false
    property propagation_stopped = false
    property default_prevented = false

    method new(event_type)
        self.type = event_type
        self.target = null
        return self
```

```end
    method get_type()
        return self.type
```

```end
    method get_target()
        return self.target
```

```end
    method set_target(tgt)
        self.target = tgt
```

```end
    method get_bubbles()
        return self.bubbles
```

```end
    method stop_propagation()
        self.propagation_stopped = true
```

```end
    method get_propagation_stopped()
        return self.propagation_stopped
```

```end
    method prevent_default()
        self.default_prevented = true
```

```end
    method get_default_prevented()
        return self.default_prevented
```

## Part 4: LOWLUX Desktop Manager (lowlux/desktop.lowl)

```lua
lowl_script lowlux_desktop version 1.0.0

namespace lowlux

class DesktopManager
    property document = null
    property render_backend = null
    property layout_engine = null
    property containers = []
    property components = []
    property workspaces = []
    property active_workspace = 0
    property active_container = null
    property lock_screen_active = false
    property lock_screen_container = null

    method new()
        return self
```

```end
    method set_document(doc)
        self.document = doc
```

```end
    method set_render_backend(backend)
        self.render_backend = backend
```

```end
    method set_layout_engine(engine)
        self.layout_engine = engine
```

```end
    method initialize()
        self.render_backend.initialize(1920, 1080)
        self.layout_engine.initialize()
        lowl.system.console.log("Desktop manager initialized")
```

```end
    method add_container(container)
        container.parent = self
        self.containers.append(container)
        self.sort_containers()
        container.dispatch_event(ContainerEvent.new("added"))
```

```end
    method remove_container(container)
        let index = self.containers.index_of(container)
        if index >= 0
            self.containers.remove_at(index)
            container.dispatch_event(ContainerEvent.new("removed"))
```

```end
    method get_containers()
        return self.containers
```

```end
    method get_containers_by_workspace(workspace)
        let result = []
```

for each container in self.containers

```end
            if container.get_workspace() == workspace
                result.append(container)
```

```end
        return result
```

```end
    method get_container_at_position(x, y)
        // Return topmost container at position (reverse order for z-order)
```

for i from self.containers.size() - 1 down to 0

```end
            let container = self.containers[i]
```

if container.is_visible() and container.contains_point(x, y)

```end
                return container
```

```end
        return null
```

```end
    method get_active_container()
        return self.active_container
```

```end
    method set_active_container(container)
        if self.active_container != null
            self.active_container.set_active(false)
            self.active_container.dispatch_event(ContainerEvent.new("deactivated"))
```

```end
        self.active_container = container

        if container != null
            container.set_active(true)
            container.dispatch_event(ContainerEvent.new("activated"))
            self.raise_container(container)
```

```end
    method raise_container(container)
        let index = self.containers.index_of(container)
```

if index >= 0 and index < self.containers.size() - 1

```end
            self.containers.remove_at(index)
            self.containers.append(container)
            self.sort_containers()
```

```end
    method lower_container(container)
        let index = self.containers.index_of(container)
        if index > 0
            self.containers.remove_at(index)
            self.containers.insert(0, container)
            self.sort_containers()
```

```end
    method sort_containers()
        self.containers.sort(func(a, b) a.get_z_order() < b.get_z_order() end)
```

```end
    method add_component(component)
        self.components.append(component)
```

```end
    method remove_component(component)
        let index = self.components.index_of(component)
        if index >= 0
            self.components.remove_at(index)
```

```end
    method add_workspace(workspace)
        workspace.set_index(self.workspaces.size())
        self.workspaces.append(workspace)
```

```end
    method set_active_workspace(index)
```

if index >= 0 and index < self.workspaces.size()

```end
            self.active_workspace = index
```

for each workspace in self.workspaces

```end
                workspace.set_active(workspace.get_index() == index)
```

```end
            self.dispatch_event(DesktopEvent.new("workspace_changed"))
```

```end
    method get_active_workspace()
        return self.active_workspace
```

```end
    method get_workspace_count()
        return self.workspaces.size()
```

```end
    method lock_screen()
        self.lock_screen_active = true

        if self.lock_screen_container == null
            self.lock_screen_container = VisualContainer.new()
            self.lock_screen_container.set_shape_rectangle(0, 0, 1920, 1080)
            self.lock_screen_container.set_background_color("rgba(0,0,0,0.9)")
            self.lock_screen_container.set_backdrop_filter("blur(20px)")
            self.lock_screen_container.set_z_order(10000)

            let lock_doc = Document.new()
            let lock_root = lock_doc.get_document_element()
            lock_root.set_style("display", "flex")
            lock_root.set_style("justify_content", "center")
            lock_root.set_style("align_items", "center")
            lock_root.set_style("height", "100vh")

            let lock_card = lock_root.create_element("div")
            lock_card.set_style("background", "rgba(30,30,40,0.95)")
            lock_card.set_style("border_radius", "24px")
            lock_card.set_style("padding", "48px")
            lock_card.set_style("text_align", "center")
            lock_card.set_style("min_width", "320px")
            lock_root.append_child(lock_card)

            let lock_icon = lock_card.create_element("div")
            lock_icon.set_text("🔒")
            lock_icon.set_style("font_size", "64px")
            lock_icon.set_style("margin_bottom", "24px")
            lock_card.append_child(lock_icon)

            let lock_title = lock_card.create_element("h2")
            lock_title.set_text("System Locked")
            lock_title.set_style("color", "white")
            lock_title.set_style("margin_bottom", "16px")
            lock_card.append_child(lock_title)

            let password_field = lock_card.create_element("input")
            password_field.set_attribute("type", "password")
            password_field.set_attribute("placeholder", "Enter password")
            password_field.set_style("width", "100%")
            password_field.set_style("padding", "12px")
            password_field.set_style("margin_bottom", "16px")
            password_field.set_style("background", "rgba(255,255,255,0.15)")
            password_field.set_style("border", "none")
            password_field.set_style("border_radius", "8px")
            password_field.set_style("color", "white")
            lock_card.append_child(password_field)

            let unlock_btn = lock_card.create_element("button")
            unlock_btn.set_text("Unlock")
            unlock_btn.set_style("width", "100%")
            unlock_btn.set_style("padding", "12px")
            unlock_btn.set_style("background", "#0a84ff")
            unlock_btn.set_style("color", "white")
            unlock_btn.set_style("border", "none")
            unlock_btn.set_style("border_radius", "8px")
            unlock_btn.set_style("cursor", "pointer")
            unlock_btn.on("click", func()
                if password_field.get_value() == self.get_unlock_password()
                    self.unlock_screen()
```

```else
                    lock_title.set_text("Invalid password")
                    lock_title.set_style("color", "#ff3b30")
```

```end
            end)
            lock_card.append_child(unlock_btn)

            password_field.on("keypress", func(e)
                if e.get_key() == "Enter"
                    unlock_btn.click()
```

```end
            end)

            self.lock_screen_container.set_content_document(lock_doc)
```

```end
        self.add_container(self.lock_screen_container)
        self.set_active_container(self.lock_screen_container)
        self.dispatch_event(DesktopEvent.new("locked"))
```

```end
    method unlock_screen()
        if self.lock_screen_active
            self.remove_container(self.lock_screen_container)
            self.lock_screen_active = false
            self.dispatch_event(DesktopEvent.new("unlocked"))
```

```end
    method get_unlock_password()
        return lowl.security.keyring.get("user_password")
```

```end
    method show_desktop()
```

for each container in self.containers

```end
            container.hide()
```

```end
        self.dispatch_event(DesktopEvent.new("desktop_shown"))
```

```end
    method restore_windows()
```

for each container in self.containers

```end
            container.show()
```

```end
        self.dispatch_event(DesktopEvent.new("windows_restored"))
```

```end
    method show_task_view()
        self.dispatch_event(DesktopEvent.new("task_view_shown"))
```

```end
    method cycle_focus()
        let visible_containers = []
```

for each container in self.containers

```end
            if container.is_visible()
                visible_containers.append(container)
```

```end
        if visible_containers.size() > 0
            let current_index = visible_containers.index_of(self.active_container)
            let next_index = (current_index + 1) % visible_containers.size()
            self.set_active_container(visible_containers[next_index])
```

```end
    method close_active_container()
```

if self.active_container != null and self.active_container.is_closable()

```end
            self.active_container.close()
```

```end
    method render()
        if self.lock_screen_active
            self.lock_screen_container.render(self.render_backend)
```

for each container in self.containers

```else
                if container.is_visible() and container.get_workspace() == self.active_workspace
                    container.render(self.render_backend)
```

for each component in self.components

```end
                component.render(self.render_backend)
```

```end
        self.render_backend.present()
```

```end
    method add_event_listener(type, listener)
        if not self.event_listeners.has_key(type)
            self.event_listeners[type] = []
```

```end
        self.event_listeners[type].append(listener)
```

```end
    method dispatch_event(event)
        let type = event.get_type()
        if self.event_listeners.has_key(type)
```

for each listener in self.event_listeners[type]

```end
                listener(event)
```

```end
class Dock
    property x = 0
    property y = 0
    property width = 0
    property height = 0
    property position = "bottom"
    property alignment = "center"
    property magnification = false
    property magnification_size = 96
    property normal_size = 64
    property transparency = 0.85
    property backdrop_filter = ""
    property border_radius = 0
    property items = []
    property visible = true

    method set_position(pos)
        self.position = pos
```

```end
    method set_height(h)
        self.height = h
```

```end
    method set_alignment(align)
        self.alignment = align
```

```end
    method set_magnification(enable)
        self.magnification = enable
```

```end
    method set_magnification_size(size)
        self.magnification_size = size
```

```end
    method set_transparency(transp)
        self.transparency = transp
```

```end
    method set_backdrop_filter(filter)
        self.backdrop_filter = filter
```

```end
    method set_border_radius(radius)
        self.border_radius = radius
```

```end
    method add_item(id, icon_path, click_handler)
        let item = {id: id, icon: icon_path, handler: click_handler}
        self.items.append(item)
```

```end
    method remove_item(id)
        self.items = self.items.filter(func(item) item.id != id end)
```

```end
    method render(renderer)
        if not self.visible
```

```end
        let screen_width = renderer.get_width()
        let screen_height = renderer.get_height()

        if self.position == "bottom"
            self.y = screen_height - self.height
            self.width = 0
```

for each item in self.items

```end
                self.width = self.width + self.normal_size + 16
```

```end
            self.x = (screen_width - self.width) / 2
```

```end
        renderer.save()
        renderer.set_opacity(self.transparency)

        renderer.fill_rounded_rect(self.x, self.y, self.width, self.height, self.border_radius, "rgba(0,0,0," + self.transparency + ")")

        if self.backdrop_filter != ""
            renderer.apply_backdrop_filter(self.backdrop_filter)
```

```end
        let item_x = self.x + 16
```

for each item in self.items

```end
            let icon = renderer.load_image(item.icon)
            let size = self.normal_size
            renderer.draw_image(icon, item_x, self.y + (self.height - size) / 2, size, size)

            // Draw indicator if application is running
            if self.is_running(item.id)

                renderer.fill_rect(item_x + size / 2 - 4, self.y + self.height - 8, 8, 4, "rgba(255,255,255,0.8)")
```

```end
            item_x = item_x + self.normal_size + 16
```

```end
        renderer.restore()
```

```end
    method is_running(app_id)
        // Check if application is running
        return false
```

```end
class StartMenu
    property x = 0
    property y = 0
    property width = 560
    property height = 700
    property visible = false
    property animation = "slide_up"
    property animation_duration = 200
    property background = "rgba(30,30,40,0.95)"
    property backdrop_filter = "blur(30px)"
    property border_radius = 16
    property user_image = ""
    property user_name = ""
    property user_email = ""
    property applications = {}
    property pinned_apps = []
    property power_options = {}

    method set_size(w, h)
        self.width = w
        self.height = h
```

```end
    method set_position(pos)
        // pos can be "bottom-left", "bottom-right", "center"
        self.position = pos
```

```end
    method set_animation(anim)
        self.animation = anim
```

```end
    method set_animation_duration(duration)
        self.animation_duration = duration
```

```end
    method set_background(bg)
        self.background = bg
```

```end
    method set_backdrop_filter(filter)
        self.backdrop_filter = filter
```

```end
    method set_border_radius(radius)
        self.border_radius = radius
```

```end
    method set_user_image(path)
        self.user_image = path
```

```end
    method set_user_name(name)
        self.user_name = name
```

```end
    method set_user_email(email)
        self.user_email = email
```

```end
    method set_applications(apps)
        self.applications = apps
```

```end
    method set_power_options(options)
        self.power_options = options
```

```end
    method add_pinned_app(app)
        self.pinned_apps.append(app)
```

```end
    method show()
        self.visible = true
        // Calculate position based on screen
        let screen_width = lowl.system.get_display_width()
        let screen_height = lowl.system.get_display_height()

        if self.position == "bottom-left"
            self.x = 20
            self.y = screen_height - self.height - 60
        else if self.position == "bottom-right"
            self.x = screen_width - self.width - 20
            self.y = screen_height - self.height - 60
```

```else
            self.x = (screen_width - self.width) / 2
            self.y = (screen_height - self.height) / 2
```

```end
        self.dispatch_event(MenuEvent.new("opened"))
```

```end
    method hide()
        self.visible = false
        self.dispatch_event(MenuEvent.new("closed"))
```

```end
    method is_visible()
        return self.visible
```

```end
    method toggle()
        if self.visible
            self.hide()
```

```else
            self.show()
```

```end
    method render(renderer)
        if not self.visible
```

```end
        renderer.save()
        renderer.set_opacity(1.0)

        renderer.fill_rounded_rect(self.x, self.y, self.width, self.height, self.border_radius, self.background)

        if self.backdrop_filter != ""
            renderer.apply_backdrop_filter(self.backdrop_filter)
```

```end
        // User section
        if self.user_image != ""
            let user_icon = renderer.load_image(self.user_image)
            renderer.draw_image(user_icon, self.x + 20, self.y + 20, 48, 48)
```

```else
            renderer.fill_rect(self.x + 20, self.y + 20, 48, 48, "rgba(255,255,255,0.2)")
```

```end
        renderer.draw_text(self.user_name, self.x + 80, self.y + 28, 16, "white")
        renderer.draw_text(self.user_email, self.x + 80, self.y + 48, 12, "rgba(255,255,255,0.6)")

        renderer.draw_line(self.x + 20, self.y + 80, self.x + self.width - 20, self.y + 80, 1, "rgba(255,255,255,0.1)")

        // Pinned apps section
        renderer.draw_text("Pinned", self.x + 20, self.y + 100, 14, "rgba(255,255,255,0.8)")

        let grid_x = self.x + 20
        let grid_y = self.y + 120
        let per_row = 4
        let icon_size = 64
        let spacing = 16
```

for i from 0 to self.pinned_apps.size() - 1

```end
            let app = self.pinned_apps[i]
            let col = i % per_row
            let row = i / per_row
            let icon_x = grid_x + col * (icon_size + spacing)
            let icon_y = grid_y + row * (icon_size + spacing + 20)

            let icon = renderer.load_image(app.icon)
            renderer.draw_image(icon, icon_x, icon_y, icon_size, icon_size)

            renderer.draw_text(app.name, icon_x, icon_y + icon_size + 4, 10, "rgba(255,255,255,0.8)", "center", icon_size)
```

```end
        // All apps section
        let all_apps_y = grid_y + 200
        renderer.draw_text("All Apps", self.x + 20, all_apps_y, 14, "rgba(255,255,255,0.8)")

        let app_list_y = all_apps_y + 20
```

for each name, app in self.applications

```end
            let list_icon = renderer.load_image(app.icon)
            renderer.draw_image(list_icon, self.x + 20, app_list_y, 32, 32)
            renderer.draw_text(name, self.x + 70, app_list_y + 10, 14, "rgba(255,255,255,0.9)")
            app_list_y = app_list_y + 48
```

```end
        // Power section
        let power_y = self.y + self.height - 80

        renderer.draw_line(self.x + 20, power_y - 10, self.x + self.width - 20, power_y - 10, 1, "rgba(255,255,255,0.1)")

        let power_x = self.x + 30
```

for each name, action in self.power_options

```end
            renderer.draw_text(name, power_x, power_y + 10, 12, "rgba(255,255,255,0.8)")
            power_x = power_x + 60 + renderer.measure_text(name, 12)
```

```end
        renderer.restore()
```

```end
    method add_event_listener(type, listener)
        if not self.event_listeners.has_key(type)
            self.event_listeners[type] = []
```

```end
        self.event_listeners[type].append(listener)
```

```end
    method dispatch_event(event)
        let type = event.get_type()
        if self.event_listeners.has_key(type)
```

for each listener in self.event_listeners[type]

```end
                listener(event)
```

```end
class SystemTray
    property x = 0
    property y = 0
    property width = 0
    property height = 40
    property position = "top-right"
    property background = "rgba(0,0,0,0.5)"
    property backdrop_filter = "blur(10px)"
    property icons = []
    property clock_text = ""
    property visible = true

    method set_position(pos)
        self.position = pos
```

```end
    method set_height(h)
        self.height = h
```

```end
    method set_width(w)
        self.width = w
```

```end
    method set_background(bg)
        self.background = bg
```

```end
    method set_backdrop_filter(filter)
        self.backdrop_filter = filter
```

```end
    method add_icon(id, icon_text, click_handler)
        let icon = {id: id, text: icon_text, handler: click_handler}
        self.icons.append(icon)
```

```end
    method remove_icon(id)
        self.icons = self.icons.filter(func(icon) icon.id != id end)
```

```end
    method set_clock(clock_callback)
        self.clock_callback = clock_callback
        self.update_clock()
```

```end
    method update_clock()
        if self.clock_callback != null
            self.clock_text = self.clock_callback()
```

```else
            let now = lowl.system.get_time()
            self.clock_text = now.format("%I:%M %p")
```

```end
        lowl.system.timer.interval(1000, func() self.update_clock() end)
```

```end
    method render(renderer)
        if not self.visible
```

```end
        let screen_width = renderer.get_width()

        if self.position == "top-right"
            self.x = screen_width - self.width - 10
            self.y = 10
        else if self.position == "top-left"
            self.x = 10
            self.y = 10
```

```end
        renderer.save()
        renderer.fill_rounded_rect(self.x, self.y, self.width, self.height, 12, self.background)

        if self.backdrop_filter != ""
            renderer.apply_backdrop_filter(self.backdrop_filter)
```

```end
        let icon_x = self.x + self.width - 30
```

for i from self.icons.size() - 1 down to 0

```end
            let icon = self.icons[i]
            renderer.draw_text(icon.text, icon_x, self.y + self.height / 2 - 10, 16, "white")
            icon_x = icon_x - 30
```

```end
        renderer.draw_text(self.clock_text, icon_x - 20, self.y + self.height / 2 - 10, 12, "white")

        renderer.restore()
```

```end
class NotificationCenter
    property x = 0
    property y = 0
    property width = 380
    property max_notifications = 10
    property timeout = 5
    property visible = false
    property background = "rgba(0,0,0,0.85)"
    property backdrop_filter = "blur(20px)"
    property border_radius = 16
    property notifications = []

    method set_position(pos)
        self.position = pos
```

```end
    method set_width(w)
        self.width = w
```

```end
    method set_max_notifications(max)
        self.max_notifications = max
```

```end
    method set_timeout(sec)
        self.timeout = sec
```

```end
    method set_background(bg)
        self.background = bg
```

```end
    method set_backdrop_filter(filter)
        self.backdrop_filter = filter
```

```end
    method set_border_radius(radius)
        self.border_radius = radius
```

```end
    method set_visible(vis)
        self.visible = vis
```

```end
    method is_visible()
        return self.visible
```

```end
    method add_notification(title, message, type)
        let notif = {
            id: lowl.utils.uuid.generate_v4(),
            title: title,
            message: message,
            type: type,
            timestamp: lowl.system.get_time(),
            dismissed: false
        }

        self.notifications.insert(0, notif)

        if self.notifications.size() > self.max_notifications
            self.notifications.pop()
```

```end
        if self.visible
            self.dispatch_event(NotificationEvent.new("notification_added"))
```

```end
        if self.timeout > 0
            lowl.system.timer.delay(self.timeout * 1000, func()
                self.dismiss_notification(notif.id)
            end)
```

```end
        return notif
```

```end
    method dismiss_notification(id)
```

for each notif in self.notifications

```end
            if notif.id == id
                notif.dismissed = true
                self.notifications = self.notifications.filter(func(n) n.id != id end)
```

```end
        self.dispatch_event(NotificationEvent.new("notification_dismissed"))
```

```end
    method clear_all()
        self.notifications.clear()
        self.dispatch_event(NotificationEvent.new("cleared"))
```

```end
    method render(renderer)
        if not self.visible
```

```end
        let screen_width = renderer.get_width()
        let screen_height = renderer.get_height()

        if self.position == "top-right"
            self.x = screen_width - self.width - 10
            self.y = 60
```

```end
        renderer.save()

        renderer.fill_rounded_rect(self.x, self.y, self.width, self.height, self.border_radius, self.background)

        if self.backdrop_filter != ""
            renderer.apply_backdrop_filter(self.backdrop_filter)
```

```end
        let header_y = self.y + 16
        renderer.draw_text("Notifications", self.x + 16, header_y, 16, "white")

        renderer.draw_line(self.x + 16, header_y + 24, self.x + self.width - 16, header_y + 24, 1, "rgba(255,255,255,0.1)")

        let notif_y = header_y + 36
```

for each notif in self.notifications

```end
            // Determine icon based on type
            let icon = "ℹ️"
            if notif.type == "success"
                icon = "✅"
            else if notif.type == "error"
                icon = "❌"
            else if notif.type == "warning"
                icon = "⚠️"
```

```end
            renderer.draw_text(icon, self.x + 16, notif_y, 20)
            renderer.draw_text(notif.title, self.x + 48, notif_y, 14, "white")
            renderer.draw_text(notif.message, self.x + 48, notif_y + 18, 11, "rgba(255,255,255,0.6)")

            let time_str = lowl.system.format_time_relative(notif.timestamp)

            renderer.draw_text(time_str, self.x + self.width - 50, notif_y + 4, 10, "rgba(255,255,255,0.4)")

            notif_y = notif_y + 60
```

```end
        renderer.restore()
```

```end
class Workspace
    property name = ""
    property index = 0
    property active = false
    property background_color = "rgba(0,0,0,0.95)"

    method set_name(n)
        self.name = n
```

```end
    method get_name()
        return self.name
```

```end
    method set_index(idx)
        self.index = idx
```

```end
    method get_index()
        return self.index
```

```end
    method set_active(active_flag)
        self.active = active_flag
```

```end
    method is_active()
        return self.active
```

```end
    method set_background_color(color)
        self.background_color = color
```

```end
class DesktopEvent
    property type = ""
    property data = null

    method new(event_type)
        self.type = event_type
        return self
```

```end
    method get_type()
        return self.type
```

```end
    method set_data(d)
        self.data = d
```

```end
    method get_data()
        return self.data
```

```end
class MenuEvent extends DesktopEvent
    method new(event_type)
        super(event_type)
        return self
```

```end
class NotificationEvent extends DesktopEvent
    method new(event_type)
        super(event_type)
        return self
```

```end
Summary
```

This complete implementation provides:

Component  Description Key Features

NebulaPlatform Boot controller

Configuration loading, security init, hardware detection, kernel loading

POSIX Kernel  lowlPython kernel Process mgmt, memory mgmt, scheduling, file system,

LOWLUX Document DOM system Document, Element, styles, events, serialization

LOWLUX Container Window system VisualContainer, ShapeContainer, shape definitions

LOWLUX Desktop Desktop manager Container mgmt, workspace mgmt, lock screen,

```rendering
LOWLUX UI
The entire system runs in pure LOWL script with no C++ components. The POSIX kernel is written in lowlPython and loaded as a module. The desktop uses LOWLUX DOM for all visual elements. The boot controller orchestrates everything.
```

Full POSIX-Compatible Kernel with Process Scheduler

Target: Intel x86_64 | Language: lowlPython | Platform: NEBULA OS / Bare Metal

Table of Contents

System Overview

```rendering
Process Control Block (PCB)

Scheduler Core

Process Management

Memory Management

File System

System Calls

Interrupt Handling

Context Switching

POSIX Signal Handling

Timer and Time Management
IPC Mechanisms

Boot and Initialization

Complete Example

Build Configuration
```

## Part 1: System Overview

```lowlpython
# File: posix/kernel.lp
# Complete POSIX Kernel for Intel x86_64

module POSIXKernel version(1,0,0) author(NEBULA_OS):

    metadata:
        name = "POSIX Kernel"
        version = "1.0.0"
        platform = "x86_64"
        preemptive = true
        tick_hz = 1000  # 1kHz timer interrupt

    dependencies:
        system.cpu,
        system.memory,
        system.interrupt,
        system.timer,
        drivers.uart,
        drivers.ps2_keyboard

    exports:
        Kernel,
        Scheduler,
        Process,
        Thread,
        MemoryManager,
        FileSystem,
        SyscallHandler,
        SignalHandler,
        IPCManager,
        TimerManager,
        posix_kernel_init

    published kernel: ptr[Kernel] = null
    published scheduler: ptr[Scheduler] = null

register_driver(POSIXKernel)
```

## Part 2: Process Control Block (PCB)

```lowlpython
# File: posix/process.lp

class ProcessState(Enum):
    NEW = 0
    READY = 1
    RUNNING = 2
    WAITING = 3
    TERMINATED = 4
    ZOMBIE = 5
    STOPPED = 6

class SchedulingPolicy(Enum):
```

FIFO = 0      # First In First Out

RR = 1        # Round Robin

CFS = 2       # Completely Fair Scheduler

class ProcessPriority:

```lowlpython
    IDLE: i8 = -20
    LOW: i8 = 0
    NORMAL: i8 = 10
    HIGH: i8 = 20
    REALTIME: i8 = 30
    CRITICAL: i8 = 40

class ProcessFlags:
```

PF_KTHREAD: u32 = 0x00000001   # Kernel thread

PF_USER: u32 = 0x00000002      # User process

PF_SIGNALED: u32 = 0x00000004  # Signal pending

PF_EXITING: u32 = 0x00000008   # Process exiting

PF_FORKED: u32 = 0x00000010    # New forked process

PF_TRACED: u32 = 0x00000020    # Being traced

class ContextRegisters:

```lowlpython
    # General purpose registers
    rax: u64 = 0
    rbx: u64 = 0
    rcx: u64 = 0
    rdx: u64 = 0
    rsi: u64 = 0
    rdi: u64 = 0
    rbp: u64 = 0
    rsp: u64 = 0
    r8: u64 = 0
    r9: u64 = 0
    r10: u64 = 0
    r11: u64 = 0
    r12: u64 = 0
    r13: u64 = 0
    r14: u64 = 0
    r15: u64 = 0

    # Segment registers
    cs: u16 = 0
    ds: u16 = 0
    es: u16 = 0
    fs: u16 = 0
    gs: u16 = 0
    ss: u16 = 0

    # Control registers
    rip: u64 = 0
    rflags: u64 = 0
    cr3: u64 = 0   # Page table base

    # FPU/SSE state
    fpu_state: u8[512] = [0; 512]  # 512 bytes for FXSAVE/FXRSTOR

class ProcessControlBlock:
    # Identification
    pid: i32 = -1
    ppid: i32 = -1
    tgid: i32 = -1   # Thread group ID
    uid: u32 = 0
    gid: u32 = 0
    euid: u32 = 0
    egid: u32 = 0

    # State
    state: ProcessState = ProcessState.NEW
    flags: u32 = 0
    exit_code: i32 = 0
    exit_signal: u8 = 0

    # Scheduling
    priority: i8 = ProcessPriority.NORMAL
    nice: i8 = 0
    policy: SchedulingPolicy = SchedulingPolicy.RR
    time_slice: u32 = 0
```

vruntime: u64 = 0         # Virtual runtime for CFS

```lowlpython
    last_ran: u64 = 0         # Last run timestamp

    # Context
    registers: ContextRegisters = ContextRegisters()
    kernel_stack: ptr[u8] = null
    user_stack: ptr[u8] = null
    page_table: ptr[PageTable] = null

    # Memory
    text_start: u64 = 0
    text_end: u64 = 0
    data_start: u64 = 0
    data_end: u64 = 0
    heap_start: u64 = 0
    heap_end: u64 = 0
    stack_start: u64 = 0
    stack_end: u64 = 0

    # File descriptors
    fd_table: ptr[FileDescriptor][256] = null
    cwd: ptr[DirectoryEntry] = null

    # Signals
    pending_signals: u64 = 0      # Signal mask (64 signals)
    signal_handlers: ptr[SignalHandler][64] = null
    sigmask: u64 = 0

    # Timing
    start_time: u64 = 0
    user_time: u64 = 0
    system_time: u64 = 0
    children_user_time: u64 = 0
    children_system_time: u64 = 0

    # Resource limits
    rlim_cpu: u64 = 0xFFFFFFFF   # CPU time limit
    rlim_fsize: u64 = 0xFFFFFFFF  # File size limit
    rlim_data: u64 = 0xFFFFFFFF   # Data segment limit
    rlim_stack: u64 = 0xFFFFFFFF  # Stack limit
    rlim_core: u64 = 0xFFFFFFFF   # Core dump size
    rlim_nofile: u64 = 256        # Open file limit

    # IPC
    message_queue: ptr[MessageQueue] = null
    shared_memory: ptr[SharedMemory] = null
    semaphores: ptr[Semaphore][32] = null

    # Children
    children: ptr[ProcessControlBlock][256] = null
    child_count: u32 = 0
    sibling_next: ptr[ProcessControlBlock] = null
    sibling_prev: ptr[ProcessControlBlock] = null

    # Waiting queue
    wait_queue: ptr[WaitQueue] = null

    def init(self, pid: i32) -> void:
        self.pid = pid
        self.state = ProcessState.NEW
        self.flags = 0
        self.exit_code = 0
        self.start_time = rdtsc()

        # Allocate kernel stack (16KB)
        self.kernel_stack = physical_alloc(16384, 4096)

        # Initialize file descriptors
        self.fd_table = physical_alloc(256 * 8, 8)
```

for i in range(256):

```lowlpython
            self.fd_table[i] = null

        # Initialize signal handlers
        self.signal_handlers = physical_alloc(64 * 8, 8)
```

for i in range(64):

```lowlpython
            self.signal_handlers[i] = null

        # Initialize IPC
        self.message_queue = MessageQueue.new()
        self.shared_memory = SharedMemory.new()
```

## Part 3: Scheduler Core

```lowlpython
# File: posix/scheduler.lp
class Scheduler:

    # Run queues for each priority level (41 levels: -20 to 20)
    private run_queues: ptr[RunQueue][41] = null
    private expired_queues: ptr[RunQueue][41] = null
    private current_queue: ptr[RunQueue] = null

    # All processes list
    private all_processes: ptr[ProcessControlBlock] = null
    private process_count: u32 = 0
    private next_pid: i32 = 1

    # Current running process
    private current: ptr[ProcessControlBlock] = null
    private idle_process: ptr[ProcessControlBlock] = null

    # Timekeeping
    private ticks: u64 = 0
    private scheduler_tick_count: u64 = 0
    private load_avg: f64 = 0.0

    # Scheduling parameters
    private min_timeslice: u32 = 10      # ms
    private max_timeslice: u32 = 100     # ms
    private default_timeslice: u32 = 50  # ms

    class RunQueue:
        head: ptr[ProcessControlBlock] = null
        tail: ptr[ProcessControlBlock] = null
        count: u32 = 0
        def enqueue(self, proc: ptr[ProcessControlBlock]) -> void:
            proc.sibling_next = null
            if self.tail == null:
                self.head = proc
                self.tail = proc
            else:
                self.tail.sibling_next = proc
                self.tail = proc
            self.count = self.count + 1

        def dequeue(self) -> ptr[ProcessControlBlock]:
            if self.head == null:
                return null
            proc = self.head
            self.head = proc.sibling_next
            if self.head == null:
                self.tail = null
            self.count = self.count - 1
            return proc

        def remove(self, proc: ptr[ProcessControlBlock]) -> bool:
            if self.head == null:
                return false

            if self.head == proc:
                self.head = proc.sibling_next
                if self.head == null:
                    self.tail = null
                self.count = self.count - 1
                return true
            curr = self.head
```

while curr != null and curr.sibling_next != proc:

```lowlpython
                curr = curr.sibling_next

            if curr != null:
                curr.sibling_next = proc.sibling_next
                if self.tail == proc:
                    self.tail = curr
                self.count = self.count - 1
                return true
            return false

    def init(self) -> void:
        # Initialize run queues
```

for i in range(41):

```lowlpython
            self.run_queues[i] = RunQueue()
            self.expired_queues[i] = RunQueue()

        self.current_queue = self.run_queues

        # Create idle process
        self.idle_process = self.create_idle_process()
        self.current = self.idle_process

        # Register timer interrupt handler
        register_interrupt(0x20, self.timer_interrupt)

        # Set scheduler tick rate
        self.set_timer_frequency(1000)  # 1kHz

    def create_idle_process(self) -> ptr[ProcessControlBlock]:
        idle = ProcessControlBlock()
        idle.init(0)
        idle.state = ProcessState.RUNNING
        idle.flags = idle.flags | ProcessFlags.PF_KTHREAD
        idle.priority = ProcessPriority.IDLE
        idle.policy = SchedulingPolicy.FIFO
        idle.registers.rip = self.idle_loop.as_u64()
        return idle

    def idle_loop(self) -> void:
        while true:
            disable_interrupts()
```

if not self.has_runnable_processes():

```lowlpython
                halt()  # No processes, halt CPU
            enable_interrupts()
            pause()

    def schedule(self) -> ptr[ProcessControlBlock]:
        disable_interrupts()

        # Save current process state
        if self.current != null and self.current.state == ProcessState.RUNNING:
            self.current.registers.rsp = read_rsp()
            self.current.state = ProcessState.READY
            self.enqueue_process(self.current)

        # Find next process to run
        next_proc = self.pick_next_process()

        if next_proc == null:
            next_proc = self.idle_process
        if next_proc == self.current:
            enable_interrupts()
            return self.current

        next_proc.state = ProcessState.RUNNING
        next_proc.last_ran = self.ticks

        # Context switch
        let old = self.current
        self.current = next_proc

        self.switch_context(old, next_proc)

        enable_interrupts()
        return self.current

    def pick_next_process(self) -> ptr[ProcessControlBlock]:
        # Find highest priority runnable process
```

for priority in range(40, -1, -1):

```lowlpython
            let queue = self.current_queue[priority]
            if queue.count > 0:
                proc = queue.dequeue()
                if proc != null and proc.state == ProcessState.READY:
                    return self.calculate_timeslice(proc)

        # If current queue empty, swap with expired queue
        if self.current_queue == self.run_queues:
            if self.expired_queues_has_processes():
                self.swap_queues()
                return self.pick_next_process()
        return null

    def calculate_timeslice(self, proc: ptr[ProcessControlBlock]) -> ptr[ProcessControlBlock]:
        # Calculate timeslice based on priority
```

let base = (proc.priority + 20).as_u32()  # 0 to 40

```lowlpython
        let slice = self.min_timeslice + (base * (self.max_timeslice - self.min_timeslice) / 40)
        proc.time_slice = slice
        return proc

    def check_timeslice_expired(self, proc: ptr[ProcessControlBlock]) -> bool:
        if proc.policy == SchedulingPolicy.FIFO:
            return false  # FIFO processes run until they yield

        let elapsed = self.ticks - proc.last_ran
        if elapsed >= proc.time_slice:
            return true
        return false

    def update_load_average(self) -> void:
        let runnable = self.get_runnable_count()
        let current_load = runnable.as_f64() / self.process_count.as_f64()
        self.load_avg = self.load_avg * 0.8 + current_load * 0.2

    def get_runnable_count(self) -> u32:
        count = 0
```

for i in range(41):

```lowlpython
            count = count + self.current_queue[i].count
        return count

    def expired_queues_has_processes(self) -> bool:
```

for i in range(41):

```lowlpython
            if self.expired_queues[i].count > 0:
                return true
        return false

    def swap_queues(self) -> void:
        self.current_queue = self.expired_queues
        self.expired_queues = self.run_queues
        self.run_queues = self.current_queue
        self.current_queue = self.run_queues

    def enqueue_process(self, proc: ptr[ProcessControlBlock]) -> void:
        if proc.priority < -20:
            proc.priority = -20
        if proc.priority > 20:
            proc.priority = 20

        let idx = (proc.priority + 20).as_u32()

        if proc.time_slice > 0:
            self.current_queue[idx].enqueue(proc)
        else:
            # Reset timeslice and move to expired queue
            self.calculate_timeslice(proc)
            self.expired_queues[idx].enqueue(proc)

    def dequeue_process(self, proc: ptr[ProcessControlBlock]) -> bool:
        let idx = (proc.priority + 20).as_u32()
        if self.current_queue[idx].remove(proc):
            return true
        return self.expired_queues[idx].remove(proc)
    def set_timer_frequency(self, hz: u32) -> void:
        let divisor = 1193180 / hz
        outb(0x43, 0x36)
        outb(0x40, (divisor & 0xFF).as_u8())
        outb(0x40, (divisor >> 8).as_u8())

    @interrupt_handler(vector=0x20)
    def timer_interrupt(self) -> void:
        self.ticks = self.ticks + 1
        self.scheduler_tick_count = self.scheduler_tick_count + 1

        # Update current process time
        if self.current != null and self.current.state == ProcessState.RUNNING:
            if self.current.flags & ProcessFlags.PF_KTHREAD:
                self.current.system_time = self.current.system_time + 1
            else:
                self.current.user_time = self.current.user_time + 1

        # Check if timeslice expired
```

if self.current != null and self.current != self.idle_process:

```lowlpython
            if self.check_timeslice_expired(self.current):
                self.schedule()

        # Update load average every 100 ticks
        if self.scheduler_tick_count >= 100:
            self.update_load_average()
            self.scheduler_tick_count = 0

        # Send EOI to PIC
        outb(0x20, 0x20)
    def switch_context(self, old: ptr[ProcessControlBlock], new: ptr[ProcessControlBlock]) -> void:
        # Save FPU state
        asm("fxsave [%0]" : : "r"(old.registers.fpu_state))

        # Switch page tables
        if old.page_table != new.page_table:
            asm("mov cr3, %0" : : "r"(new.page_table))

        # Restore FPU state
        asm("fxrstor [%0]" : : "r"(new.registers.fpu_state))

        # Switch stack and restore registers
        asm("""
            mov rsp, %0
            pop r15
            pop r14
            pop r13
            pop r12
            pop r11
            pop r10
            pop r9
            pop r8
            pop rbp
            pop rdi
            pop rsi
            pop rdx
            pop rcx
            pop rbx
            pop rax
```

```iretq
        """ : : "r"(new.registers.rsp))

    def yield_cpu(self) -> void:
        self.schedule()

    def has_runnable_processes(self) -> bool:
```

for i in range(41):

```iretq
            if self.current_queue[i].count > 0:
                return true
        return false
```

## Part 4: Process Management

```lowlpython
# File: posix/process_mgmt.lp

class ProcessManager:

    private kernel: ptr[Kernel] = null
    private scheduler: ptr[Scheduler] = null
    private init_process: ptr[ProcessControlBlock] = null

    def init(self) -> void:
        self.scheduler = Scheduler.get_instance()

        # Create init process (PID 1)
        self.init_process = self.create_kernel_thread(self.init_task, "init")
        self.init_process.state = ProcessState.READY
        self.scheduler.enqueue_process(self.init_process)

    def init_task(self) -> void:
        # Mount root filesystem
        fs = FileSystem()
        fs.mount_root()
        # Run init script or shell
```

if not self.run_init_script():

```lowlpython
            self.run_shell()

        # Wait for children
        while true:
            let status = self.waitpid(-1, null, 0)
            if status < 0:
```

```break
    def fork(self) -> i32:
        let parent = self.scheduler.get_current()
        let child = self.duplicate_process(parent)

        if child == null:
            return -1

        child.pid = self.allocate_pid()
        child.ppid = parent.pid
        child.state = ProcessState.READY

        # Add to parent's children list
        parent.children[parent.child_count] = child
        parent.child_count = parent.child_count + 1

        self.scheduler.enqueue_process(child)

        return child.pid

    def duplicate_process(self, parent: ptr[ProcessControlBlock]) -> ptr[ProcessControlBlock]:
        let child = ProcessControlBlock()

        # Copy basic attributes
        child.uid = parent.uid
        child.gid = parent.gid
        child.euid = parent.euid
        child.egid = parent.egid
        child.nice = parent.nice
        child.priority = parent.priority
        child.policy = parent.policy
        child.flags = parent.flags & ~ProcessFlags.PF_FORKED

        # Copy memory (copy-on-write)
        child.page_table = self.duplicate_page_table(parent.page_table)
        child.registers = parent.registers
        child.registers.rax = 0  # Return 0 to child

        # Allocate new kernel stack
        child.kernel_stack = physical_alloc(16384, 4096)

        # Duplicate file descriptors
```

for i in range(256):

```break
            if parent.fd_table[i] != null:
                child.fd_table[i] = self.duplicate_fd(parent.fd_table[i])

        # Copy working directory
        child.cwd = parent.cwd

        # Copy signal handlers
```

for i in range(64):

```break
            child.signal_handlers[i] = parent.signal_handlers[i]
        child.sigmask = parent.sigmask

        return child.as_ptr()

    def execve(self, path: ptr[u8], argv: ptr[ptr[u8]], envp: ptr[ptr[u8]]) -> i32:
        let proc = self.scheduler.get_current()

        # Load binary
        let binary = self.load_binary(path)
        if binary == null:
            return -1

        # Clear old memory
        self.clear_process_memory(proc)

        # Setup new memory image
        self.setup_process_memory(proc, binary)

        # Setup arguments and environment on stack
        self.setup_stack(proc, argv, envp)

        # Reset signal handlers
```

for i in range(64):

if i != 0x0B:  # Keep SIGSEGV handler?

```break
                proc.signal_handlers[i] = null

        # Reset time accounting
        proc.user_time = 0
        proc.system_time = 0
        # Set entry point
        proc.registers.rip = binary.entry_point

        return 0

    def exit(self, code: i32) -> void:
        let proc = self.scheduler.get_current()
        let parent = self.find_process(proc.ppid)

        proc.state = ProcessState.ZOMBIE
        proc.exit_code = code

        # Close all file descriptors
```

for i in range(256):

```break
            if proc.fd_table[i] != null:
                self.close_fd(proc.fd_table[i])

        # Wake parent waiting for this process
```

if parent != null and parent.wait_queue != null:

```break
            parent.wait_queue.wake_all()

        # Reparent children to init
```

for i in range(proc.child_count):

```break
            let child = proc.children[i]
            child.ppid = 1
            let init = self.find_process(1)
            if init != null:
                init.children[init.child_count] = child
                init.child_count = init.child_count + 1

        self.scheduler.schedule()  # Never returns
    def waitpid(self, pid: i32, status: ptr[i32], options: u32) -> i32:
        let proc = self.scheduler.get_current()

        while true:
            # Check for zombie children
```

for i in range(proc.child_count):

```break
                let child = proc.children[i]
                if child.state == ProcessState.ZOMBIE:
                    if pid == -1 or child.pid == pid:
                        if status != null:
                            status.value = child.exit_code
                        self.reap_process(child)
                        return child.pid

            # No matching zombie, wait if NOHANG not set
            if options & 0x01:  # WNOHANG
                return 0

            # Wait for child
            proc.wait_queue.wait()

    def kill(self, pid: i32, sig: u8) -> i32:
        let proc = self.find_process(pid)
        if proc == null:
            return -1

        if proc.state == ProcessState.ZOMBIE:
            return -1

        # Set signal pending
        if sig > 0:
            proc.pending_signals = proc.pending_signals | (1 << (sig - 1))

        # If signal is fatal, terminate process
        if self.is_fatal_signal(sig):
            proc.state = ProcessState.TERMINATED
            self.exit(sig)

        return 0

    def getpid(self) -> i32:
        return self.scheduler.get_current().pid

    def getppid(self) -> i32:
        return self.scheduler.get_current().ppid

    def getuid(self) -> u32:
        return self.scheduler.get_current().uid

    def getgid(self) -> u32:
        return self.scheduler.get_current().gid

    def setpriority(self, which: u32, who: i32, prio: i8) -> i32:
        let proc = self.get_process_by_id(who)
        if proc == null:
            return -1

        if prio < -20:
            prio = -20
        if prio > 20:
            prio = 20
        proc.nice = prio
        proc.priority = prio
        return 0

    def nice(self, increment: i32) -> i32:
        let proc = self.scheduler.get_current()
        let new_prio = proc.priority + increment
        if new_prio > 20:
            new_prio = 20
        if new_prio < -20:
            new_prio = -20
        proc.priority = new_prio
        return new_prio

    def sched_yield(self) -> void:
        self.scheduler.yield_cpu()

    def sched_setaffinity(self, pid: i32, mask: u64) -> i32:
        let proc = self.find_process(pid)
        if proc == null:
            return -1
        proc.affinity_mask = mask
        return 0

    private def allocate_pid(self) -> i32:
        let pid = self.next_pid
        self.next_pid = self.next_pid + 1
        return pid

    private def find_process(self, pid: i32) -> ptr[ProcessControlBlock]:
        # Linear search - can be optimized with hash table
        let proc = self.all_processes
        while proc != null:
            if proc.pid == pid:
                return proc
            proc = proc.next
        return null

    private def reap_process(self, proc: ptr[ProcessControlBlock]) -> void:
        # Free resources
        physical_free(proc.kernel_stack)
        physical_free(proc.page_table)
        physical_free(proc.fd_table)
        physical_free(proc.signal_handlers)

        # Remove from parent's children list
        let parent = self.find_process(proc.ppid)
        if parent != null:
```

for i in range(parent.child_count):

```break
                if parent.children[i] == proc:
                    parent.children[i] = null
```

## Part 5: Memory Management

```lowlpython
# File: posix/memory.lp

class MemoryManager:

    # Page sizes
    const PAGE_SIZE_4K: u64 = 4096
    const PAGE_SIZE_2M: u64 = 2097152
    const PAGE_SIZE_1G: u64 = 1073741824
    # Page flags (Intel x86_64)
    const PAGE_PRESENT: u64 = 0x00000001
    const PAGE_WRITABLE: u64 = 0x00000002
    const PAGE_USER: u64 = 0x00000004
    const PAGE_WRITETHROUGH: u64 = 0x00000008
    const PAGE_CACHE_DISABLE: u64 = 0x00000010
    const PAGE_ACCESSED: u64 = 0x00000020
    const PAGE_DIRTY: u64 = 0x00000040
    const PAGE_HUGE: u64 = 0x00000080
    const PAGE_GLOBAL: u64 = 0x00000100

    # Physical memory management
    private phys_memory_size: u64 = 0
    private free_pages_bitmap: ptr[u8] = null
    private total_pages: u64 = 0
    private free_pages: u64 = 0

    # Virtual memory management
    private kernel_page_table: ptr[PageTable] = null
    private process_page_tables: ptr[PageTable] = null

    # Memory zones
    const ZONE_DMA: u32 = 0      # < 16MB
    const ZONE_NORMAL: u32 = 1   # 16MB - 896MB
    const ZONE_HIGHMEM: u32 = 2  # > 896MB

    class Page:
        flags: u32 = 0
        order: u8 = 0            # Allocation order (2^order pages)
        refcount: u32 = 0
        virtual: ptr = null
        physical: u64 = 0
        next: ptr[Page] = null

    class PageTable:
        entries: ptr[u64][512] = null  # 512 entries per level

        def init(self, is_kernel: bool) -> void:
            self.entries = physical_alloc(4096, 4096)
            set_memory(self.entries.as_ptr(), 0, 4096)

    def init(self) -> void:
        # Detect physical memory from BIOS E820 map
        self.phys_memory_size = self.detect_memory()
        self.total_pages = self.phys_memory_size / PAGE_SIZE_4K
        self.free_pages = self.total_pages

        # Initialize page bitmap (1 bit per page)
        let bitmap_size = self.total_pages / 8
        self.free_pages_bitmap = physical_alloc(bitmap_size, 4096)
        set_memory(self.free_pages_bitmap, 0xFF, bitmap_size)  # All free initially

        # Initialize kernel page tables
        self.kernel_page_table = PageTable()
        self.kernel_page_table.init(true)

        # Map kernel text, data, stack
        self.map_kernel_regions()

        # Initialize memory zones
        self.init_memory_zones()
    def allocate_page(self) -> u64:
        # Find free page in bitmap
```

for i in range(self.total_pages):

```lowlpython
            let byte = i / 8
            let bit = i % 8
            if (self.free_pages_bitmap[byte] >> bit) & 1:
                self.free_pages_bitmap[byte] = self.free_pages_bitmap[byte] & ~(1 << bit)
                self.free_pages = self.free_pages - 1
                return i * PAGE_SIZE_4K
```

return 0  # Out of memory

```lowlpython
    def free_page(self, phys: u64) -> void:
        let page_num = phys / PAGE_SIZE_4K
        let byte = page_num / 8
        let bit = page_num % 8
        self.free_pages_bitmap[byte] = self.free_pages_bitmap[byte] | (1 << bit)
        self.free_pages = self.free_pages + 1

    def allocate_pages(self, order: u8) -> u64:
        let count = (1 << order).as_u64()
        let start = self.allocate_contiguous_pages(count)
        if start == 0:
            return 0

        # Mark page as allocated and set order
```

for i in range(count):

```lowlpython
            self.mark_page_allocated(start + i * PAGE_SIZE_4K, order)

        return start
    def allocate_contiguous_pages(self, count: u64) -> u64:
        let contiguous = 0
        let found = 0
```

for i in range(self.total_pages):

```lowlpython
            let byte = i / 8
            let bit = i % 8
            if (self.free_pages_bitmap[byte] >> bit) & 1:
                if contiguous == 0:
                    found = i
                contiguous = contiguous + 1
                if contiguous >= count:
                    return found * PAGE_SIZE_4K
            else:
                contiguous = 0
        return 0

    def setup_user_page_table(self, proc: ptr[ProcessControlBlock]) -> ptr[PageTable]:
        let page_table = PageTable()
        page_table.init(false)

        # Clone kernel entries from kernel page table
```

for i in range(256):  # User space is lower half

```lowlpython
            page_table.entries[i] = 0
```

for i in range(256, 512):  # Kernel space is upper half

```lowlpython
            page_table.entries[i] = self.kernel_page_table.entries[i]

        proc.page_table = page_table.as_ptr()
        return page_table.as_ptr()

    def map_page(self, page_table: ptr[PageTable], virtual: u64, physical: u64, flags: u64) -> void:
        let pml4_index = (virtual >> 39) & 0x1FF
        let pdp_index = (virtual >> 30) & 0x1FF
        let pd_index = (virtual >> 21) & 0x1FF
        let pt_index = (virtual >> 12) & 0x1FF

        # Get or create PDP table
        let pdp = self.get_or_create_pdp(page_table, pml4_index)
        if pdp == null:
```

```return
        # Get or create PD table
        let pd = self.get_or_create_pd(pdp, pdp_index)
        if pd == null:
```

```return
        # Get or create PT table
        let pt = self.get_or_create_pt(pd, pd_index)
        if pt == null:
```

```return
        # Set page table entry
        pt[pt_index] = (physical & ~0xFFF) | flags
        self.invalidate_tlb(virtual)

    def unmap_page(self, page_table: ptr[PageTable], virtual: u64) -> void:
        let pml4_index = (virtual >> 39) & 0x1FF
        let pdp_index = (virtual >> 30) & 0x1FF
        let pd_index = (virtual >> 21) & 0x1FF
        let pt_index = (virtual >> 12) & 0x1FF

        let pdp = self.get_pdp(page_table, pml4_index)
        if pdp == null: return

        let pd = self.get_pd(pdp, pdp_index)
        if pd == null: return

        let pt = self.get_pt(pd, pd_index)
        if pt == null: return

        pt[pt_index] = 0
        self.invalidate_tlb(virtual)

    def handle_page_fault(self, fault_addr: u64, error_code: u64) -> bool:
        let proc = scheduler.get_current()

        # Check if address is in process memory region
```

if fault_addr >= proc.heap_start and fault_addr < proc.heap_end:

```return
            # Demand page allocation for heap
            let page = self.allocate_page()
            if page == 0:
                return false

            self.map_page(proc.page_table, fault_addr & ~0xFFF, page,
```

PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)

```return
            return true
```

if fault_addr >= proc.stack_start and fault_addr < proc.stack_end:

```return
            # Demand page allocation for stack
            let page = self.allocate_page()
            if page == 0:
                return false
            self.map_page(proc.page_table, fault_addr & ~0xFFF, page,
```

PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)

```return
            return true

        # Check for copy-on-write
        if error_code & 0x02:  # Write fault
            let old_page = self.get_physical_address(proc.page_table, fault_addr)
            if old_page != 0:
                let new_page = self.allocate_page()
                if new_page != 0:
                    copy_memory(new_page.as_ptr(), old_page.as_ptr(), PAGE_SIZE_4K)
                    self.map_page(proc.page_table, fault_addr & ~0xFFF, new_page,
```

PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)

```return
                    self.free_page(old_page)
                    return true

        return false

    def get_physical_address(self, page_table: ptr[PageTable], virtual: u64) -> u64:
        let pml4_index = (virtual >> 39) & 0x1FF
        let pdp_index = (virtual >> 30) & 0x1FF
        let pd_index = (virtual >> 21) & 0x1FF
        let pt_index = (virtual >> 12) & 0x1FF

        let pdp = self.get_pdp(page_table, pml4_index)
        if pdp == null: return 0

        let pd = self.get_pd(pdp, pdp_index)
        if pd == null: return 0

        let pt = self.get_pt(pd, pd_index)
        if pt == null: return 0

        let pte = pt[pt_index]
        if pte & PAGE_PRESENT:
            return pte & ~0xFFF
        return 0

    def copy_on_write(self, src_page_table: ptr[PageTable], dst_page_table: ptr[PageTable],
                     virtual: u64) -> bool:
        let phys = self.get_physical_address(src_page_table, virtual)
        if phys == 0:
            return false

        # Mark source page as read-only (COW)
        self.unmap_page(src_page_table, virtual)
        self.map_page(src_page_table, virtual, phys, PAGE_PRESENT | PAGE_USER)

        # Map destination page to same physical (COW)
        self.map_page(dst_page_table, virtual, phys, PAGE_PRESENT | PAGE_USER)

        return true

    def mmap(self, addr: ptr, length: u64, prot: u32, flags: u32, fd: i32, offset: u64) -> ptr:
        let proc = scheduler.get_current()
        let page_aligned_len = (length + PAGE_SIZE_4K - 1) & ~(PAGE_SIZE_4K - 1)

        # Find free virtual address region
        let virtual = self.find_free_region(proc, addr, page_aligned_len)
        if virtual == 0:
            return null
        # Allocate physical pages
```

for off in range(0, page_aligned_len, PAGE_SIZE_4K):

```return
            let page = self.allocate_page()
            if page == 0:
                return null
            self.map_page(proc.page_table, virtual + off, page,
                         self.prot_to_flags(prot, flags))

        # If file mapping, read data
        if fd >= 0:
            let file = proc.fd_table[fd]
            if file != null:
                file.read(virtual.as_ptr(), length, offset)

        return virtual.as_ptr()

    def munmap(self, addr: ptr, length: u64) -> i32:
        let proc = scheduler.get_current()
        let page_aligned_len = (length + PAGE_SIZE_4K - 1) & ~(PAGE_SIZE_4K - 1)
```

for off in range(0, page_aligned_len, PAGE_SIZE_4K):

```return
            let virtual = addr.as_u64() + off
            let phys = self.get_physical_address(proc.page_table, virtual)
            if phys != 0:
                self.unmap_page(proc.page_table, virtual)
                self.free_page(phys)

        return 0

    def brk(self, new_break: u64) -> u64:
        let proc = scheduler.get_current()
        let old_break = proc.heap_end

        if new_break > old_break:
            # Expand heap
            let pages_needed = (new_break - old_break + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K
```

for i in range(pages_needed):

```return
                let page = self.allocate_page()
                if page == 0:
                    return old_break  # Failed, return old break
                let addr = old_break + i * PAGE_SIZE_4K
                self.map_page(proc.page_table, addr, page,
```

PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)

```return
        elif new_break < old_break:
            # Shrink heap
            let pages_to_free = (old_break - new_break + PAGE_SIZE_4K - 1) / PAGE_SIZE_4K
```

for i in range(pages_to_free):

```return
                let addr = new_break + i * PAGE_SIZE_4K
                let phys = self.get_physical_address(proc.page_table, addr)
                if phys != 0:
                    self.unmap_page(proc.page_table, addr)
                    self.free_page(phys)

        proc.heap_end = new_break
        return new_break

    private def invalidate_tlb(self, virtual: u64) -> void:
        asm("invlpg [%0]" : : "r"(virtual))

    private def detect_memory(self) -> u64:
        let total = 0
        let entries = self.read_e820_map()
```

for i in range(entries):

```return
            if entries[i].type == 1:  # Usable RAM
                total = total + entries[i].length
        return total

    private def init_memory_zones(self) -> void:
        # Initialize per-zone page lists
```

## Part 6: File System

```lowlpython
# File: posix/filesystem.lp

class FileSystem:

    # File types
    const S_IFREG: u32 = 0x8000   # Regular file
    const S_IFDIR: u32 = 0x4000   # Directory
    const S_IFCHR: u32 = 0x2000   # Character device
    const S_IFBLK: u32 = 0x6000   # Block device
    const S_IFIFO: u32 = 0x1000   # FIFO
    const S_IFLNK: u32 = 0xA000   # Symbolic link
    const S_IFSOCK: u32 = 0xC000  # Socket

    # File modes
    const S_IRUSR: u32 = 0x0100
    const S_IWUSR: u32 = 0x0080
    const S_IXUSR: u32 = 0x0040
    const S_IRGRP: u32 = 0x0020
    const S_IWGRP: u32 = 0x0010
    const S_IXGRP: u32 = 0x0008
    const S_IROTH: u32 = 0x0004
    const S_IWOTH: u32 = 0x0002
    const S_IXOTH: u32 = 0x0001

    class Inode:
        ino: u32 = 0
        mode: u32 = 0
        nlink: u32 = 0
        uid: u32 = 0
        gid: u32 = 0
        size: u64 = 0
        atime: u64 = 0
        mtime: u64 = 0
        ctime: u64 = 0
        blocks: u32 = 0
        block_pointers: u64[15] = [0; 15]  # Direct + indirect

        def read_block(self, block_num: u32) -> ptr[u8]:
            if block_num < 12:
                let phys = self.block_pointers[block_num]
                return phys.as_ptr()
            # Handle indirect blocks
            return null

        def write_block(self, block_num: u32, data: ptr[u8], size: u32) -> void:
            if block_num < 12:
                if self.block_pointers[block_num] == 0:
                    self.block_pointers[block_num] = memory_manager.allocate_page()
                let dest = self.block_pointers[block_num].as_ptr()
                copy_memory(dest, data, size)

    class DirectoryEntry:
        ino: u32 = 0
        name: u8[256] = [0; 256]
        next: ptr[DirectoryEntry] = null

    class FileDescriptor:
        fd: i32 = -1
        inode: ptr[Inode] = null
        offset: u64 = 0
        flags: u32 = 0
        refcount: u32 = 0

    class Superblock:
        magic: u32 = 0x4E4653  # "NFS"
        blocks: u32 = 0
        inodes: u32 = 0
        free_blocks: u32 = 0
        free_inodes: u32 = 0
        block_size: u32 = 4096

    private superblock: Superblock = Superblock()
    private inode_table: ptr[Inode] = null
    private root_inode: ptr[Inode] = null

    def init(self) -> void:
        # Initialize superblock
        self.superblock.magic = 0x4E4653
        self.superblock.block_size = 4096

        # Allocate inode table (max 1024 inodes)
        self.inode_table = physical_alloc(1024 * sizeof(Inode), 4096)

        # Create root inode (ino = 1)
        self.root_inode = self.alloc_inode()
        self.root_inode.mode = S_IFDIR | 0o755
        self.root_inode.nlink = 2  # . and ..

        # Create root directory entries
        self.add_directory_entry(self.root_inode, ".", self.root_inode.ino)
        self.add_directory_entry(self.root_inode, "..", self.root_inode.ino)

    def mount_root(self) -> Result[void]:
        let boot_device = BlockDevice.get_root()
        if boot_device == null:
            return os_error(OS_ERROR_NODEV, "No boot device")

        # Read superblock
        boot_device.read(0, 1, self.superblock.as_ptr())
        if self.superblock.magic != 0x4E4653:
            # Format device if not formatted
            self.format_device(boot_device)

        # Load inode table
        boot_device.read(1, 1024, self.inode_table.as_ptr())

        return os_success()

    def open(self, path: ptr[u8], flags: u32, mode: u32) -> i32:
        let proc = scheduler.get_current()

        # Parse path and traverse to inode
        let inode = self.path_to_inode(path, proc.cwd)
        if inode == null:
            if flags & 0x40:  # O_CREAT
                inode = self.create_file(path, mode)
                if inode == null:
                    return -1
            else:
                return -1

        # Allocate file descriptor
        let fd = self.allocate_fd(proc)
        if fd < 0:
            return -1

        let file_desc = FileDescriptor()
        file_desc.fd = fd
        file_desc.inode = inode
        file_desc.offset = 0
        file_desc.flags = flags
        file_desc.refcount = 1

        proc.fd_table[fd] = file_desc.as_ptr()

        return fd

    def read(self, fd: i32, buffer: ptr[u8], count: u32) -> i32:
        let proc = scheduler.get_current()
```

if fd < 0 or fd >= 256:

```lowlpython
            return -1

        let file_desc = proc.fd_table[fd]
        if file_desc == null:
            return -1
        let inode = file_desc.inode
        let offset = file_desc.offset
        let remaining = count
        let pos = 0

        # Read block by block
        while remaining > 0:
            let block_num = (offset / 4096).as_u32()
            let block_offset = (offset % 4096).as_u32()
            let bytes_in_block = min(remaining, 4096 - block_offset)

            let block = inode.read_block(block_num)
            if block == null:
```

```break
            copy_memory(buffer + pos, block + block_offset, bytes_in_block)

            remaining = remaining - bytes_in_block
            pos = pos + bytes_in_block
            offset = offset + bytes_in_block

        file_desc.offset = offset
        inode.atime = rdtsc()

        return pos

    def write(self, fd: i32, buffer: ptr[u8], count: u32) -> i32:
        let proc = scheduler.get_current()
```

if fd < 0 or fd >= 256:

```break
            return -1

        let file_desc = proc.fd_table[fd]
        if file_desc == null:
            return -1

        let inode = file_desc.inode
        let offset = file_desc.offset
        let remaining = count
        let pos = 0

        # Write block by block
        while remaining > 0:
            let block_num = (offset / 4096).as_u32()
            let block_offset = (offset % 4096).as_u32()
            let bytes_in_block = min(remaining, 4096 - block_offset)

            inode.write_block(block_num, buffer + pos, bytes_in_block)
```

if offset + bytes_in_block > inode.size:

```break
                inode.size = offset + bytes_in_block

            remaining = remaining - bytes_in_block
            pos = pos + bytes_in_block
            offset = offset + bytes_in_block

        file_desc.offset = offset
        inode.mtime = rdtsc()
        inode.ctime = rdtsc()

        return pos
    def close(self, fd: i32) -> i32:
        let proc = scheduler.get_current()
```

if fd < 0 or fd >= 256:

```break
            return -1

        let file_desc = proc.fd_table[fd]
        if file_desc == null:
            return -1

        file_desc.refcount = file_desc.refcount - 1
        if file_desc.refcount == 0:
            physical_free(file_desc)
        proc.fd_table[fd] = null

        return 0

    def lseek(self, fd: i32, offset: i64, whence: u32) -> i64:
        let proc = scheduler.get_current()
```

if fd < 0 or fd >= 256:

```break
            return -1

        let file_desc = proc.fd_table[fd]
        if file_desc == null:
            return -1

        let inode = file_desc.inode
        let size = inode.size
```

let new_offset = match whence:

```break
            case 0:  # SEEK_SET
```

```offset
            case 1:  # SEEK_CUR
                file_desc.offset + offset
            case 2:  # SEEK_END
                size + offset
            case _:
                return -1
```

if new_offset < 0 or new_offset > size:

```offset
            return -1

        file_desc.offset = new_offset
        return new_offset

    def stat(self, path: ptr[u8], stat_buf: ptr[Stat]) -> i32:
        let proc = scheduler.get_current()
        let inode = self.path_to_inode(path, proc.cwd)

        if inode == null:
            return -1

        stat_buf.st_ino = inode.ino
        stat_buf.st_mode = inode.mode
        stat_buf.st_nlink = inode.nlink
        stat_buf.st_uid = inode.uid
        stat_buf.st_gid = inode.gid
        stat_buf.st_size = inode.size
        stat_buf.st_atime = inode.atime
        stat_buf.st_mtime = inode.mtime
        stat_buf.st_ctime = inode.ctime
        stat_buf.st_blocks = inode.blocks

        return 0

    def mkdir(self, path: ptr[u8], mode: u32) -> i32:
        let proc = scheduler.get_current()
        let parent_path = self.get_parent_path(path)
        let name = self.get_basename(path)

        let parent_inode = self.path_to_inode(parent_path, proc.cwd)
        if parent_inode == null or (parent_inode.mode & S_IFDIR) == 0:
            return -1

        let new_inode = self.alloc_inode()
        new_inode.mode = S_IFDIR | (mode & 0xFFF)
        new_inode.nlink = 2

        self.add_directory_entry(parent_inode, name, new_inode.ino)
        self.add_directory_entry(new_inode, ".", new_inode.ino)
        self.add_directory_entry(new_inode, "..", parent_inode.ino)

        return 0

    def rmdir(self, path: ptr[u8]) -> i32:
        let proc = scheduler.get_current()
        let parent_path = self.get_parent_path(path)
        let name = self.get_basename(path)

        let parent_inode = self.path_to_inode(parent_path, proc.cwd)
        if parent_inode == null:
            return -1

        let target_inode = self.path_to_inode(path, proc.cwd)
        if target_inode == null or (target_inode.mode & S_IFDIR) == 0:
            return -1
```

if not self.is_directory_empty(target_inode):

```offset
            return -1

        self.remove_directory_entry(parent_inode, name)
        self.free_inode(target_inode)

        return 0

    def unlink(self, path: ptr[u8]) -> i32:
        let proc = scheduler.get_current()
        let parent_path = self.get_parent_path(path)
        let name = self.get_basename(path)

        let parent_inode = self.path_to_inode(parent_path, proc.cwd)
        if parent_inode == null:
            return -1

        let target_inode = self.path_to_inode(path, proc.cwd)
        if target_inode == null:
            return -1

        target_inode.nlink = target_inode.nlink - 1
        if target_inode.nlink == 0:
            self.free_inode(target_inode)
        self.remove_directory_entry(parent_inode, name)

        return 0

    private def path_to_inode(self, path: ptr[u8], cwd: ptr[DirectoryEntry]) -> ptr[Inode]:
        let current = if path[0] == '/' then self.root_inode else cwd

        let token = self.strtok(path, "/")
        while token != null:
            let entry = self.find_directory_entry(current, token)
            if entry == null:
                return null
            current = self.inode_table[entry.ino]
            token = self.strtok(null, "/")

        return current

    private def find_directory_entry(self, dir_inode: ptr[Inode], name: ptr[u8]) -> ptr[DirectoryEntry]:
        if (dir_inode.mode & S_IFDIR) == 0:
            return null

        let block = dir_inode.read_block(0)
        if block == null:
            return null

        let entry = block.as_ptr[DirectoryEntry]()
        while entry != null:
            if self.strcmp(entry.name, name) == 0:
                return entry
            entry = entry.next
        return null
    private def add_directory_entry(self, dir_inode: ptr[Inode], name: ptr[u8], ino: u32) -> void:
        let new_entry = physical_alloc(sizeof(DirectoryEntry), 8)
        new_entry.ino = ino
        self.strcpy(new_entry.name, name)
        new_entry.next = null

        let first_block = dir_inode.read_block(0)
        if first_block == null:
            let new_block = memory_manager.allocate_page()
            dir_inode.write_block(0, new_block.as_ptr(), 4096)
            let block = new_block.as_ptr[DirectoryEntry]()
            block = new_entry
        else:
            let last = first_block.as_ptr[DirectoryEntry]()
            while last.next != null:
                last = last.next
            last.next = new_entry

    private def remove_directory_entry(self, dir_inode: ptr[Inode], name: ptr[u8]) -> void:
        let block = dir_inode.read_block(0)
        if block == null:
```

```return
        let prev = null
        let curr = block.as_ptr[DirectoryEntry]()
        while curr != null:
            if self.strcmp(curr.name, name) == 0:
                if prev == null:
                    block.as_ptr[DirectoryEntry]() = curr.next
                else:
                    prev.next = curr.next
                physical_free(curr)
```

```return
            prev = curr
            curr = curr.next

    private def alloc_inode(self) -> ptr[Inode]:
```

for i in range(1024):

```return
            if self.inode_table[i].mode == 0:
                self.inode_table[i].ino = i + 1
                self.superblock.free_inodes = self.superblock.free_inodes - 1
                return (self.inode_table + i).as_ptr()
        return null

    private def free_inode(self, inode: ptr[Inode]) -> void:
        inode.mode = 0
        inode.ino = 0
        self.superblock.free_inodes = self.superblock.free_inodes + 1
```

## Part 7: System Calls

```lowlpython
# File: posix/syscall.lp

class SyscallHandler:

    # System call numbers
    const SYS_READ: u64 = 0
    const SYS_WRITE: u64 = 1
    const SYS_OPEN: u64 = 2
    const SYS_CLOSE: u64 = 3
    const SYS_STAT: u64 = 4
    const SYS_FSTAT: u64 = 5
    const SYS_LSEEK: u64 = 8
    const SYS_MMAP: u64 = 9
    const SYS_MUNMAP: u64 = 11
    const SYS_BRK: u64 = 12
    const SYS_GETPID: u64 = 20
    const SYS_GETPPID: u64 = 39
    const SYS_GETUID: u64 = 24
    const SYS_GETGID: u64 = 25
    const SYS_SETSID: u64 = 66
    const SYS_KILL: u64 = 62
    const SYS_EXIT: u64 = 60
    const SYS_FORK: u64 = 57
    const SYS_EXECVE: u64 = 59
    const SYS_WAIT4: u64 = 61
    const SYS_NICE: u64 = 34
    const SYS_SCHED_YIELD: u64 = 24
    const SYS_SCHED_GET_PRIORITY: u64 = 140
    const SYS_SCHED_SET_PRIORITY: u64 = 141
    const SYS_SIGACTION: u64 = 13
    const SYS_SIGPROCMASK: u64 = 14
    const SYS_SIGRETURN: u64 = 15
    const SYS_IOCTL: u64 = 16
    const SYS_MKDIR: u64 = 83
    const SYS_RMDIR: u64 = 84
    const SYS_UNLINK: u64 = 87
    const SYS_CHDIR: u64 = 80
    const SYS_GETCWD: u64 = 79
    const SYS_DUP: u64 = 32
    const SYS_DUP2: u64 = 33
    const SYS_PIPE: u64 = 22
    const SYS_SOCKET: u64 = 41
    private syscall_table: ptr[Function][512] = null

    def init(self) -> void:
        # Initialize syscall table
```

for i in range(512):

```lowlpython
            self.syscall_table[i] = self.syscall_unknown

        # Register syscalls
        self.syscall_table[SYS_READ] = self.sys_read
        self.syscall_table[SYS_WRITE] = self.sys_write
        self.syscall_table[SYS_OPEN] = self.sys_open
        self.syscall_table[SYS_CLOSE] = self.sys_close
        self.syscall_table[SYS_STAT] = self.sys_stat
        self.syscall_table[SYS_LSEEK] = self.sys_lseek
        self.syscall_table[SYS_MMAP] = self.sys_mmap
        self.syscall_table[SYS_MUNMAP] = self.sys_munmap
        self.syscall_table[SYS_BRK] = self.sys_brk
        self.syscall_table[SYS_GETPID] = self.sys_getpid
        self.syscall_table[SYS_GETPPID] = self.sys_getppid
        self.syscall_table[SYS_GETUID] = self.sys_getuid
        self.syscall_table[SYS_GETGID] = self.sys_getgid
        self.syscall_table[SYS_KILL] = self.sys_kill
        self.syscall_table[SYS_EXIT] = self.sys_exit
        self.syscall_table[SYS_FORK] = self.sys_fork
        self.syscall_table[SYS_EXECVE] = self.sys_execve
        self.syscall_table[SYS_WAIT4] = self.sys_wait4
        self.syscall_table[SYS_NICE] = self.sys_nice
        self.syscall_table[SYS_SCHED_YIELD] = self.sys_sched_yield
        self.syscall_table[SYS_SIGACTION] = self.sys_sigaction
        self.syscall_table[SYS_SIGPROCMASK] = self.sys_sigprocmask
        self.syscall_table[SYS_IOCTL] = self.sys_ioctl
        self.syscall_table[SYS_MKDIR] = self.sys_mkdir
        self.syscall_table[SYS_RMDIR] = self.sys_rmdir
        self.syscall_table[SYS_UNLINK] = self.sys_unlink
        self.syscall_table[SYS_CHDIR] = self.sys_chdir
        self.syscall_table[SYS_GETCWD] = self.sys_getcwd
        self.syscall_table[SYS_DUP] = self.sys_dup
        self.syscall_table[SYS_DUP2] = self.sys_dup2
        self.syscall_table[SYS_PIPE] = self.sys_pipe
        self.syscall_table[SYS_SOCKET] = self.sys_socket

        # Set up syscall interrupt (int 0x80) or syscall instruction
        self.setup_syscall_gate()

    @interrupt_handler(vector=0x80)
    def syscall_dispatcher(self, frame: InterruptFrame) -> void:
        let sysno = frame.rax
        let arg1 = frame.rdi
        let arg2 = frame.rsi
        let arg3 = frame.rdx
        let arg4 = frame.r10
        let arg5 = frame.r8
        let arg6 = frame.r9

        if sysno < 512:
            let handler = self.syscall_table[sysno]
            if handler != null:
                let result = handler(arg1, arg2, arg3, arg4, arg5, arg6)
                frame.rax = result
```

```return
        frame.rax = -1
    def setup_syscall_gate(self) -> void:
        # Set up MSR for syscall instruction (AMD64)
        let star = (0x001B000000000000) | (0x0008000000000000)
        write_msr(0xC0000081, star)  # STAR
        write_msr(0xC0000080, 0x00000001)  # EFER.SCE
        write_msr(0xC0000082, self.syscall_dispatcher.as_u64())  # LSTAR

    # System call implementations
    def sys_read(self, fd: u64, buf: u64, count: u64) -> i64:
        let proc = scheduler.get_current()
        if fd >= 256:
            return -1

        if fd == 0:  # stdin
            return keyboard_read(buf.as_ptr(), count).as_i64()

        return filesystem.read(fd.as_i32(), buf.as_ptr(), count.as_u32()).as_i64()

    def sys_write(self, fd: u64, buf: u64, count: u64) -> i64:
        let proc = scheduler.get_current()
        if fd >= 256:
            return -1

        if fd == 1 or fd == 2:  # stdout/stderr
```

for i in range(count):

```return
                let ch = (buf + i).as_ptr[u8]().value
                putchar(ch)
            return count.as_i64()

        return filesystem.write(fd.as_i32(), buf.as_ptr(), count.as_u32()).as_i64()
    def sys_open(self, pathname: u64, flags: u64, mode: u64) -> i64:
        return filesystem.open(pathname.as_ptr(), flags.as_u32(), mode.as_u32()).as_i64()

    def sys_close(self, fd: u64) -> i64:
        return filesystem.close(fd.as_i32()).as_i64()

    def sys_stat(self, pathname: u64, statbuf: u64) -> i64:
        return filesystem.stat(pathname.as_ptr(), statbuf.as_ptr[Stat]()).as_i64()

    def sys_lseek(self, fd: u64, offset: u64, whence: u64) -> i64:
        return filesystem.lseek(fd.as_i32(), offset.as_i64(), whence.as_u32())

    def sys_mmap(self, addr: u64, length: u64, prot: u64, flags: u64, fd: u64, offset: u64) -> u64:
        return memory_manager.mmap(addr.as_ptr(), length, prot.as_u32(),
                                   flags.as_u32(), fd.as_i32(), offset).as_u64()

    def sys_munmap(self, addr: u64, length: u64) -> i64:
        return memory_manager.munmap(addr.as_ptr(), length).as_i64()

    def sys_brk(self, addr: u64) -> u64:
        return memory_manager.brk(addr)

    def sys_getpid(self) -> i64:
        return process_manager.getpid().as_i64()

    def sys_getppid(self) -> i64:
        return process_manager.getppid().as_i64()

    def sys_getuid(self) -> i64:
        return process_manager.getuid().as_i64()
    def sys_getgid(self) -> i64:
        return process_manager.getgid().as_i64()

    def sys_kill(self, pid: u64, sig: u64) -> i64:
        return process_manager.kill(pid.as_i32(), sig.as_u8()).as_i64()

    def sys_exit(self, status: u64) -> void:
        process_manager.exit(status.as_i32())
        while true: halt()  # Never reached

    def sys_fork(self) -> i64:
        return process_manager.fork().as_i64()

    def sys_execve(self, path: u64, argv: u64, envp: u64) -> i64:
        return process_manager.execve(path.as_ptr(), argv.as_ptr[ptr[u8]](),
                                      envp.as_ptr[ptr[u8]]()).as_i64()

    def sys_wait4(self, pid: u64, status: u64, options: u64, rusage: u64) -> i64:
        return process_manager.waitpid(pid.as_i32(), status.as_ptr[i32](),
                                       options.as_u32()).as_i64()

    def sys_nice(self, inc: u64) -> i64:
        return process_manager.nice(inc.as_i32()).as_i64()

    def sys_sched_yield(self) -> i64:
        process_manager.sched_yield()
        return 0

    def sys_sigaction(self, signum: u64, act: u64, oldact: u64) -> i64:
        let proc = scheduler.get_current()
```

if signum < 1 or signum > 64:

```return
            return -1

        if act != 0:
            let handler = act.as_ptr[SignalAction]()
            proc.signal_handlers[signum - 1] = handler.sa_handler
            proc.sigmask = handler.sa_mask

        if oldact != 0:
            let old_handler = proc.signal_handlers[signum - 1]
            oldact.as_ptr[SignalAction]().sa_handler = old_handler

        return 0

    def sys_sigprocmask(self, how: u64, set: u64, oldset: u64) -> i64:
        let proc = scheduler.get_current()

        if oldset != 0:
            oldset.as_ptr[u64]().value = proc.sigmask

        if set != 0:
            match how:
                case 0:  # SIG_BLOCK
                    proc.sigmask = proc.sigmask | set.as_ptr[u64]().value
                case 1:  # SIG_UNBLOCK
                    proc.sigmask = proc.sigmask & ~set.as_ptr[u64]().value
                case 2:  # SIG_SETMASK
                    proc.sigmask = set.as_ptr[u64]().value
                case _:
                    return -1
        return 0

    def sys_ioctl(self, fd: u64, request: u64, argp: u64) -> i64:
        # Device-specific ioctl
        let proc = scheduler.get_current()
        let file = proc.fd_table[fd]
        if file == null:
            return -1

        if file.inode.mode & S_IFCHR:
            # Character device ioctl
            return self.device_ioctl(file.inode.ino, request, argp)
        return -1

    def sys_mkdir(self, pathname: u64, mode: u64) -> i64:
        return filesystem.mkdir(pathname.as_ptr(), mode.as_u32()).as_i64()

    def sys_rmdir(self, pathname: u64) -> i64:
        return filesystem.rmdir(pathname.as_ptr()).as_i64()

    def sys_unlink(self, pathname: u64) -> i64:
        return filesystem.unlink(pathname.as_ptr()).as_i64()

    def sys_chdir(self, pathname: u64) -> i64:
        let proc = scheduler.get_current()
        let new_dir = filesystem.path_to_inode(pathname.as_ptr(), proc.cwd)
        if new_dir == null or (new_dir.mode & S_IFDIR) == 0:
            return -1
        proc.cwd = new_dir
        return 0
    def sys_getcwd(self, buf: u64, size: u64) -> u64:
        let proc = scheduler.get_current()
        let path = filesystem.inode_to_path(proc.cwd)
        let len = strlen(path)
        if len >= size:
            return 0
        strcpy(buf.as_ptr(), path)
        return buf

    def sys_dup(self, oldfd: u64) -> i64:
        let proc = scheduler.get_current()
        if oldfd >= 256:
            return -1

        if proc.fd_table[oldfd] == null:
            return -1

        let newfd = self.allocate_fd(proc)
        if newfd < 0:
            return -1

        proc.fd_table[newfd] = proc.fd_table[oldfd]
        proc.fd_table[newfd].refcount = proc.fd_table[newfd].refcount + 1
        return newfd

    def sys_dup2(self, oldfd: u64, newfd: u64) -> i64:
        let proc = scheduler.get_current()
```

if oldfd >= 256 or newfd >= 256:

```return
            return -1

        if proc.fd_table[oldfd] == null:
            return -1

        if proc.fd_table[newfd] != null:
            self.sys_close(newfd)

        proc.fd_table[newfd] = proc.fd_table[oldfd]
        proc.fd_table[newfd].refcount = proc.fd_table[newfd].refcount + 1
        return newfd.as_i64()

    def sys_pipe(self, pipefd: u64) -> i64:
        let proc = scheduler.get_current()
        let pipe = Pipe.new()

        let read_fd = self.allocate_fd(proc)
        let write_fd = self.allocate_fd(proc)
```

if read_fd < 0 or write_fd < 0:

```return
            return -1

        proc.fd_table[read_fd] = pipe.read_end
        proc.fd_table[write_fd] = pipe.write_end

        pipefd.as_ptr[i32]()[0] = read_fd
        pipefd.as_ptr[i32]()[1] = write_fd

        return 0

    def sys_socket(self, domain: u64, type: u64, protocol: u64) -> i64:
        let proc = scheduler.get_current()
        let socket = Socket.new(domain.as_i32(), type.as_i32(), protocol.as_i32())
        let fd = self.allocate_fd(proc)
        if fd < 0:
            return -1

        proc.fd_table[fd] = socket
        return fd

    def syscall_unknown(self) -> i64:
        return -1

    private def device_ioctl(self, dev_num: u32, request: u64, argp: u64) -> i64:
        # Route to appropriate device driver
        if dev_num >= 0x1000:  # TTY device
            return tty_ioctl(dev_num, request, argp)
        return -1

    private def allocate_fd(self, proc: ptr[ProcessControlBlock]) -> i32:
```

for i in range(256):

```return
            if proc.fd_table[i] == null:
                return i
        return -1
```

## Part 8: Context Switching

```lowlpython
# File: posix/context.lp

class ContextSwitch:

    @naked
    def switch_to(self, old_stack: ptr, new_stack: ptr) -> void:
        asm("""
            # Save current context
            push rax
            push rbx
            push rcx
            push rdx
            push rsi
            push rdi
            push rbp
            push r8
            push r9
            push r10
            push r11
            push r12
            push r13
            push r14
            push r15

            # Save stack pointer
            mov [rdi], rsp

            # Switch to new stack
            mov rsp, [rsi]

            # Restore new context
            pop r15
            pop r14
            pop r13
            pop r12
            pop r11
            pop r10
            pop r9
            pop r8
            pop rbp
            pop rdi
            pop rsi
            pop rdx
            pop rcx
            pop rbx
            pop rax
```

```ret
        """)

    @naked
    def switch_to_user(self, user_rip: u64, user_rsp: u64, user_cr3: u64) -> void:
        asm("""
            # Switch to user page table
            mov cr3, rdx

            # Set up user stack
            mov rsp, rsi

            # Push return address and segment registers for iretq
            push 0x23          # User data segment
            push rsi           # User stack
            pushfq             # RFLAGS
            push 0x2B          # User code segment
            push rdi           # User RIP

            # Return to user mode
```

```iretq
        """)

    def save_fpu_state(self, buffer: ptr[u8]) -> void:
        asm("fxsave [%0]" : : "r"(buffer))

    def restore_fpu_state(self, buffer: ptr[u8]) -> void:
        asm("fxrstor [%0]" : : "r"(buffer))
```

## Part 9: Signal Handling

```lowlpython
# File: posix/signal.lp

class SignalHandler:

    # Standard signals
    const SIGHUP: u8 = 1
    const SIGINT: u8 = 2
    const SIGQUIT: u8 = 3
    const SIGILL: u8 = 4
    const SIGTRAP: u8 = 5
    const SIGABRT: u8 = 6
    const SIGBUS: u8 = 7
    const SIGFPE: u8 = 8
    const SIGKILL: u8 = 9
    const SIGUSR1: u8 = 10
    const SIGSEGV: u8 = 11
    const SIGUSR2: u8 = 12
    const SIGPIPE: u8 = 13
    const SIGALRM: u8 = 14
    const SIGTERM: u8 = 15
    const SIGCHLD: u8 = 17
    const SIGCONT: u8 = 18
    const SIGSTOP: u8 = 19
    const SIGTSTP: u8 = 20

    # Signal action flags
    const SA_NOCLDSTOP: u32 = 0x00000001
    const SA_NOCLDWAIT: u32 = 0x00000002
    const SA_SIGINFO: u32 = 0x00000004
    const SA_ONSTACK: u32 = 0x00000008
    const SA_RESTART: u32 = 0x00000010

    class SignalAction:
        sa_handler: ptr[Function] = null
        sa_mask: u64 = 0
        sa_flags: u32 = 0
        sa_restorer: ptr[Function] = null

    class SignalFrame:
        rip: u64 = 0
        rsp: u64 = 0
        rbp: u64 = 0
        rflags: u64 = 0
        signo: i32 = 0
        siginfo: ptr[SignalInfo] = null
        ucontext: ptr[UserContext] = null

    def deliver_signal(self, proc: ptr[ProcessControlBlock], sig: u8) -> void:
        if sig == SIGKILL:
            process_manager.exit(128 + sig)
```

```return
        if proc.signal_handlers[sig - 1] == SIG_IGN:
```

```return
        if proc.signal_handlers[sig - 1] == SIG_DFL:
            self.default_action(proc, sig)
```

```return
        # User-defined handler
        self.setup_signal_frame(proc, sig)
        proc.pending_signals = proc.pending_signals & ~(1 << (sig - 1))

    def setup_signal_frame(self, proc: ptr[ProcessControlBlock], sig: u8) -> void:
        let user_rsp = proc.registers.rsp
        let frame_addr = user_rsp - sizeof(SignalFrame)

        let frame = frame_addr.as_ptr[SignalFrame]()
        frame.rip = proc.registers.rip
        frame.rsp = proc.registers.rsp
        frame.rbp = proc.registers.rbp
        frame.rflags = proc.registers.rflags
        frame.signo = sig

        proc.registers.rip = proc.signal_handlers[sig - 1].as_u64()
        proc.registers.rsp = frame_addr
        proc.registers.rdi = sig.as_u64()

    def sigreturn(self) -> void:
        let proc = scheduler.get_current()
        let frame = proc.registers.rsp.as_ptr[SignalFrame]()

        proc.registers.rip = frame.rip
        proc.registers.rsp = frame.rsp
        proc.registers.rbp = frame.rbp
        proc.registers.rflags = frame.rflags

    def default_action(self, proc: ptr[ProcessControlBlock], sig: u8) -> void:
        match sig:
```

case SIGINT | SIGQUIT | SIGILL | SIGABRT | SIGFPE | SIGSEGV | SIGTERM:

```return
                process_manager.exit(128 + sig)
            case SIGCHLD:
                # Ignore
```

```pass
            case SIGSTOP | SIGTSTP:
                proc.state = ProcessState.STOPPED
            case SIGCONT:
                proc.state = ProcessState.READY
```

## Part 10: Complete Boot and Initialization

```lowlpython
# File: posix/boot.lp

target cpu: IntelCore i7-13700K
target motherboard: Generic x86_64

module POSIXBoot version(1,0,0) author(NEBULA_OS):

    exports: boot_main

    # Global kernel components
    published kernel: ptr[Kernel] = null
    published scheduler: ptr[Scheduler] = null
    published process_manager: ptr[ProcessManager] = null
    published memory_manager: ptr[MemoryManager] = null
    published filesystem: ptr[FileSystem] = null
    published syscall_handler: ptr[SyscallHandler] = null

    def boot_main() -> void:
        # Disable interrupts during bootstrap
        disable_interrupts()
        # Clear screen
        fb = Framebuffer()
        fb.init_from_multiboot()
        fb.clear(0x00000000)

        # Initialize memory manager
        memory_manager = MemoryManager()
        memory_manager.init()
        log("Memory Manager initialized")

        # Initialize scheduler
        scheduler = Scheduler()
        scheduler.init()
        log("Scheduler initialized")

        # Initialize process manager
        process_manager = ProcessManager()
        process_manager.init()
        log("Process Manager initialized")

        # Initialize file system
        filesystem = FileSystem()
        filesystem.init()
        log("File System initialized")

        # Initialize syscall handler
        syscall_handler = SyscallHandler()
        syscall_handler.init()
        log("Syscall Handler initialized")
        # Initialize kernel
        kernel = Kernel()
        kernel.init()
        log("Kernel initialized")

        # Initialize device drivers
        init_devices()
        log("Device drivers initialized")

        # Create init process
        let init_pid = process_manager.fork()
        if init_pid == 0:
            # Child process - load init binary
            process_manager.execve("/sbin/init".as_ptr(), null, null)
            while true: halt()

        # Enable interrupts
        enable_interrupts()

        # Start scheduler
        log("Entering main scheduler loop")
        while true:
            scheduler.schedule()
            halt()

    def log(message: ptr[u8]) -> void:
        fb = Framebuffer()
        fb.write_string(message)
        fb.write_char(10)

    def init_devices() -> void:
        # Initialize PCI bus
        pci = PCIController()
        pci.init()
        log("PCI Controller initialized")

        # Initialize UART for console
        uart = UART(0x3F8)
        uart.init(115200)
        log("UART initialized")

        # Initialize keyboard
        kbd = PS2Keyboard()
        kbd.init()
        log("Keyboard initialized")

        # Initialize timer
        timer = PITTimer()
        timer.init(1000)  # 1kHz
        log("Timer initialized")
```

## Part 11: Complete Example - Userspace Program

```lowlpython
# File: init_shell.lp - Simple shell program for POSIX system

def main() -> i32:
    while true:
        # Print prompt
        write(1, "$ ", 2)

        # Read command
        buffer: u8[256] = [0; 256]
        let len = read(0, buffer.as_ptr(), 255)
        if len <= 0:
```

```continue
        # Remove newline
        if buffer[len-1] == 10:
            buffer[len-1] = 0

        # Parse command
        let args = parse_args(buffer.as_ptr())

        # Handle built-in commands
        if strcmp(args[0], "exit") == 0:
```

```break
        elif strcmp(args[0], "cd") == 0:
            chdir(args[1] if args[1] != null else "/")
        elif strcmp(args[0], "ps") == 0:
            show_processes()
        else:
            # Fork and execute external command
            let pid = fork()
            if pid == 0:
                execvp(args[0], args)
                write(2, "Command not found\n", 18)
                exit(1)
            else:
                wait(pid)

    return 0

def show_processes() -> void:
    let proc_dir = opendir("/proc")
    let entry = readdir(proc_dir)
    write(1, "PID\tSTATE\tCOMMAND\n", 19)
    while entry != null:
        let pid_str = entry.d_name
        let stat_file = open("/proc/" + pid_str + "/stat", 0)
        if stat_file >= 0:
            let stat_buf: u8[256] = [0; 256]
            read(stat_file, stat_buf.as_ptr(), 255)
            close(stat_file)
            # Parse and display process info
            write(1, pid_str, strlen(pid_str))
            write(1, "\t", 1)
            # ... display state and command
        entry = readdir(proc_dir)
    closedir(proc_dir)

# Run the shell
let result = main()
exit(result)
```

## Part 12: Build Configuration

```lua
-- build_posix.lua
```

-- Build configuration for POSIX kernel

project "POSIXKernel"

```lua
    target_platform = "x86_64-nebula"
    target_cpu = "IntelCore i7-13700K"
    kernel_name = "posix_kernel"

    sources = {
        "posix/kernel.lp",
        "posix/process.lp",
        "posix/scheduler.lp",
        "posix/process_mgmt.lp",
        "posix/memory.lp",
        "posix/filesystem.lp",
        "posix/syscall.lp",
        "posix/context.lp",
        "posix/signal.lp",
        "posix/boot.lp"
    }

    drivers = {
        "drivers/pci.lp",
        "drivers/uart.lp",
        "drivers/ps2_keyboard.lp",
        "drivers/pit_timer.lp",
        "drivers/framebuffer.lp"
    }

    optimization = "O2"
    kernel_stack_size = 16384
    user_stack_size = 8192
    max_processes = 1024
    max_fds = 256
    tick_hz = 1000

    output_format = "elf"

-- Build command:
-- lowl compile --config build_posix.lua --output posix_kernel.elf
Summary: POSIX System Features
Component
Scheduler Multi-level priority queue
Process Management
Memory Management
```
