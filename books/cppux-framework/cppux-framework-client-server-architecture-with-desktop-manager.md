# CPPUX Framework - Client/Server Architecture with Desktop Manager

**By Anthony Matarazzo (c) 2026**

*35-page technical reference, translated to Markdown from `CPPUX Framework - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

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

- [Complete Separation of Client and Desktop Manager](#complete-separation-of-client-and-desktop-manager)
  - [Directory Structure](#directory-structure)
  - [1. include/cppux/common.h - Shared Types](#1-includecppuxcommonh---shared-types)
  - [2. include/cppux/client.h - Client-Side API (Single Header)](#2-includecppuxclienth---client-side-api-single-header)
  - [3. include/cppux/desktop.h - Desktop Manager API](#3-includecppuxdesktoph---desktop-manager-api)
  - [4. src/client/lib.cpp - Client Library Implementation](#4-srcclientlibcpp---client-library-implementation)
  - [5. examples/client/simple_app.cpp - Simple Client Example](#5-examplesclientsimpleappcpp---simple-client-example)
  - [6. examples/desktop/desktop_main.cpp - Desktop Manager Example](#6-examplesdesktopdesktopmaincpp---desktop-manager-example)
  - [7. CMakeLists.txt - Build Configuration](#7-cmakeliststxt---build-configuration)
- [Summary](#summary)
  - [Security Benefits:](#security-benefits)
- [3. Message Validation - Desktop validates all client messages](#3-message-validation---desktop-validates-all-client-messages)
  - [Client Development (Single Header):](#client-development-single-header)
  - [Desktop Manager:](#desktop-manager)

---

## Complete Separation of Client and Desktop Manager

### Directory Structure

```text
cppux/
```

```text
├── include/
│   ├── cppux/
│   │   ├── common.h          # Shared types and structures
│   │   ├── client.h          # Client-side API (single header)
│   │   └── desktop.h         # Desktop manager API
├── src/
│   ├── desktop/
│   │   ├── main.cpp          # Desktop manager entry
│   │   ├── x11_wm.cpp        # X11 window manager
│   │   ├── wayland_wm.cpp    # Wayland compositor
│   │   ├── renderer.cpp      # Hardware rendering
│   │   └── shm_server.cpp    # Shared memory server
│   └── client/
│       ├── lib.cpp           # Client library implementation
│       └── stub.cpp          # Client stub for linking
├── examples/
│   ├── desktop/
│   │   └── desktop_main.cpp  # Desktop manager example
│   └── client/
│       ├── simple_app.cpp    # Simple client app
│       └── complex_app.cpp   # Complex client app
└── CMakeLists.txt
```

### 1. include/cppux/common.h - Shared Types

```cpp
/**
 * @file common.h
 * @author Anthony Matarazzo
 * @copyright Copyright (c) 2026 Anthony Matarazzo
 * @license MIT License
 * @brief Shared types between desktop manager and clients
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace cppux {

//
```

```text
============================================================================// Version
```

```cpp
//
```

```text
============================================================================constexpr const char* CPPUX_VERSION = "4.0.0";
```

```cpp
//
```

```text
============================================================================// Shared Enumerations
```

```cpp
//
```

```text
============================================================================
```

```cpp
enum class unit : uint8_t { px, pt, em, pct, auto_ };
enum class display_type : uint8_t { none, block, inline, flex, grid };

enum class position_type : uint8_t { static_, relative, absolute, fixed };enum class text_align : uint8_t { left, center, right, justify };
enum class border_style : uint8_t { none, solid, dashed, dotted };
enum class edge : uint8_t { sharp, bevel, rounded, ridge };

enum class material_type : uint8_t { plastic, glass, metal, skin, wood, chrome, gold, silver };

enum class cursor_type : uint8_t { arrow, hand, text, move, wait, crosshair };
enum class key_modifier : uint8_t { none, shift, ctrl, alt, meta };

enum class event_type : uint16_t {
    // Mouse events

    click, double_click, mouse_down, mouse_up, mouse_move, mouse_enter, mouse_leave,
    // Keyboard events
    key_down, key_up, key_press,
    // Focus events
    focus, blur,
    // Window events
    resize, paint, close,
    // Touch events
    touch_start, touch_move, touch_end,
    // Scroll events
    scroll, wheel,
    // Client events
    client_connect, client_disconnect, client_message
};

enum class key : uint16_t {

    none = 0, backspace = 8, tab = 9, enter = 13, escape = 27, space = 32,    left = 37, up = 38, right = 39, down = 40, home = 36, end = 35,
    page_up = 33, page_down = 34, insert = 45, del = 46,
    a = 65, b = 66, c = 67, d = 68, e = 69, f = 70, g = 71, h = 72,
    i = 73, j = 74, k = 75, l = 76, m = 77, n = 78, o = 79, p = 80,

    q = 81, r = 82, s = 83, t = 84, u = 85, v = 86, w = 87, x = 88, y = 89, z = 90,
    num0 = 48, num1 = 49, num2 = 50, num3 = 51, num4 = 52,
    num5 = 53, num6 = 54, num7 = 55, num8 = 56, num9 = 57,
    f1 = 112, f2 = 113, f3 = 114, f4 = 115, f5 = 116, f6 = 117,
    f7 = 118, f8 = 119, f9 = 120, f10 = 121, f11 = 122, f12 = 123
};

enum class mouse_button : uint8_t { none = 0, left = 1, right = 2, middle = 3 };

//
```

```text
============================================================================// Shared Memory Protocol
```

```cpp
//
```

```text
============================================================================
```

```cpp
constexpr const char* SHM_PREFIX = "/cppux_";
constexpr size_t MAX_CLIENTS = 256;
constexpr size_t MAX_DOM_SIZE = 1024 * 1024;  // 1MB
constexpr size_t MAX_BUFFER_SIZE = 1920 * 1080 * 4;  // 4K framebuffer

enum class message_type : uint32_t {
    // Client -> Desktop
    register_client,
    unregister_client,
    dom_update,
    buffer_update,
    event_ack,

    // Desktop -> Client
    event_dispatch,
    ping,
    pong,
    sync_request,
    sync_response
};

struct message_header {
    message_type type;
    uint32_t client_id;
    uint32_t sequence;
    uint32_t payload_size;
    uint64_t timestamp;
};

struct client_info {
    uint32_t id;
    char name[64];
    int width;
    int height;
    int x;
    int y;
    bool focused;
    uint64_t last_active;
};

//
```

```text
============================================================================// Event Structure (POD for IPC)
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct serialized_event {
    event_type type;
    bool handled;
    bool bubbles;

    // Mouse data
    double mouse_x, mouse_y;
    double screen_x, screen_y;
    uint8_t button;
    int click_count;
    double wheel_delta;

    // Keyboard data
    uint16_t key_code;
    char character;
    bool shift, ctrl, alt, meta;
    bool caps_lock, num_lock;

    // Window data
    int width, height;
    uint32_t client_id;

    // Serialized touch points
    uint8_t touch_count;
    struct {
        int id;
        double x, y;
    } touches[10];
};

//
```

```text
============================================================================// Shared Memory Regions
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct shared_memory_region {
    int shmid;
    void* ptr;
    size_t size;
    char name[64];
    uint32_t client_id;
};

} // namespace cppux
```

### 2. include/cppux/client.h - Client-Side API (Single Header)

```cpp
/**
 * @file client.h
 * @author Anthony Matarazzo
 * @copyright Copyright (c) 2026 Anthony Matarazzo
 * @license MIT License
 * @brief Client-side API - Single header for application development
 *
 * Include this one header to build CPPUX client applications.
 * All desktop communication is handled transparently.
 */

#pragma once

#include "common.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <sstream>
#include <chrono>
namespace cppux {

//
```

```text
============================================================================// Forward Declarations
```

```cpp
//
```

```text
============================================================================
```

```cpp
class element;
class client_app;
class desktop_connection;

//
```

```text
============================================================================// Connection to Desktop Manager (Secure IPC)
```

```cpp
//
```

```text
============================================================================
```

```cpp
class desktop_connection {
private:
    int shm_fd_;
    void* shm_ptr_;
    size_t shm_size_;
    uint32_t client_id_;
    bool connected_;

    int msg_queue_id_;
    std::thread listener_thread_;
    std::atomic<bool> running_;

    void listener_loop();
    void handle_event(const serialized_event& evt);
    void send_message(const message_header& header, const void* payload);

public:
    desktop_connection();
    ~desktop_connection();

    bool connect(const std::string& app_name, int width, int height);
    void disconnect();
    bool is_connected() const { return connected_; }
    uint32_t client_id() const { return client_id_; }

    void send_dom(const std::string& dom_data);
    void send_buffer(const uint8_t* data, size_t size);
    void send_event_ack(uint32_t sequence);

    void register_event_handler(event_type type, std::function<void(const serialized_event&)> handler);
    static desktop_connection& instance();
};

//
```

```text
============================================================================// Value Class
```

```cpp
//
```

```text
============================================================================
```

```cpp
class value {
    double num_;
    unit unit_;
public:
    value() : num_(0), unit_(unit::px) {}
    value(double n, unit u = unit::px) : num_(n), unit_(u) {}
    explicit value(const std::string& str);

    double num() const { return num_; }
    unit units() const { return unit_; }
    double to_px(double parent = 0, double font = 16) const;
    std::string str() const;
};

inline value operator""_px(unsigned long long v) { return value((double)v, unit::px); }

inline value operator""_pt(unsigned long long v) { return value((double)v, unit::pt); }

inline value operator""_em(unsigned long long v) { return value((double)v, unit::em); }

inline value operator""_pct(unsigned long long v) { return value((double)v, unit::pct); }

//
```

```text
============================================================================// Color Class
```

```cpp
//
```

```text
============================================================================
```

```cpp
class color {
    double r_, g_, b_, a_;
public:
    color() : r_(0), g_(0), b_(0), a_(1) {}

    color(double r, double g, double b, double a = 1) : r_(r), g_(g), b_(b), a_(a) {}
    explicit color(const std::string& name);
    explicit color(uint32_t hex);

    static color rgb(double r, double g, double b, double a = 1) { return
color(r, g, b, a); }
    static color hsl(double h, double s, double l, double a = 1);
    static color hex(const std::string& hex);

    double r() const { return r_; }
    double g() const { return g_; }
    double b() const { return b_; }
    double a() const { return a_; }

    color lighter(double amount = 0.1) const;
    color darker(double amount = 0.1) const;
    color alpha(double a) const { return color(r_, g_, b_, a); }

    uint32_t rgb() const;
    std::string hex() const;

    // Named colors
    static color black() { return color(0,0,0); }
    static color white() { return color(1,1,1); }
    static color red() { return color(1,0,0); }
    static color green() { return color(0,1,0); }
    static color blue() { return color(0,0,1); }
    static color yellow() { return color(1,1,0); }
    static color cyan() { return color(0,1,1); }
    static color magenta() { return color(1,0,1); }
    static color gray() { return color(0.5,0.5,0.5); }
    static color transparent() { return color(0,0,0,0); }
};

//
```

```text
============================================================================// Style Properties
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct x { value v; x(value v_) : v(v_) {} };
struct y { value v; y(value v_) : v(v_) {} };
struct w { value v; w(value v_) : v(v_) {} };
struct h { value v; h(value v_) : v(v_) {} };
struct top { value v; top(value v_) : v(v_) {} };
struct left { value v; left(value v_) : v(v_) {} };
struct right { value v; right(value v_) : v(v_) {} };
struct bottom { value v; bottom(value v_) : v(v_) {} };
struct width { value v; width(value v_) : v(v_) {} };
struct height { value v; height(value v_) : v(v_) {} };
struct margin { value v; margin(value v_) : v(v_) {} };
struct padding { value v; padding(value v_) : v(v_) {} };
struct gap { value v; gap(value v_) : v(v_) {} };
struct bg { color c; bg(color c_) : c(c_) {} };
struct fg { color c; fg(color c_) : c(c_) {} };
struct border_color { color c; border_color(color c_) : c(c_) {} };

struct font_family { std::string v; font_family(const std::string& v_) : v(v_) {} };
struct font_size { value v; font_size(value v_) : v(v_) {} };
struct font_weight { int v; font_weight(int v_) : v(v_) {} };

struct text_align { text_align v; text_align(text_align v_) : v(v_) {} };struct line_height { value v; line_height(value v_) : v(v_) {} };

struct display { display_type v; display(display_type v_) : v(v_) {} };struct position { position_type v; position(position_type v_) : v(v_) {} };struct border { border_style v; border(border_style v_) : v(v_) {} };
struct radius { value v; radius(value v_) : v(v_) {} };
struct opacity { double v; opacity(double v_) : v(v_) {} };
struct z_index { int v; z_index(int v_) : v(v_) {} };
struct cursor { cursor_type v; cursor(cursor_type v_) : v(v_) {} };
struct id { std::string v; id(const std::string& v_) : v(v_) {} };
struct class_ { std::string v; class_(const std::string& v_) : v(v_) {} };

//
```

```text
============================================================================// Element Base Class (Client-Side)
```

```cpp
//
```

```text
============================================================================
```

```cpp
class element {
private:
    element* parent_ = nullptr;
    std::vector<std::unique_ptr<element>> children_;
    std::unordered_map<std::type_index, std::any> attrs_;
    std::string tag_;
    std::string text_;
    int cursor_ = 0;
    bool focus_ = false;
    bool disabled_ = false;

    std::vector<std::string> text_data_;

    void serialize_attributes(std::stringstream& ss) const;
    void serialize_children(std::stringstream& ss) const;

public:
    explicit element(const std::string& tag);
    virtual ~element() = default;

    // DOM manipulation
    element* parent() const { return parent_; }
    const auto& children() const { return children_; }

    element& add(std::unique_ptr<element> child);
    element& remove(element* child);
    void detach();
    void clear();

    // Fluent attribute API
    template<typename T>
    element& set(const T& attr) {
        attrs_[std::type_index(typeid(T))] = attr;
        return *this;
    }

    template<typename... Args>
    element& attr(Args&&... args) {
        (set(std::forward<Args>(args)), ...);
        return *this;
    }

    template<typename T>
    T* get() {
        auto it = attrs_.find(std::type_index(typeid(T)));
        if (it != attrs_.end()) return std::any_cast<T>(&it->second);
        return nullptr;
    }

    // Content
    element& text(const std::string& t);
    std::string text() const;

    element& operator<<(const std::string& str);
    element& operator<<(const char* str);

    // Text input methods
    void insert(const std::string& txt);
    void erase_left();
    void erase_right();
    void cursor_left();
    void cursor_right();
    void focus(bool f);
    bool focused() const { return focus_; }
    void disabled(bool d) { disabled_ = d; }
    bool disabled() const { return disabled_; }

    // Serialization for desktop communication
    std::string serialize() const;
    void deserialize(const std::string& data);

    const std::string& tag() const { return tag_; }
};
//
```

```text
============================================================================// Standard Elements
```

```cpp
//
```

```text
============================================================================
```

```cpp
class document : public element {
public:
    document() : element("doc") {
        set(display(display_type::block));
        set(width(100_pct));
        set(height(100_pct));
    }
};

class title : public element {
public:
    explicit title(const std::string& t = "") : element("title") {
        set(display(display_type::block));
        set(font_size(24_pt));
        set(font_weight(700));
        if (!t.empty()) text(t);
    }
};

class heading1 : public element {
public:
    explicit heading1(const std::string& t = "") : element("h1") {
        set(display(display_type::block));
        set(font_size(2_em));
        set(font_weight(700));
        if (!t.empty()) text(t);
    }
};

class heading2 : public element {
public:
    explicit heading2(const std::string& t = "") : element("h2") {
        set(display(display_type::block));
        set(font_size(1.5_em));
        set(font_weight(600));
        if (!t.empty()) text(t);
    }
};

class heading3 : public element {
public:
    explicit heading3(const std::string& t = "") : element("h3") {
        set(display(display_type::block));
        set(font_size(1.17_em));
        set(font_weight(600));
        if (!t.empty()) text(t);
    }
};

class paragraph : public element {
public:
    explicit paragraph(const std::string& t = "") : element("p") {
        set(display(display_type::block));
        set(line_height(1.5));
        if (!t.empty()) text(t);
    }
};

class div : public element {
public:
    div() : element("div") { set(display(display_type::block)); }
};

class span : public element {
public:
    span() : element("span") { set(display(display_type::inline)); }
};

class button : public element {
public:

    explicit button(const std::string& label = "") : element("button") {        set(display(display_type::inline_block));
        set(padding(8_px));
        set(bg(color::blue()));
        set(fg(color::white()));
        set(radius(4_px));
        set(cursor(cursor_type::hand));
        if (!label.empty()) text(label);
    }
};

class textbox : public element {
public:

    explicit textbox(const std::string& place = "") : element("textbox") {        set(display(display_type::inline_block));
        set(padding(8_px));
        set(border(border_style::solid));
        set(border_color(color::gray()));
        set(bg(color::white()));
        set(width(200_px));
        set(cursor(cursor_type::text));
        if (!place.empty()) set(placeholder(place));
    }

    std::string value() const { return text(); }
    void value(const std::string& v) { text(v); }

private:

    struct placeholder { std::string v; placeholder(const std::string& s) : v(s) {} };
};

//
```

```text
============================================================================// Client Application Base Class
```

```cpp
//
```

```text
============================================================================
```

```cpp
class client_app {
protected:
    std::unique_ptr<element> root_;
    std::string app_name_;
    int width_, height_;
    bool running_ = true;
    uint32_t client_id_;

    virtual void on_create() {}
    virtual void on_destroy() {}
    virtual void on_event(const serialized_event& e) {}
    virtual void on_render() {}

    void send_dom_update();

public:
    client_app(const std::string& name);
    virtual ~client_app();

    bool init(int width, int height);
    void run();
    void stop();
    void quit();

    element* root() { return root_.get(); }
    uint32_t client_id() const { return client_id_; }
    const std::string& name() const { return app_name_; }

    static client_app* instance();
};

//
```

```text
============================================================================
```

```cpp
// Global Creation Functions
//
```

```text
============================================================================
```

```cpp
template<typename T, typename... Args>
T& create(Args&&... args) {
    static std::vector<std::unique_ptr<element>> elements;
    auto el = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = el.get();
    elements.push_back(std::move(el));
    return *ptr;
}

} // namespace cppux
```

### 3. include/cppux/desktop.h - Desktop Manager API

```cpp
/**
 * @file desktop.h
 * @author Anthony Matarazzo
 * @copyright Copyright (c) 2026 Anthony Matarazzo
 * @license MIT License

 * @brief Desktop Manager API - X11/Wayland compositor and window management */

#pragma once

#include "common.h"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

namespace cppux {

//
```

```text
============================================================================// Forward Declarations
```

```cpp
//
```

```text
============================================================================
```

```cpp
class desktop_manager;
class render_backend;
class shared_memory_server;
class window_manager;
class compositor;
//
```

```text
============================================================================// Render Backend (Hardware Abstraction)
```

```cpp
//
```

```text
============================================================================
```

```cpp
class render_backend {
public:
    virtual ~render_backend() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void clear(const color& c) = 0;

    virtual void draw_rect(int x, int y, int w, int h, const color& c) = 0;    virtual void draw_text(const std::string& text, const std::string& font, double size,
                           double x, double y, const color& c) = 0;

    virtual void draw_image(const uint8_t* data, int w, int h, int x, int y) = 0;

    virtual void present() = 0;
    virtual void resize(int w, int h) = 0;

    static std::unique_ptr<render_backend> create();
};

//
```

```text
============================================================================// Shared Memory Server
```

```cpp
//
```

```text
============================================================================
```

```cpp
class shared_memory_server {
private:
    std::unordered_map<uint32_t, shared_memory_region> clients_;
    std::unordered_map<uint32_t, client_info> client_infos_;
    std::mutex mutex_;
    uint32_t next_id_ = 1;

public:
    shared_memory_server();
    ~shared_memory_server();

    uint32_t register_client(const std::string& name, int width, int
height);
    void unregister_client(uint32_t id);

    shared_memory_region* get_region(uint32_t id);
    client_info* get_client_info(uint32_t id);

    bool write_dom(uint32_t id, const std::string& dom);
    std::string read_dom(uint32_t id);

    bool write_buffer(uint32_t id, const uint8_t* data, size_t size);
    const uint8_t* read_buffer(uint32_t id);

    void broadcast_event(const serialized_event& evt, uint32_t exclude = 0);    void send_event(uint32_t id, const serialized_event& evt);

    std::vector<client_info> get_clients() const;
};

//
```

```text
============================================================================// X11 Window Manager
```

```cpp
//
```

```text
============================================================================
```

```cpp
#ifdef CPPUX_X11_WM

class x11_window_manager : public window_manager {
private:
    Display* display_;
    Window root_;
    std::unordered_map<Window, uint32_t> window_to_client_;
    std::unordered_map<uint32_t, Window> client_to_window_;

    Atom wm_protocols_;
    Atom wm_delete_;
    Atom net_active_window_;

    void setup_atoms();
    void manage_window(Window w, uint32_t client_id);
    void unmanage_window(Window w);

public:
    x11_window_manager();
    ~x11_window_manager();

    bool init() override;
    void run() override;
    void stop() override;

    void create_window(uint32_t client_id, int x, int y, int w, int h)
override;
    void destroy_window(uint32_t client_id) override;
    void resize_window(uint32_t client_id, int w, int h) override;
    void move_window(uint32_t client_id, int x, int y) override;
    void focus_window(uint32_t client_id) override;

    void set_title(uint32_t client_id, const std::string& title) override;};

#endif

//
```

```text
============================================================================// Wayland Compositor
```

```cpp
//
```

```text
============================================================================
```

```cpp
#ifdef CPPUX_WAYLAND_WM

class wayland_compositor : public window_manager {
private:
    struct wl_display* display_;
    struct wlr_backend* backend_;
    struct wlr_renderer* renderer_;
    struct wlr_compositor* compositor_;
    struct wlr_xdg_shell* xdg_shell_;

    std::unordered_map<uint32_t, struct wlr_xdg_surface*> surfaces_;
    std::unordered_map<struct wlr_xdg_surface*, uint32_t>
surface_to_client_;

    void setup_listeners();
    void handle_new_surface(struct wlr_xdg_surface* surface, uint32_t
client_id);

public:
    wayland_compositor();
    ~wayland_compositor();

    bool init() override;
    void run() override;
    void stop() override;

    void create_window(uint32_t client_id, int x, int y, int w, int h)
override;
    void destroy_window(uint32_t client_id) override;
    void resize_window(uint32_t client_id, int w, int h) override;
    void move_window(uint32_t client_id, int x, int y) override;
    void focus_window(uint32_t client_id) override;
};

#endif

//
```

```text
============================================================================// Desktop Manager (Main Orchestrator)
```

```cpp
//
```

```text
============================================================================
```

```cpp
class desktop_manager {
private:
    std::unique_ptr<render_backend> renderer_;
    std::unique_ptr<shared_memory_server> shm_server_;
    std::unique_ptr<window_manager> wm_;

    std::unique_ptr<element> desktop_root_;
    std::unique_ptr<element> panel_;
    std::unique_ptr<element> workspace_bar_;
    std::unique_ptr<element> clock_;
    std::unique_ptr<element> system_tray_;

    std::unordered_map<uint32_t, std::unique_ptr<element>> client_windows_;

    int current_workspace_ = 0;
    int workspace_count_ = 4;
    bool running_ = true;
    std::thread render_thread_;

    void setup_desktop_ui();
    void setup_panel();
    void setup_workspace_indicators();
    void setup_clock();
    void setup_system_tray();

    void layout_windows();
    void render_loop();
    void update_clock();

    void handle_client_connect(uint32_t id, const client_info& info);
    void handle_client_disconnect(uint32_t id);
    void handle_client_dom_update(uint32_t id, const std::string& dom);

public:
    desktop_manager();
    ~desktop_manager();

    bool init();
    void run();
    void stop();

    void switch_workspace(int workspace);
    int current_workspace() const { return current_workspace_; }

    void show_notification(const std::string& title, const std::string& message);
    void show_launcher();
    void show_app_menu();

    static desktop_manager& instance();
};

//
```

```text
============================================================================// Desktop Entry Point
```

```cpp
//
```

```text
============================================================================
```

```cpp
int desktop_main(int argc, char** argv);

} // namespace cppux
```

### 4. src/client/lib.cpp - Client Library Implementation

```cpp
/**
 * @file lib.cpp
 * @author Anthony Matarazzo
 * @brief Client library implementation
 */

#include "cppux/client.h"
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <chrono>

namespace cppux {

//
```

```text
============================================================================// Value Implementation
```

```cpp
//
```

```text
============================================================================
```

```cpp
value::value(const std::string& str) : num_(0), unit_(unit::px) {
    std::regex pattern(R"(^(\d+(?:\.\d+)?)\s*(px|pt|em|%|auto)?$)");
    std::smatch match;
    if (std::regex_match(str, match, pattern)) {
        num_ = std::stod(match[1].str());
        std::string u = match[2].str();
        if (u == "pt") unit_ = unit::pt;
        else if (u == "em") unit_ = unit::em;
        else if (u == "%") unit_ = unit::pct;
        else if (u == "auto") unit_ = unit::auto_;
        else unit_ = unit::px;
    }
}

double value::to_px(double parent, double font) const {
    switch (unit_) {
        case unit::px: return num_;
        case unit::pt: return num_ * 1.333;
        case unit::em: return num_ * font;
        case unit::pct: return num_ * parent / 100.0;
        default: return 0;
    }
}

std::string value::str() const {
    std::string u;
    switch (unit_) {
        case unit::px: u = "px"; break;
        case unit::pt: u = "pt"; break;
        case unit::em: u = "em"; break;
        case unit::pct: u = "%"; break;
        default: u = "";
    }
    return std::to_string(num_) + u;
}

//
```

```text
============================================================================// Color Implementation
```

```cpp
//
```

```text
============================================================================
```

```cpp
static const std::unordered_map<std::string, uint32_t> color_names = {
    {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000},
    {"green", 0x00FF00}, {"blue", 0x0000FF}, {"yellow", 0xFFFF00},
    {"cyan", 0x00FFFF}, {"magenta", 0xFF00FF}, {"gray", 0x808080},

    {"darkgray", 0xA9A9A9}, {"lightgray", 0xD3D3D3}, {"orange", 0xFFA500},    {"purple", 0x800080}, {"brown", 0xA52A2A}, {"pink", 0xFFC0CB},
    {"navy", 0x000080}, {"teal", 0x008080}, {"olive", 0x808000},
    {"maroon", 0x800000}, {"coral", 0xFF7F50}, {"crimson", 0xDC143C},
    {"gold", 0xFFD700}, {"silver", 0xC0C0C0}, {"violet", 0xEE82EE}
};

color::color(const std::string& name) : r_(0), g_(0), b_(0), a_(1) {
    std::string lower = name;

    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);    auto it = color_names.find(lower);
    if (it != color_names.end()) {
        uint32_t rgb = it->second;
        r_ = ((rgb >> 16) & 0xFF) / 255.0;
        g_ = ((rgb >> 8) & 0xFF) / 255.0;
        b_ = (rgb & 0xFF) / 255.0;
    }
}

color::color(uint32_t hex) : r_(((hex >> 16) & 0xFF) / 255.0),
                             g_(((hex >> 8) & 0xFF) / 255.0),
                             b_((hex & 0xFF) / 255.0),
                             a_(1) {}

color color::hsl(double h, double s, double l, double a) {
    h = std::fmod(h, 360.0);
    s = std::clamp(s, 0.0, 1.0);
    l = std::clamp(l, 0.0, 1.0);

    double c = (1 - std::abs(2 * l - 1)) * s;
    double hp = h / 60.0;
    double x = c * (1 - std::abs(std::fmod(hp, 2) - 1));

    double r1 = 0, g1 = 0, b1 = 0;
    if (hp < 1) { r1 = c; g1 = x; }
    else if (hp < 2) { r1 = x; g1 = c; }
    else if (hp < 3) { g1 = c; b1 = x; }
    else if (hp < 4) { g1 = x; b1 = c; }
    else if (hp < 5) { r1 = x; b1 = c; }
    else { r1 = c; b1 = x; }

    double m = l - c / 2;
    return color(r1 + m, g1 + m, b1 + m, a);
}

color color::lighter(double amount) const {
    return color(std::min(1.0, r_ + amount),
                 std::min(1.0, g_ + amount),
                 std::min(1.0, b_ + amount), a_);
}

color color::darker(double amount) const {
    return color(std::max(0.0, r_ - amount),
                 std::max(0.0, g_ - amount),
                 std::max(0.0, b_ - amount), a_);
}

uint32_t color::rgb() const {
    return (static_cast<uint32_t>(r_ * 255) << 16) |
           (static_cast<uint32_t>(g_ * 255) << 8) |
           (static_cast<uint32_t>(b_ * 255));
}

std::string color::hex() const {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X",
             static_cast<int>(r_ * 255),
             static_cast<int>(g_ * 255),
             static_cast<int>(b_ * 255));
    return buf;
}

//
```

```text
============================================================================// Desktop Connection Implementation
```

```cpp
//
```

```text
============================================================================
```

```cpp
static desktop_connection* g_connection = nullptr;

desktop_connection::desktop_connection()
    : shm_fd_(-1), shm_ptr_(nullptr), shm_size_(0),
      client_id_(0), connected_(false), running_(false) {}

desktop_connection::~desktop_connection() {
    disconnect();
}

bool desktop_connection::connect(const std::string& app_name, int width, int height) {
    if (connected_) return true;

    // Create unique shared memory name
    std::string shm_name = std::string(SHM_PREFIX) + app_name + "_" +
std::to_string(getpid());

    // Create shared memory
    shm_fd_ = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd_ == -1) return false;

    shm_size_ = MAX_BUFFER_SIZE;
    ftruncate(shm_fd_, shm_size_);

    shm_ptr_ = mmap(NULL, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);

    // Send registration message to desktop
    message_header header;
    header.type = message_type::register_client;
    header.client_id = 0;
    header.sequence = 0;
    header.payload_size = sizeof(client_info);

    header.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    client_info info;
    strncpy(info.name, app_name.c_str(), 63);
    info.width = width;
    info.height = height;
    info.x = 0;
    info.y = 0;
    info.focused = false;
    info.last_active = header.timestamp;

    // TODO: Send to desktop via message queue
    // For now, simulate response
    client_id_ = 1;
    connected_ = true;

    // Start listener thread
    running_ = true;
    listener_thread_ = std::thread(&desktop_connection::listener_loop,
this);

    return true;
}

void desktop_connection::disconnect() {
    if (!connected_) return;

    running_ = false;
    if (listener_thread_.joinable()) listener_thread_.join();

    // Send unregister message
    message_header header;
    header.type = message_type::unregister_client;
    header.client_id = client_id_;
    send_message(header, nullptr);

    if (shm_ptr_) munmap(shm_ptr_, shm_size_);
    if (shm_fd_ != -1) close(shm_fd_);

    connected_ = false;
}

void desktop_connection::listener_loop() {
    while (running_) {
        // TODO: Receive messages from desktop
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void desktop_connection::send_dom(const std::string& dom_data) {
    if (!connected_) return;

    message_header header;
    header.type = message_type::dom_update;
    header.client_id = client_id_;
    header.sequence = 0;
    header.payload_size = dom_data.size();

    header.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    send_message(header, dom_data.c_str());
}

void desktop_connection::send_buffer(const uint8_t* data, size_t size) {    if (!connected_ || !shm_ptr_) return;
    memcpy(shm_ptr_, data, std::min(size, shm_size_));

    message_header header;
    header.type = message_type::buffer_update;
    header.client_id = client_id_;
    header.payload_size = size;
    send_message(header, nullptr);
}

desktop_connection& desktop_connection::instance() {
    if (!g_connection) g_connection = new desktop_connection();
    return *g_connection;
}

//
```

```text
============================================================================// Element Implementation
```

```cpp
//
```

```text
============================================================================
```

```cpp
element::element(const std::string& tag) : tag_(tag) {}

element& element::add(std::unique_ptr<element> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
    return *this;
}

element& element::remove(element* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const auto& p) { return p.get() == child; });
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
    return *this;
}

void element::detach() {
    if (parent_) parent_->remove(this);
}

void element::clear() {
    for (auto& child : children_) child->parent_ = nullptr;
    children_.clear();
    text_data_.clear();
    text_.clear();
}

element& element::text(const std::string& t) {
    text_ = t;
    text_data_ = {t};
    return *this;
}

std::string element::text() const { return text_; }

element& element::operator<<(const std::string& str) {
    text_ += str;
    text_data_.push_back(str);
    return *this;
}

element& element::operator<<(const char* str) {
    *this << std::string(str);
    return *this;
}

void element::insert(const std::string& txt) {
    text_.insert(cursor_, txt);
    cursor_ += txt.length();
    text_data_ = {text_};
}

void element::erase_left() {
    if (cursor_ > 0) {
        text_.erase(cursor_ - 1, 1);
        cursor_--;
        text_data_ = {text_};
    }
}

void element::erase_right() {
    if (cursor_ < static_cast<int>(text_.length())) {
        text_.erase(cursor_, 1);
        text_data_ = {text_};
    }
}

void element::cursor_left() { if (cursor_ > 0) cursor_--; }
void element::cursor_right() { if (cursor_ < static_cast<int>
(text_.length())) cursor_++; }
void element::focus(bool f) { focus_ = f; }

std::string element::serialize() const {
    std::stringstream ss;
    ss << "<" << tag_;

    // Serialize attributes
    for (const auto& [type, value] : attrs_) {
        // TODO: Serialize each attribute type
    }

    ss << ">";
    ss << text_;

    for (const auto& child : children_) {
        ss << child->serialize();
    }

    ss << "</" << tag_ << ">";
    return ss.str();
}

//
```

```text
============================================================================// Client Application Implementation
```

```cpp
//
```

```text
============================================================================
```

```cpp
static client_app* g_client_app = nullptr;

client_app::client_app(const std::string& name) : app_name_(name) {
    g_client_app = this;
}

client_app::~client_app() {
    on_destroy();
}

bool client_app::init(int width, int height) {
    width_ = width;
    height_ = height;

    if (!desktop_connection::instance().connect(app_name_, width, height)) {        return false;
    }

    client_id_ = desktop_connection::instance().client_id();
    root_ = std::make_unique<document>();

    on_create();
    send_dom_update();

    return true;
}

void client_app::send_dom_update() {
    if (root_) {
        desktop_connection::instance().send_dom(root_->serialize());
    }
}

void client_app::run() {
    running_ = true;

    while (running_) {
        on_render();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void client_app::stop() {
    running_ = false;
}

void client_app::quit() {
    stop();
}

client_app* client_app::instance() {
    return g_client_app;
}

} // namespace cppux
```

### 5. examples/client/simple_app.cpp - Simple Client Example

```cpp
/**
 * @file simple_app.cpp
 * @brief Simple CPPUX client application
 *

 * Compile with: g++ -std=c++17 simple_app.cpp -lcppux_client -o simple_app */

#include "cppux/client.h"
#include <iostream>

using namespace cppux;

class hello_app : public client_app {
private:
    button* hello_button_;
    textbox* name_input_;
    paragraph* greeting_;

public:
    hello_app() : client_app("Hello World") {}

protected:
    void on_create() override {
        root_->attr(
            bg(color::white()),
            padding(20_px)
        );

        // Title
        auto& title = create<title>("CPPUX Hello World");
        root_->add(std::unique_ptr<title>(&title));

        // Description
        auto& desc = create<paragraph>("This is a simple CPPUX client
application.");
        root_->add(std::unique_ptr<paragraph>(&desc));
        // Name input
        auto& name_label = create<paragraph>("Enter your name:");
        root_->add(std::unique_ptr<paragraph>(&name_label));

        name_input_ = &create<textbox>("Your name here...");
        root_->add(std::unique_ptr<textbox>(name_input_));

        // Hello button
        hello_button_ = &create<button>("Say Hello");
        hello_button_->on_click([this](const serialized_event&) {
            std::string name = name_input_->value();
            if (name.empty()) name = "World";

            if (greeting_) {
                greeting_->detach();
            }

            greeting_ = &create<paragraph>("Hello, " + name + "!");
            root_->add(std::unique_ptr<paragraph>(greeting_));
        });
        root_->add(std::unique_ptr<button>(hello_button_));

        // Quit button
        auto& quit = create<button>("Quit");
        quit.attr(bg(color::red()));
        quit.on_click([this](const serialized_event&) {
            quit();
        });
        root_->add(std::unique_ptr<button>(&quit));
    }

    void on_event(const serialized_event& e) override {
        if (e.type == event_type::key_down && e.key_code ==
static_cast<uint16_t>(key::escape)) {
            quit();
        }
    }
};

int main(int argc, char** argv) {
    std::cout << "CPPUX Hello World Client v" << CPPUX_VERSION << std::endl;

    hello_app app;

    if (!app.init(400, 300)) {
        std::cerr << "Failed to initialize client" << std::endl;
        return 1;
    }
    app.run();
    return 0;
}
```

### 6. examples/desktop/desktop_main.cpp - Desktop Manager Example

```cpp
/**
 * @file desktop_main.cpp
 * @author Anthony Matarazzo
 * @brief CPPUX Desktop Manager - Main Entry Point
 */

#include "cppux/desktop.h"
#include <iostream>
#include <csignal>

static cppux::desktop_manager* g_desktop = nullptr;

void signal_handler(int sig) {
    std::cout << "\nShutting down CPPUX Desktop..." << std::endl;
    if (g_desktop) {
        g_desktop->stop();
    }
}

int main(int argc, char** argv) {
    std::cout << "CPPUX Desktop Manager v" << cppux::CPPUX_VERSION <<
std::endl;
    std::cout << "Copyright (c) 2026 Anthony Matarazzo" << std::endl;
    std::cout << "Licensed under MIT License" << std::endl;
    std::cout << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto& desktop = cppux::desktop_manager::instance();
    g_desktop = &desktop;

    if (!desktop.init()) {

        std::cerr << "Failed to initialize desktop manager" << std::endl;        return 1;
    }

    std::cout << "Desktop manager initialized successfully" << std::endl;    std::cout << "Features:" << std::endl;
    std::cout << "  - X11/Wayland window management" << std::endl;

    std::cout << "  - Shared memory client communication" << std::endl;    std::cout << "  - DOM-based UI rendering" << std::endl;
    std::cout << "  - Multiple workspaces" << std::endl;
    std::cout << "  - System tray and notifications" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    desktop.run();

    return 0;
}
```

### 7. CMakeLists.txt - Build Configuration

```cmake
cmake_minimum_required(VERSION 3.16)
project(CPPUX VERSION 4.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(CPPUX_X11_WM "Build X11 Window Manager" ON)
option(CPPUX_WAYLAND_WM "Build Wayland Compositor" ON)
option(CPPUX_SHARED_MEMORY "Enable Shared Memory" ON)
option(CPPUX_BUILD_DESKTOP "Build Desktop Manager" ON)
option(CPPUX_BUILD_CLIENT "Build Client Library" ON)

# Dependencies
find_package(PkgConfig REQUIRED)

if(CPPUX_X11_WM)

    pkg_check_modules(X11 REQUIRED x11 xcomposite xdamage xfixes xrender)    pkg_check_modules(XEXT REQUIRED xext)
endif()

if(CPPUX_WAYLAND_WM)
    pkg_check_modules(WAYLAND REQUIRED wayland-server wayland-client)
    pkg_check_modules(WLROOTS REQUIRED wlr-roots)
endif()

if(CPPUX_SHARED_MEMORY)
    find_library(RT rt)
    find_library(PTHREAD pthread)
endif()
#
```

```text
============================================================================# Client Library (Single header implementation)
```

```cmake
#
```

```text
============================================================================
```

```cmake
if(CPPUX_BUILD_CLIENT)
    add_library(cppux_client SHARED
        src/client/lib.cpp
    )

    target_include_directories(cppux_client PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(cppux_client
        ${RT}
        ${PTHREAD}
    )

    target_compile_definitions(cppux_client PUBLIC
        CPPUX_VERSION="${CPPUX_VERSION}"
        CPPUX_SHARED_MEMORY
    )

    # Create single-header client distribution
    add_custom_command(
        TARGET cppux_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/include/cppux/client.h
            ${CMAKE_BINARY_DIR}/cppux.h
    )
endif()

#
```

```text
============================================================================# Desktop Manager
```

```cmake
#
```

```text
============================================================================
```

```cmake
if(CPPUX_BUILD_DESKTOP)
    add_library(cppux_desktop SHARED
        src/desktop/shm_server.cpp
        src/desktop/renderer.cpp
        src/desktop/x11_wm.cpp
        src/desktop/wayland_wm.cpp
        src/desktop/desktop_manager.cpp
    )
    target_include_directories(cppux_desktop PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(cppux_desktop
        cppux_client
        ${X11_LIBRARIES}
        ${XEXT_LIBRARIES}
        ${WAYLAND_LIBRARIES}
        ${WLROOTS_LIBRARIES}
        ${RT}
        ${PTHREAD}
    )

    target_compile_definitions(cppux_desktop PUBLIC
        CPPUX_VERSION="${CPPUX_VERSION}"
        $<$<BOOL:${CPPUX_X11_WM}>:CPPUX_X11_WM>
        $<$<BOOL:${CPPUX_WAYLAND_WM}>:CPPUX_WAYLAND_WM>
        CPPUX_SHARED_MEMORY
    )

    # Desktop executable
    add_executable(cppux-desktop
        examples/desktop/desktop_main.cpp
    )

    target_link_libraries(cppux-desktop cppux_desktop)
endif()

#
```

```text
============================================================================# Client Examples
```

```cmake
#
```

```text
============================================================================
```

```cmake
add_executable(cppux-hello
    examples/client/simple_app.cpp
)

target_link_libraries(cppux-hello cppux_client)

#
```

```text
============================================================================# Installation
```

```cmake
#
```

```text
============================================================================
```

```cmake
install(TARGETS cppux_client cppux_desktop
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES
    include/cppux/common.h
    include/cppux/client.h
    include/cppux/desktop.h
    DESTINATION include/cppux
)

install(PROGRAMS
    ${CMAKE_BINARY_DIR}/cppux.h
    DESTINATION include
)

#
```

```text
============================================================================# Summary
```

```cmake
#
```

```text
============================================================================
```

```cmake
message(STATUS "")
message(STATUS "CPPUX Configuration:")
message(STATUS "  Version: ${CPPUX_VERSION}")
message(STATUS "  Build Desktop: ${CPPUX_BUILD_DESKTOP}")
message(STATUS "  Build Client: ${CPPUX_BUILD_CLIENT}")
message(STATUS "  X11 Support: ${CPPUX_X11_WM}")
message(STATUS "  Wayland Support: ${CPPUX_WAYLAND_WM}")
message(STATUS "  Shared Memory: ${CPPUX_SHARED_MEMORY}")
message(STATUS "")
```

## Summary

This client/server architecture provides:

### Security Benefits:

\1. Process Isolation - Clients run in separate processes, cannot access desktop memory directly

\2. Shared Memory Limits - Each client gets isolated shared memory regions

## 3. Message Validation - Desktop validates all client messages

\4. No Direct Hardware Access - Clients render through desktop, never touch GPU/display

### Client Development (Single Header):

```cpp
#include "cppux.h"  // Single header for clients

using namespace cppux;

class MyApp : public client_app {
    void on_create() override {
        root()->attr(bg(color::white()));
        auto& btn = create<button>("Click");

        btn.on_click([](auto&) { std::cout << "Clicked!" << std::endl; });        root()->add(std::unique_ptr<button>(&btn));
    }
};
```

### Desktop Manager:

Full X11/Wayland compositing

Hardware-accelerated rendering

Window management and tiling

System tray, notifications, workspaces

Client lifecycle management
