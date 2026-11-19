# Java Kernel and LOWL

**By Anthony Matarazzo (c) 2026**

*43-page technical reference, translated to Markdown from `Java Kernel and LOWL - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

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

- [LOWL Platform - Java Native Interface (JNI) Object System](#lowl-platform---java-native-interface-jni-object-system)
- [Architecture Overview](#architecture-overview)
- [Core Type Conversion Layer](#core-type-conversion-layer)
- [System Object](#system-object)
- [Desktop Manager Objects](#desktop-manager-objects)
- [Document Object Model (DOM) Objects](#document-object-model-dom-objects)
- [Graphics and Animation Objects](#graphics-and-animation-objects)
- [Input Objects](#input-objects)
- [Font and Typography Objects](#font-and-typography-objects)
- [Example Application](#example-application)
- [JNI Native Implementation (C++)](#jni-native-implementation-c)
- [Build Configuration](#build-configuration)
- [CMake Build for JNI Library](#cmake-build-for-jni-library)
- [Summary](#summary)
  - [Kerning, shape LayoutManager](#kerning-shape-layoutmanager)

---

## LOWL Platform - Java Native Interface (JNI) Object System

## Architecture Overview

The Java-LOWL Integration Layer provides complete access to the LOWL platform from Java applications. The system uses JNI to bridge between Java's object-oriented world and LOWL's C++ object system. All LOWL objects are wrapped as Java classes, with automatic type conversion between Java types and LOWL Box types. The visualization system is fully accessible through Java AWT/Swing integration, allowing Java applications to render LOWL documents, receive input events, and participate in the desktop environment.

```java
package lowl.java;

public class LOWL {
    static {
        System.loadLibrary("lowl_jni");
    }

    public static native void initialize();
    public static native void shutdown();
    public static native System getSystem();
    public static native DesktopManager getDesktopManager();
    public static native Security getSecurity();
    public static native Crypto getCrypto();
    public static native Compress getCompress();
    public static native Network getNetwork();
    public static native Font getFont();
    public static native Database getDatabase();
}
```

## Core Type Conversion Layer

```java
package lowl.java.types;

public class Box {
    private long nativeHandle;

    // Type enumeration
    public enum Type {
        VOID, BOOL, INT8, UINT8, INT16, UINT16, INT32, UINT32,
        INT64, UINT64, FLOAT32, FLOAT64, STRING, ARRAY, MAP, OBJECT
    }

    // Constructors for primitive types
    public Box() {}
    public Box(boolean value) { createBool(value); }
    public Box(byte value) { createInt8(value); }
    public Box(short value) { createInt16(value); }
    public Box(int value) { createInt32(value); }
    public Box(long value) { createInt64(value); }
    public Box(float value) { createFloat32(value); }
    public Box(double value) { createFloat64(value); }
    public Box(String value) { createString(value); }

    // Native methods
    private native void createBool(boolean value);
    private native void createInt8(byte value);
    private native void createInt16(short value);
    private native void createInt32(int value);
    private native void createInt64(long value);
    private native void createFloat32(float value);
    private native void createFloat64(double value);
    private native void createString(String value);

    public native Type getType();
    public native boolean asBool();
    public native byte asInt8();
    public native short asInt16();
    public native int asInt32();
    public native long asInt64();
    public native float asFloat32();
    public native double asFloat64();
    public native String asString();

    // Container operations
    public static native Box createArray();
    public static native Box createMap();
    public native void pushBack(Box value);
    public native Box get(int index);
    public native void put(String key, Box value);
    public native Box get(String key);
    public native int size();

    // Arithmetic operations
    public native Box add(Box other);
    public native Box subtract(Box other);
    public native Box multiply(Box other);
    public native Box divide(Box other);

    // Cleanup
    public native void close();
}
```

## System Object

```java
package lowl.java.system;

import lowl.java.types.Box;

public class System {
    private long nativeHandle;

    // System information
    public native String getHostname();
    public native void setHostname(String name);
    public native String getKernelVersion();
    public native long getUptimeSeconds();
    public native double getLoadAverage1Min();
    public native double getLoadAverage5Min();
    public native double getLoadAverage15Min();

    // Power management
    public native void shutdown();
    public native void reboot();
    public native void sleep();
    public native void suspend();
    public native void hibernate();

    // Process management
    public native Process[] getProcesses();
    public native Process getProcessByPid(int pid);
    public native Process getCurrentProcess();
    public native Process spawn(String path, String[] args);

    public native Process spawnWithEnv(String path, String[] args, String[] env);
    // Memory
    public native MemoryInfo getMemoryInfo();
    public native long getAllocatedMemory();
    public native long getTotalMemory();
    public native long getFreeMemory();

    // Time
    public native long getTimeMillis();
    public native long getTimeNanos();
    public native void sleep(long milliseconds);

    // Hardware
    public native CPU getCPU();
    public native GPU getGPU();
    public native Memory getMemory();
    public native Storage getStorage();
    public native NetworkInterface[] getNetworkInterfaces();
    public native USBDevice[] getUSBDevices();
    public native PCIDevice[] getPCIDevices();
    public native Sensor[] getSensors();
    public native Battery getBattery();

    // Namespace and isolation
    public native Namespace createNamespace(int type);
    public native void enterNamespace(Namespace ns);
    public native Namespace getCurrentNamespace();

    public static class MemoryInfo {
        public long total;
        public long free;
        public long available;
        public long cached;
        public long buffers;
        public long swapTotal;
        public long swapFree;
    }

    public static class CPU {
        public native String getVendor();
        public native String getModel();
        public native int getCores();
        public native int getThreads();
        public native int getFrequencyMHz();
        public native void setFrequencyMHz(int freq);
        public native double getTemperature();
        public native double getUsage();
        public native int[] getPerCoreUsage();
    }
    public static class GPU {
        public native String getVendor();
        public native String getModel();
        public native int getVRAMMB();
        public native double getTemperature();
        public native double getUsage();
        public native double getMemoryUsage();
        public native double getPowerDrawWatts();
    }
}

public class Process {
    private long nativeHandle;

    public native int getPid();
    public native String getName();
    public native int getPriority();
    public native void setPriority(int priority);
    public native int[] getCpuAffinity();
    public native void setCpuAffinity(int[] cpus);
    public native long getMemoryUsage();
    public native double getCpuUsage();
    public native void suspend();
    public native void resume();
    public native void terminate();
    public native void terminate(int signal);
    public native int waitFor();
    public native boolean isRunning();
    public native String[] getCommandLine();
    public native String[] getEnvironment();
    public native Process[] getChildren();
    public native Process getParent();
}

public class Namespace {
    private long nativeHandle;

    public static final int MOUNT = 0x00020000;
    public static final int NETWORK = 0x00040000;
    public static final int PID = 0x00080000;
    public static final int USER = 0x00100000;
    public static final int UTS = 0x00200000;
    public static final int IPC = 0x00400000;
    public static final int CGROUP = 0x00800000;

    public native int getType();
    public native int getId();
    public native void enter();
    public native void exit();
}
```

## Desktop Manager Objects

```java
package lowl.java.desktop;

import lowl.java.dom.Document;
import lowl.java.dom.Element;
import lowl.java.types.Box;
import java.awt.Component;
import java.awt.Rectangle;

public class DesktopManager {
    private long nativeHandle;

    public static native DesktopManager getInstance();

    // Lifecycle
    public native void initialize();
    public native void run();
    public native void shutdown();

    // Container management
    public native VisualContainer createContainer();
    public native VisualContainer createShapeContainer();
    public native void addContainer(VisualContainer container);
    public native void removeContainer(VisualContainer container);
    public native VisualContainer[] getContainers();
    public native VisualContainer getActiveContainer();
    public native void setActiveContainer(VisualContainer container);

    // Container operations

    public native void moveContainer(VisualContainer container, int x, int y);

    public native void resizeContainer(VisualContainer container, int width, int height);
    public native void focusContainer(VisualContainer container);
    public native void closeContainer(VisualContainer container);
    public native void minimizeContainer(VisualContainer container);
    public native void maximizeContainer(VisualContainer container);
    public native void restoreContainer(VisualContainer container);

    // Workspace management
    public native void setWorkspaceCount(int count);
    public native int getWorkspaceCount();
    public native void switchWorkspace(int workspace);
    public native int getCurrentWorkspace();

    public native void moveContainerToWorkspace(VisualContainer container, int workspace);
    public native VisualContainer[] getContainersOnWorkspace(int workspace);

    // Desktop appearance
    public native void setWallpaper(String path);
    public native void setWallpaper(Document doc);
    public native String getWallpaper();

    public native void setBackgroundColor(float r, float g, float b, float a);
    public native void setCursorTheme(String theme);
    public native void setCursorShape(String shape);

    // Lock screen
    public native void lockScreen();
    public native void unlockScreen();
    public native boolean isLocked();
    public native void setLockScreenDocument(Document doc);

    // Input routing
    public native void setInputRouter(InputRouter router);
    public native InputRouter getInputRouter();

    // Notifications

    public native void showNotification(String title, String message);    public native void showNotification(String title, String message, String type);
    public native NotificationCenter getNotificationCenter();

    // Screen capture
    public native byte[] captureFullscreen();

    public native byte[] captureRegion(int x, int y, int width, int height);    public native byte[] captureContainer(VisualContainer container);}

public class VisualContainer {
    private long nativeHandle;

    // Shape configuration

    public native void setShapeRectangle(int x, int y, int width, int height);
    public native void setShapeRectangle(Rectangle rect);

    public native void setShapeRoundedRectangle(int x, int y, int width, int height, int radius);

    public native void setShapeOval(int centerX, int centerY, int radiusX, int radiusY);
    public native void setShapePolygon(int[] xPoints, int[] yPoints);
    public native void setShapeFreeform(String svgPath);
    public native void setShapeFromDefinition(ShapeDefinition shape);

    // Content
    public native void setContentDocument(Document doc);
    public native Document getContentDocument();

    public native void setContentComponent(Component swingComponent);    public native void setContentBox(Box content);

    // Appearance

    public native void setBackgroundColor(float r, float g, float b, float a);
    public native void setBackgroundImage(String path);
    public native void setBackgroundBlur(float radius);
    public native void setBackdropFilter(String filter);

    public native void setBorder(float width, float r, float g, float b, float a);
    public native void setBorderRadius(int radius);
    public native void setBoxShadow(int offsetX, int offsetY, int
blurRadius, int spreadRadius, float r, float g, float b, float a);
    public native void setOpacity(float opacity);

    // Transform
    public native void setPosition(int x, int y);
    public native void setSize(int width, int height);
    public native void setTransform(Transform transform);
    public native void translate(int dx, int dy);
    public native void rotate(float degrees);
    public native void scale(float sx, float sy);

    // State
    public native void show();
    public native void hide();
    public native boolean isVisible();
    public native void setActive(boolean active);
    public native boolean isActive();
    public native void setModal(boolean modal);
    public native boolean isModal();
    public native void setResizable(boolean resizable);
    public native boolean isResizable();
    public native void setClosable(boolean closable);
    public native boolean isClosable();
    public native void setMovable(boolean movable);
    public native boolean isMovable();

    // Z-order
    public native void setZOrder(int zOrder);
    public native int getZOrder();
    public native void raise();
    public native void lower();
    public native void bringToFront();
    public native void sendToBack();

    // Animation
    public native void setAnimation(Animation animation);

    public native void animateProperty(String property, Box from, Box to, int durationMs);

    public native void animateTransform(Transform from, Transform to, int durationMs);

    // Events

    public native void addContainerListener(ContainerListener listener);    public native void removeContainerListener(ContainerListener listener);

    // Cleanup
    public native void close();
    public native void destroy();
}

public class ShapeDefinition {
    private long nativeHandle;

    public static native ShapeDefinition createRectangle(int x, int y, int width, int height);

    public static native ShapeDefinition createRoundedRectangle(int x, int y, int width, int height, int radius);

    public static native ShapeDefinition createOval(int centerX, int centerY, int radiusX, int radiusY);

    public static native ShapeDefinition createPolygon(int[] xPoints, int[] yPoints);

    public static native ShapeDefinition createStar(int centerX, int centerY, int points, int innerRadius, int outerRadius);

    public static native ShapeDefinition createRegularPolygon(int centerX, int centerY, int sides, int radius);
    public static native ShapeDefinition createFromSVG(String svgPath);

    public native void setCornerRadius(int radius);
    public native void setCornerSmoothing(float smoothing);
    public native boolean containsPoint(int x, int y);
    public native Rectangle getBounds();
    public native void transform(Transform transform);
}

public class Transform {
    public float m00, m01, m02;
    public float m10, m11, m12;
    public float m20, m21, m22;
    public Transform() {
        m00 = m11 = m22 = 1.0f;
    }

    public static Transform identity() {
        return new Transform();
    }

    public static Transform translation(float x, float y) {
        Transform t = new Transform();
        t.m02 = x;
        t.m12 = y;
        return t;
    }

    public static Transform rotation(float angleDegrees) {
        float rad = (float)Math.toRadians(angleDegrees);
        float cos = (float)Math.cos(rad);
        float sin = (float)Math.sin(rad);
        Transform t = new Transform();
        t.m00 = cos; t.m01 = -sin;
        t.m10 = sin; t.m11 = cos;
        return t;
    }

    public static Transform scale(float sx, float sy) {
        Transform t = new Transform();
        t.m00 = sx;
        t.m11 = sy;
        return t;
    }

    public native Transform multiply(Transform other);
}

public interface ContainerListener {
    void containerShown(VisualContainer container);
    void containerHidden(VisualContainer container);

    void containerMoved(VisualContainer container, int oldX, int oldY);    void containerResized(VisualContainer container, int oldWidth, int oldHeight);
    void containerFocused(VisualContainer container);
    void containerClosed(VisualContainer container);
}
```

## Document Object Model (DOM) Objects

```java
package lowl.java.dom;

import lowl.java.types.Box;
import java.util.List;
import java.util.Map;

public class Document {
    private long nativeHandle;

    public static native Document create();
    public native void parse(String html);
    public native String serialize();
    public native void setTitle(String title);
    public native String getTitle();
    public native void setViewport(int width, int height, float scale);

    // Element creation
    public native Element createElement(String tagName);
    public native Element createTextNode(String text);
    public native Element createComment(String text);
    public native Element createDocumentFragment();

    // Query methods
    public native Element getElementById(String id);

    public native Element[] getElementsByClassName(String className);    public native Element[] getElementsByTagName(String tagName);
    public native Element querySelector(String selector);
    public native Element[] querySelectorAll(String selector);

    // Tree navigation
    public native Element getDocumentElement();
    public native Element getBody();
    public native void setBody(Element body);
    public native Element getHead();
    public native void setHead(Element head);

    // Style sheets
    public native void addStyleSheet(String css);
    public native void removeStyleSheet(int index);
    public native void addStyleRule(String selector, String declarations);

    // Events

    public native void addEventListener(String type, DOMEventListener listener);

    public native void removeEventListener(String type, DOMEventListener listener);
    public native void dispatchEvent(DOMEvent event);
    // Rendering
    public native void render();
    public native void reflow();
    public native void repaint();
    public native Box getComputedStyle(Element element, String property);

    // Cleanup
    public native void close();
}

public class Element {
    private long nativeHandle;

    // Attributes
    public native String getAttribute(String name);
    public native void setAttribute(String name, String value);
    public native void removeAttribute(String name);
    public native boolean hasAttribute(String name);
    public native Map<String, String> getAttributes();

    // Classes
    public native String getClassName();
    public native void setClassName(String className);
    public native void addClass(String className);
    public native void removeClass(String className);
    public native void toggleClass(String className);
    public native boolean hasClass(String className);

    // Styles
    public native String getStyle(String property);
    public native void setStyle(String property, String value);
    public native void setStyles(Map<String, String> styles);
    public native Box getComputedStyle(String property);

    // Content
    public native String getInnerHTML();
    public native void setInnerHTML(String html);
    public native String getInnerText();
    public native void setInnerText(String text);
    public native String getOuterHTML();
    public native void setOuterHTML(String html);

    // Tree manipulation
    public native Element getParent();
    public native Element[] getChildren();
    public native Element getFirstChild();
    public native Element getLastChild();
    public native Element getNextSibling();
    public native Element getPreviousSibling();
    public native void appendChild(Element child);

    public native void insertBefore(Element newChild, Element refChild);    public native void removeChild(Element child);

    public native void replaceChild(Element newChild, Element oldChild);    public native Element cloneNode(boolean deep);

    // Geometry
    public native Rectangle getBoundingClientRect();
    public native Rectangle[] getClientRects();
    public native int getOffsetWidth();
    public native int getOffsetHeight();
    public native int getOffsetLeft();
    public native int getOffsetTop();
    public native Element getOffsetParent();
    public native int getScrollWidth();
    public native int getScrollHeight();
    public native int getScrollLeft();
    public native void setScrollLeft(int value);
    public native int getScrollTop();
    public native void setScrollTop(int value);

    // Events

    public native void addEventListener(String type, DOMEventListener listener);

    public native void removeEventListener(String type, DOMEventListener listener);
    public native void dispatchEvent(DOMEvent event);
    public native void click();
    public native void focus();
    public native void blur();

    // State
    public native boolean isVisible();
    public native void setVisible(boolean visible);
    public native boolean isEnabled();
    public native void setEnabled(boolean enabled);

    // Cleanup
    public native void remove();
}

public interface DOMEventListener {
    void handleEvent(DOMEvent event);
}

public class DOMEvent {
    private long nativeHandle;

    public enum Phase {
        NONE, CAPTURING, AT_TARGET, BUBBLING
    }

    public native String getType();
    public native Element getTarget();
    public native Element getCurrentTarget();
    public native Phase getEventPhase();
    public native long getTimestamp();
    public native boolean isBubbles();
    public native boolean isCancelable();
    public native void stopPropagation();
    public native void stopImmediatePropagation();
    public native void preventDefault();
    public native boolean isDefaultPrevented();

    // Mouse-specific
    public native int getClientX();
    public native int getClientY();
    public native int getScreenX();
    public native int getScreenY();
    public native int getButton();
    public native int getButtons();

    // Keyboard-specific
    public native String getKey();
    public native String getCode();
    public native boolean isAltKey();
    public native boolean isCtrlKey();
    public native boolean isShiftKey();
    public native boolean isMetaKey();

    // Touch-specific
    public native Touch[] getTouches();
    public native Touch[] getTargetTouches();
    public native Touch[] getChangedTouches();
}

public class Touch {
    public int identifier;
    public int clientX;
    public int clientY;
    public int screenX;
    public int screenY;
    public float radiusX;
    public float radiusY;
    public float rotationAngle;
    public float force;
}
```

## Graphics and Animation Objects

```java
package lowl.java.graphics;

import lowl.java.types.Box;
import lowl.java.dom.Document;

public class Compositor {
    private long nativeHandle;

    public static native Compositor getInstance();

    // Layer management
    public native void addLayer(Layer layer);
    public native void removeLayer(Layer layer);
    public native void setLayerOrder(Layer[] layers);
    public native Layer getLayerAt(int x, int y);
    public native Layer[] getLayers();

    // Compositing

    public native void setBackgroundColor(float r, float g, float b, float a);
    public native void setBackgroundTexture(Texture texture);
    public native void setWallpaper(Document wallpaper);
    public native void render();
    public native void present();
    public native void setVsync(boolean enabled);
    public native void setMaxFPS(int fps);

    // Effects
    public native void addEffect(Layer layer, Effect effect);
    public native void removeEffect(Layer layer, Effect effect);
    public native void setGlobalEffect(Effect effect);

    // Workspaces
    public native void setWorkspaceCount(int count);
    public native void switchWorkspace(int index);
    public native int getCurrentWorkspace();
    public native void setWorkspaceTransition(String animation, int
durationMs);
}

public class Layer {
    private long nativeHandle;

    public static native Layer create();
    public native void setContent(Document doc);
    public native void setContent(Texture texture);
    public native void setContent(Box box);
    public native void setOpacity(float opacity);
    public native void setTransform(Transform transform);
    public native void setClip(Rectangle clip);
    public native void setBlendMode(String mode);
    public native void setVisible(boolean visible);
    public native boolean isVisible();
    public native void setZOrder(int zOrder);
    public native int getZOrder();
    public native void destroy();
}

public class Shader {
    private long nativeHandle;

    public static native Shader create();
    public native void loadVertex(String source);
    public native void loadFragment(String source);
    public native void loadCompute(String source);
    public native void compile();
    public native void bind();
    public native void unbind();
    public native void setUniform(String name, float value);
    public native void setUniform(String name, float[] values);
    public native void setUniform(String name, int value);
    public native void setUniform(String name, int[] values);
    public native void setUniform(String name, Texture texture);
    public native void setUniform(String name, Matrix4f matrix);
    public native void dispatch(int x, int y, int z);
    public native String getCompilationLog();
    public native void destroy();
}

public class Texture {
    private long nativeHandle;

    public static native Texture create();
    public native void loadFromFile(String path);

    public native void loadFromMemory(byte[] data, int width, int height, int format);

    public native void loadFromDocument(Document doc, int width, int height);
    public native void upload(byte[] pixels);
    public native void bind(int unit);
    public native void unbind();
    public native int getWidth();
    public native int getHeight();
    public native void setFilter(String minFilter, String magFilter);
    public native void setWrap(String sWrap, String tWrap);
    public native void generateMipmaps();
    public native void destroy();
}

public class Canvas {
    private long nativeHandle;

    public static native Canvas create(int width, int height);
    public native void beginFrame();
    public native void endFrame();
    public native void clear();
    public native void clear(float r, float g, float b, float a);

    public native void drawRect(int x, int y, int width, int height);    public native void drawRect(int x, int y, int width, int height, Paint paint);

    public native void drawRoundedRect(int x, int y, int width, int height, int radius);
    public native void drawCircle(int cx, int cy, int radius);
    public native void drawLine(int x1, int y1, int x2, int y2);
    public native void drawText(String text, int x, int y);

    public native void drawText(String text, int x, int y, Font font);    public native void drawImage(Texture texture, int x, int y);

    public native void drawImage(Texture texture, int x, int y, int width, int height);

    public native void drawImageSub(Texture texture, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY, int dstW, int dstH);
    public native void drawTexture(Texture texture, int x, int y);
    public native void drawPath(Path path);
    public native void pushTransform();
    public native void popTransform();
    public native void translate(float x, float y);
    public native void rotate(float degrees);
    public native void scale(float sx, float sy);
    public native void setClip(int x, int y, int width, int height);
    public native void resetClip();
    public native void save();
    public native void restore();
    public native byte[] toPNG();
    public native byte[] toJPEG(int quality);
    public native void destroy();
}

public class Paint {
    public float r, g, b, a;
    public float strokeWidth;
    public int style; // 0 = fill, 1 = stroke, 2 = fill and stroke

    public Paint() {
        r = g = b = 1.0f;
        a = 1.0f;
        strokeWidth = 1.0f;
    }

    public static Paint color(float r, float g, float b) {
        Paint p = new Paint();
        p.r = r;
        p.g = g;
        p.b = b;
        return p;
    }

    public static Paint color(float r, float g, float b, float a) {
        Paint p = new Paint();
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = a;
        return p;
    }

    public Paint stroke(float width) {
        this.strokeWidth = width;
        this.style = 1;
        return this;
    }

    public Paint fill() {
        this.style = 0;
        return this;
    }
}

public class Path {
    private long nativeHandle;

    public static native Path create();
    public native void moveTo(float x, float y);
    public native void lineTo(float x, float y);
    public native void quadTo(float cx, float cy, float x, float y);

    public native void cubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    public native void close();

    public native void addRect(float x, float y, float width, float height);    public native void addCircle(float cx, float cy, float radius);

    public native void addOval(float cx, float cy, float rx, float ry);    public native void addArc(float cx, float cy, float radius, float startAngle, float sweepAngle);
    public native void transform(Transform transform);
    public native void destroy();
}

public class Animation {
    private long nativeHandle;

    public static native Animation create();
    public native void setDuration(int milliseconds);
    public native void setDelay(int milliseconds);
    public native void setEasing(String easing);
    public native void setIterations(int iterations);
    public native void setDirection(String direction);
    public native void setFillMode(String fillMode);

    public native void addKeyframe(float time, String property, Box value);    public native void addKeyframe(float time, String property, Box value, String easing);
    public native void start();
    public native void stop();
    public native void pause();
    public native void resume();
    public native void reverse();
    public native void seek(float progress);
    public native float getProgress();
    public native boolean isRunning();
    public native void onComplete(Runnable callback);
    public native void onUpdate(AnimationUpdateListener listener);
    public native void destroy();
}

public interface AnimationUpdateListener {
    void onUpdate(float progress, Map<String, Box> currentValues);
}

public class Tween {
    private long nativeHandle;

    public static native Tween create();
    public native void from(Box value);
    public native void to(Box value);
    public native void setDuration(int milliseconds);
    public native void setEasing(String easing);
    public native void setDelay(int milliseconds);
    public native void start();
    public native void stop();
    public native void pause();
    public native void resume();
    public native void reverse();
    public native void seek(float progress);
    public native Box getValue();
    public native float getProgress();
    public native boolean isRunning();
    public native void onComplete(Runnable callback);
    public native void onUpdate(TweenUpdateListener listener);
    public native void destroy();
}

public interface TweenUpdateListener {
    void onUpdate(Box value);
}

public class ParticleSystem {
    private long nativeHandle;

    public static native ParticleSystem create();
    public native void setMaxParticles(int max);
    public native void setEmissionRate(int rate);
    public native void setParticleLifetime(float lifetime);
    public native void setGravity(float x, float y);
    public native void setWind(float x, float y);

    public native void addForceField(float x, float y, float radius, float strength);
    public native void createEmitter();
    public native void destroyEmitter(int emitterId);
    public native void emit(int count);
    public native void update(float deltaTime);
    public native void render();
    public native int getActiveParticleCount();
    public native void destroy();
}
```

## Input Objects

```java
package lowl.java.input;

public class Keyboard {
    private long nativeHandle;

    public static native Keyboard getInstance();

    // Layout
    public native void setLayout(String layoutName);
    public native String getLayout();
    public native String[] getAvailableLayouts();
    // Key states
    public native boolean isKeyPressed(int keyCode);
    public native boolean isKeyReleased(int keyCode);
    public native boolean isKeyToggled(int keyCode);
    public native int[] getPressedKeys();

    // Shortcuts

    public native void registerShortcut(String name, String combination, Runnable callback);
    public native void unregisterShortcut(String name);
    public native void registerGlobalShortcut(String name, String
combination, Runnable callback);
    public native void unregisterGlobalShortcut(String name);

    // Repeat settings
    public native void setRepeatDelay(int milliseconds);
    public native int getRepeatDelay();
    public native void setRepeatInterval(int milliseconds);
    public native int getRepeatInterval();

    // LEDs
    public native void setLedState(int led, boolean on);
    public native boolean getLedState(int led);

    // Grab
    public native void grab();
    public native void ungrab();
    public native boolean isGrabbed();

    // Key codes
    public static final int KEY_NONE = 0;
    public static final int KEY_BACKSPACE = 8;
    public static final int KEY_TAB = 9;
    public static final int KEY_ENTER = 13;
    public static final int KEY_ESCAPE = 27;
    public static final int KEY_SPACE = 32;
    public static final int KEY_LEFT = 37;
    public static final int KEY_UP = 38;
    public static final int KEY_RIGHT = 39;
    public static final int KEY_DOWN = 40;
    public static final int KEY_A = 65;
    public static final int KEY_B = 66;
    public static final int KEY_C = 67;
    public static final int KEY_D = 68;
    public static final int KEY_E = 69;
    public static final int KEY_F = 70;
    public static final int KEY_G = 71;
    public static final int KEY_H = 72;
    public static final int KEY_I = 73;
    public static final int KEY_J = 74;
    public static final int KEY_K = 75;
    public static final int KEY_L = 76;
    public static final int KEY_M = 77;
    public static final int KEY_N = 78;
    public static final int KEY_O = 79;
    public static final int KEY_P = 80;
    public static final int KEY_Q = 81;
    public static final int KEY_R = 82;
    public static final int KEY_S = 83;
    public static final int KEY_T = 84;
    public static final int KEY_U = 85;
    public static final int KEY_V = 86;
    public static final int KEY_W = 87;
    public static final int KEY_X = 88;
    public static final int KEY_Y = 89;
    public static final int KEY_Z = 90;
    public static final int KEY_F1 = 112;
    public static final int KEY_F2 = 113;
    public static final int KEY_F3 = 114;
    public static final int KEY_F4 = 115;
    public static final int KEY_F5 = 116;
    public static final int KEY_F6 = 117;
    public static final int KEY_F7 = 118;
    public static final int KEY_F8 = 119;
    public static final int KEY_F9 = 120;
    public static final int KEY_F10 = 121;
    public static final int KEY_F11 = 122;
    public static final int KEY_F12 = 123;
}

public class Mouse {
    private long nativeHandle;

    public static native Mouse getInstance();

    // Position
    public native int getX();
    public native int getY();
    public native void setPosition(int x, int y);
    public native void warp(int x, int y);

    // Buttons
    public native boolean isButtonPressed(int button);
    public native boolean isButtonReleased(int button);
    public native int getButtons();

    // Speed and acceleration
    public native void setSpeed(int speed);
    public native int getSpeed();
    public native void setAcceleration(String profile);
    public native String getAcceleration();

    // Button mapping

    public native void setButtonMapping(int physicalButton, String action);    public native String getButtonMapping(int physicalButton);
    public native void swapButtons(boolean swap);
    public native boolean areButtonsSwapped();

    // Cursor
    public native void setCursorTheme(String theme);

    public native void setCustomCursor(String imagePath, int hotX, int hotY);
    public native void hideCursor();
    public native void showCursor();
    public native boolean isCursorVisible();

    // Scroll
    public native void setScrollSpeed(int speed);
    public native int getScrollSpeed();
    public native int getScrollDelta();

    // Grab
    public native void grab();
    public native void ungrab();
    public native boolean isGrabbed();
    public native void setPointerConstraint(Rectangle bounds);
    public native void clearPointerConstraint();

    // Button constants
    public static final int BUTTON_NONE = 0;
    public static final int BUTTON_LEFT = 1;
    public static final int BUTTON_RIGHT = 2;
    public static final int BUTTON_MIDDLE = 3;
    public static final int BUTTON_BACK = 4;
    public static final int BUTTON_FORWARD = 5;
}

public class Touchpad {
    private long nativeHandle;

    public static native Touchpad getInstance();

    // Gestures
    public native void enableGesture(String gesture);
    public native void disableGesture(String gesture);
    public native boolean isGestureEnabled(String gesture);
    public native void setGestureHandler(String gesture, GestureHandler
handler);

    // Scroll
    public native void setScrollSpeed(float speed);
    public native float getScrollSpeed();
    public native void setNaturalScrolling(boolean enabled);
    public native boolean isNaturalScrollingEnabled();
    public native void setTwoFingerScroll(boolean enabled);
    public native boolean isTwoFingerScrollEnabled();

    // Tap
    public native void setTapToClick(boolean enabled);
    public native boolean isTapToClickEnabled();
    public native void setTapDrag(boolean enabled);
    public native boolean isTapDragEnabled();
    public native void setTapTimeout(int milliseconds);
    public native int getTapTimeout();

    // Palm rejection
    public native void setPalmRejection(boolean enabled);
    public native boolean isPalmRejectionEnabled();
    public native void setPalmRejectionSize(float sizeMm);
    public native float getPalmRejectionSize();

    // Finger tracking
    public native int getFingerCount();
    public native TouchPoint[] getTouchPoints();
    public native float getPressure();

    // Calibration
    public native void calibrate();
    public native void resetCalibration();

    public interface GestureHandler {
        void onGesture(String gesture, GestureData data);
    }

    public static class GestureData {
        public float scale;
        public float rotation;
        public float velocityX;
        public float velocityY;
        public float angularVelocity;
        public int fingerCount;
        public float[] touchX;
        public float[] touchY;
    }

    public static class TouchPoint {
        public int id;
        public float x;
        public float y;
        public float pressure;
        public float radiusX;
        public float radiusY;
    }
}

public class Gamepad {
    private long nativeHandle;

    public static native Gamepad getInstance();

    // Device management
    public native GamepadInfo[] getDevices();
    public native void setActiveDevice(String deviceId);
    public native String getActiveDevice();
    public native void calibrate(String deviceId);

    // Axis
    public native void setDeadzone(int axis, int value);
    public native int getDeadzone(int axis);
    public native void setSensitivity(int axis, float sensitivity);
    public native float getSensitivity(int axis);
    public native int getAxisValue(int axis);
    public native float getAxisNormalized(int axis);

    // Buttons
    public native boolean isButtonPressed(int button);
    public native boolean isButtonReleased(int button);
    public native int getButtons();

    // Rumble

    public native void rumble(int strongMagnitude, int weakMagnitude);    public native void rumble(int strongMagnitude, int weakMagnitude, int durationMs);
    public native void rumbleLeft(int magnitude);
    public native void rumbleRight(int magnitude);
    public native void rumbleTrigger(int trigger, int magnitude);
    public native void stopRumble();
    public native boolean isRumbleSupported();

    // Button mapping

    public native void remapButton(int physicalButton, int logicalButton);    public native void resetButtonMapping();
    public native int getButtonMapping(int physicalButton);

    // State
    public native GamepadState getState();

    // Events
    public native void onButton(int button, ButtonHandler handler);
    public native void onAxis(int axis, AxisHandler handler);
    public native void onConnection(ConnectionHandler handler);

    public interface ButtonHandler {
        void onButtonPressed(int button);
        void onButtonReleased(int button);
    }

    public interface AxisHandler {
        void onAxisMoved(int axis, int value, float normalized);
    }

    public interface ConnectionHandler {
        void onConnected(String deviceId);
        void onDisconnected(String deviceId);
    }

    public static class GamepadInfo {
        public String deviceId;
        public String name;
        public String vendor;
        public int productId;
        public boolean isWireless;
        public int batteryLevel;
    }

    public static class GamepadState {
        public int[] axes;
        public boolean[] buttons;
        public long timestamp;
    }

    // Axis constants
    public static final int AXIS_LEFT_X = 0;
    public static final int AXIS_LEFT_Y = 1;
    public static final int AXIS_RIGHT_X = 2;
    public static final int AXIS_RIGHT_Y = 3;
    public static final int AXIS_TRIGGER_LEFT = 4;
    public static final int AXIS_TRIGGER_RIGHT = 5;

    // Button constants
    public static final int BUTTON_A = 0;
    public static final int BUTTON_B = 1;
    public static final int BUTTON_X = 2;
    public static final int BUTTON_Y = 3;
    public static final int BUTTON_LEFT_BUMPER = 4;
    public static final int BUTTON_RIGHT_BUMPER = 5;
    public static final int BUTTON_BACK = 6;
    public static final int BUTTON_START = 7;
    public static final int BUTTON_GUIDE = 8;
    public static final int BUTTON_LEFT_THUMB = 9;
    public static final int BUTTON_RIGHT_THUMB = 10;
    public static final int BUTTON_DPAD_UP = 11;
    public static final int BUTTON_DPAD_DOWN = 12;
    public static final int BUTTON_DPAD_LEFT = 13;
    public static final int BUTTON_DPAD_RIGHT = 14;
}
```

## Font and Typography Objects

```java
package lowl.java.font;

import lowl.java.graphics.Path;
import lowl.java.types.Box;
import java.awt.Rectangle;

public class FontDescriptor {
    private long nativeHandle;

    // Creation

    public static native FontDescriptor create(String id, String family);    public static native FontDescriptor loadFromFile(String path);
    public static native FontDescriptor loadFromMemory(byte[] data);

    // Font properties
    public native void setPointSize(float size);
    public native float getPointSize();
    public native void setStyle(String style);
    public native String getStyle();
    public native String getFamily();
    public native String getId();

    // Metrics
    public native FontMetrics getMetrics();
    public native float getScaleFactor();
    public native float measureText(String text);
    public native float measureChar(char c);
    public native Rectangle getGlyphBounds(char c);
    public native Path getGlyphPath(char c);

    // Kerning
    public native void setKerningMode(KerningMode mode);
    public native KerningMode getKerningMode();
    public native void addKerningPair(char left, char right, float
adjustment);

    public native void addKerningPairByName(String left, String right, float adjustment);

    public native void addContextualKerning(String context, String left, String right, float adjustment);
    public native void loadKerningFromFont(String fontPath);
    public native float getKerning(char left, char right);

    public native float getKerning(char left, char right, String context);    public native void setTracking(float tracking);
    public native float getTracking();
    public native float calculateAutoKerning(char left, char right);

    // Context presets
    public native void applyHeadlineKerning();
    public native void applyBodyKerning();
    public native void setContext(String context);

    // Glyph management
    public native void addGlyph(GlyphData glyph);
    public native GlyphData getGlyph(char c);
    public native boolean hasGlyph(char c);

    // Advanced features
    public native void enableLigatures(boolean enable);
    public native boolean areLigaturesEnabled();
    public native void setHinting(boolean enable);
    public native void setSubpixelRendering(boolean enable);

    // Cleanup
    public native void close();

    public enum KerningMode {
        NONE, AUTO, FONT_PROVIDED, MANUAL, HYBRID
    }

    public static class FontMetrics {
        public float ascender;
        public float descender;
        public float lineGap;
        public float xHeight;
        public float capHeight;
        public float unitsPerEm;
        public float getLineHeight(float multiplier) {
            return (ascender - descender + lineGap) * multiplier;
        }
    }

    public static class GlyphData {
        public char codepoint;
        public String name;
        public Path outline;
        public Rectangle bounds;
        public float advance;
        public float leftBearing;
        public float rightBearing;

        public GlyphData(char c) {
            this.codepoint = c;
        }
    }
}

public class LayoutManager {
    private long nativeHandle;

    // Construction
    public static native LayoutManager create(FontDescriptor font);

    // Shape configuration
    public native void setShapeRectangle(Rectangle rect);

    public native void setShapeRectangle(int x, int y, int width, int height);

    public native void setShapeOval(int centerX, int centerY, int radiusX, int radiusY);

    public native void setShapeOval(int cx, int cy, int rx, int ry, float rotation);

    public native void setShapePolygon(int[] xPoints, int[] yPoints);    public native void setShapeFromPath(Path path);
    public native void setShapeFreeform(String svgPath);

    // Text layout
    public native void layoutTextInShape(String text);

    public native void layoutTextOnPath(String text, float startAngle);    public native void layoutTextOnPath(String text, Path path);

    public native void layoutTextOnPath(String text, Path path, float startOffset);

    // Layout parameters
    public native void setLineHeight(float multiplier);
    public native float getLineHeight();
    public native void setLetterSpacing(float spacing);
    public native float getLetterSpacing();
    public native void setWordSpacing(float spacing);
    public native float getWordSpacing();
    public native void setTextAlign(String align);
    public native String getTextAlign();
    public native void setIndent(float indent);
    public native float getIndent();
    public native void setHyphenation(boolean enable);
    public native boolean isHyphenationEnabled();

    // Kerning and context

    public native void setKerningMode(FontDescriptor.KerningMode mode);    public native FontDescriptor.KerningMode getKerningMode();
    public native void setContext(String context);
    public native String getContext();

    // Query methods
    public native GlyphElement[] getPlacedGlyphs();
    public native ImageRegion getImageRegionForRect(Rectangle rect);
    public native ImageRegion[] getAllImageRegions();
    public native float getAlphaAtPoint(int x, int y);
    public native Rectangle getBounds();
    public native Rectangle getContentBounds();

    // Visitor pattern
    public native void accept(Visitor visitor);

    // Cleanup
    public native void close();

    public interface Visitor {
        void visitGlyph(GlyphElement glyph);
        void visitWord(WordElement word);
        void visitLine(LineElement line);
        void visitParagraph(ParagraphElement para);
        void visitImage(ImageElement image);
        void visitContainer(LayoutContainer container);
    }
}

public class GlyphElement {
    private long nativeHandle;

    public native char getCharacter();
    public native int getCharCode();
    public native String getGlyphName();
    public native Path getOutline();
    public native Rectangle getBounds();
    public native Transform getTransform();
    public native float getAdvance();
    public native ImageRegion getImageRegion();
    public native AlphaInfo getAlphaForRegion(Rectangle rect);
    public native void accept(Visitor visitor);
}

public class WordElement {
    private long nativeHandle;

    public native String getText();
    public native GlyphElement[] getGlyphs();
    public native Rectangle getBounds();
    public native void accept(Visitor visitor);
}

public class LineElement {
    private long nativeHandle;

    public native WordElement[] getWords();
    public native Rectangle getBounds();
    public native void accept(Visitor visitor);
}

public class ParagraphElement {
    private long nativeHandle;

    public native LineElement[] getLines();
    public native Rectangle getBounds();
    public native void accept(Visitor visitor);
}

public class ImageElement {
    private long nativeHandle;

    public native String getImagePath();
    public native byte[] getImageData();
    public native Rectangle getBounds();
    public native ImageRegion getImageRegion();
    public native void accept(Visitor visitor);
}

public class LayoutContainer {
    private long nativeHandle;

    public native Object[] getElements();
    public native Rectangle getBounds();
    public native void accept(Visitor visitor);
}
public class ImageRegion {
    public String id;
    public Rectangle bounds;
    public Rectangle sourceRect;
    public AlphaInfo alpha;
    public byte[] pixelData;
    public boolean hasTransparency;
    public Rectangle[] subregions;
}

public class AlphaInfo {
    public enum Type { SOLID, TRANSPARENT, VARIABLE, MASKED }
    public Type type;
    public float constantAlpha;
    public byte[] alphaMask;
    public Size maskSize;

    public static class Size {
        public int width;
        public int height;
    }
}
```

## Example Application

```java
package lowl.example;

import lowl.java.LOWL;
import lowl.java.desktop.*;
import lowl.java.dom.*;
import lowl.java.font.*;
import lowl.java.input.*;
import lowl.java.graphics.*;
import lowl.java.types.Box;

public class LOWLJavaDemo {

    static {
        LOWL.initialize();
    }

    public static void main(String[] args) {
        // Create a custom shape

        ShapeDefinition star = ShapeDefinition.createStar(400, 300, 5, 100, 200);
        // Create a visual container
        VisualContainer window =
LOWL.getDesktopManager().createShapeContainer();
        window.setShapeFromDefinition(star);
        window.setBackgroundColor(0.1f, 0.1f, 0.2f, 1.0f);
        window.setBackgroundBlur(10.0f);
        window.setBorder(2.0f, 0.2f, 0.6f, 0.9f, 1.0f);
        window.setBoxShadow(0, 8, 16, 0, 0, 0, 0, 0.3f);
        window.setOpacity(0.95f);

        // Create document
        Document doc = Document.create();
        doc.setTitle("LOWL Java Demo");

        Element root = doc.getDocumentElement();
        root.setStyle("display", "flex");
        root.setStyle("flexDirection", "column");
        root.setStyle("padding", "20px");

        // Add heading
        Element heading = doc.createElement("h1");
        heading.setInnerText("LOWL Platform - Java Integration");
        heading.setStyle("color", "#00d9ff");
        heading.setStyle("fontSize", "28px");
        heading.setStyle("textAlign", "center");
        root.appendChild(heading);

        // Add description
        Element description = doc.createElement("p");

        description.setInnerText("This application demonstrates Java integration with the LOWL platform, including DOM manipulation, custom shapes, animations, fonts, and input handling.");
        description.setStyle("color", "#a0a0b0");
        description.setStyle("fontSize", "14px");
        description.setStyle("lineHeight", "1.6");
        root.appendChild(description);

        // Add button
        Element button = doc.createElement("button");
        button.setInnerText("Click Me");
        button.setStyle("padding", "12px 24px");
        button.setStyle("backgroundColor", "#00d9ff");
        button.setStyle("color", "#1a1a2e");
        button.setStyle("border", "none");
        button.setStyle("borderRadius", "8px");
        button.setStyle("fontSize", "16px");
        button.setStyle("fontWeight", "600");
        button.setStyle("cursor", "pointer");
        button.setStyle("marginTop", "20px");
        button.addEventListener("click", event -> {

            LOWL.getDesktopManager().showNotification("Button Clicked", "Java event handler called!");
            System.out.println("Button clicked at " +
System.currentTimeMillis());
        });
        root.appendChild(button);

        window.setContentDocument(doc);

        // Add animation
        Animation fadeIn = Animation.create();
        fadeIn.setDuration(500);
        fadeIn.addKeyframe(0.0f, "opacity", new Box(0.0f));
        fadeIn.addKeyframe(1.0f, "opacity", new Box(0.95f));
        fadeIn.start();

        // Register keyboard shortcut

        Keyboard.getInstance().registerShortcut("test", "Ctrl+Shift+T", () -> {
            System.out.println("Shortcut triggered!");
            LOWL.getDesktopManager().showNotification("Shortcut",
"Ctrl+Shift+T pressed");
        });

        // Run desktop
        LOWL.getDesktopManager().addContainer(window);
        LOWL.getDesktopManager().run();
    }
}
```

## JNI Native Implementation (C++)

```cpp
// lowl_jni.cpp
#include <jni.h>
#include "lowl.hpp"
#include "lowl_jni.h"

using namespace lowl;

// Helper functions
Box JavaBoxToLowlBox(JNIEnv* env, jobject javaBox);
jobject LowlBoxToJavaBox(JNIEnv* env, const Box& box);

// Box conversion
JNIEXPORT void JNICALL Java_lowl_java_types_Box_createInt32(JNIEnv* env,
jobject obj, jint value) {
    Box* box = new Box(static_cast<int32_t>(value));
    setNativeHandle(env, obj, box);
}

JNIEXPORT jint JNICALL Java_lowl_java_types_Box_asInt32(JNIEnv* env, jobject obj) {
    Box* box = getNativeHandle<Box>(env, obj);
    return box->as_int32();
}

// Desktop Manager Implementation
JNIEXPORT jobject JNICALL

Java_lowl_java_desktop_DesktopManager_createShapeContainer(JNIEnv* env, jobject obj) {
    auto container = lowl::ui::ShapeContainer::create();
    return createJavaContainer(env, container);
}

JNIEXPORT void JNICALL

Java_lowl_java_desktop_VisualContainer_setShapeStar(JNIEnv* env, jobject obj, jint cx, jint cy, jint points, jint innerRadius, jint outerRadius) {    auto container = getNativeHandle<lowl::ui::ShapeContainer>(env, obj);

    std::vector<lowl::geometry::Point> starPoints;
    for (int i = 0; i < points * 2; i++) {
        double angle = i * M_PI / points;
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        int x = cx + radius * cos(angle);
        int y = cy + radius * sin(angle);
        starPoints.push_back({x, y});
    }

    container->set_shape(lowl::geometry::Polygon(starPoints));
}

JNIEXPORT void JNICALL

Java_lowl_java_desktop_VisualContainer_setBackgroundBlur(JNIEnv* env, jobject obj, jfloat radius) {

    auto container = getNativeHandle<lowl::ui::VisualContainer>(env, obj);    container->set_background_blur(radius);
}

JNIEXPORT void JNICALL

Java_lowl_java_desktop_VisualContainer_setBoxShadow(JNIEnv* env, jobject obj, jint offsetX, jint offsetY, jint blurRadius, jint spreadRadius, jfloat r, jfloat g, jfloat b, jfloat a) {

    auto container = getNativeHandle<lowl::ui::VisualContainer>(env, obj);    lowl::graphics::Shadow shadow;
    shadow.offset_x = offsetX;
    shadow.offset_y = offsetY;
    shadow.blur_radius = blurRadius;
    shadow.spread_radius = spreadRadius;
    shadow.color = lowl::graphics::Color(r, g, b, a);
    container->set_box_shadow(shadow);
}

JNIEXPORT void JNICALL

Java_lowl_java_desktop_VisualContainer_animateProperty(JNIEnv* env, jobject obj, jstring property, jobject fromBox, jobject toBox, jint durationMs) {    auto container = getNativeHandle<lowl::ui::VisualContainer>(env, obj);    const char* prop = env->GetStringUTFChars(property, nullptr);
    Box from = JavaBoxToLowlBox(env, fromBox);
    Box to = JavaBoxToLowlBox(env, toBox);

    container->animate_property(prop, from, to, durationMs);

    env->ReleaseStringUTFChars(property, prop);
}

// DOM Implementation

JNIEXPORT jobject JNICALL Java_lowl_java_dom_Document_createElement(JNIEnv* env, jobject obj, jstring tagName) {
    auto doc = getNativeHandle<lowl::dom::Document>(env, obj);
    const char* tag = env->GetStringUTFChars(tagName, nullptr);

    auto element = doc->create_element(tag);
    jobject javaElement = createJavaElement(env, element);

    env->ReleaseStringUTFChars(tagName, tag);
    return javaElement;
}

JNIEXPORT void JNICALL Java_lowl_java_dom_Element_setStyle(JNIEnv* env, jobject obj, jstring property, jstring value) {
    auto element = getNativeHandle<lowl::dom::Element>(env, obj);
    const char* prop = env->GetStringUTFChars(property, nullptr);
    const char* val = env->GetStringUTFChars(value, nullptr);

    element->set_style(prop, val);

    env->ReleaseStringUTFChars(property, prop);
    env->ReleaseStringUTFChars(value, val);
}

JNIEXPORT void JNICALL Java_lowl_java_dom_Element_addEventListener(JNIEnv* env, jobject obj, jstring type, jobject listener) {
    auto element = getNativeHandle<lowl::dom::Element>(env, obj);
    const char* eventType = env->GetStringUTFChars(type, nullptr);

    // Create global reference to Java listener
    jobject globalListener = env->NewGlobalRef(listener);

    element->add_event_listener(eventType, [env, globalListener](const lowl::dom::Event& event) {
        // Call Java listener
        jclass listenerClass = env->GetObjectClass(globalListener);

        jmethodID method = env->GetMethodID(listenerClass, "handleEvent", "(Llowl/java/dom/DOMEvent;)V");

        jobject javaEvent = createJavaEvent(env, event);
        env->CallVoidMethod(globalListener, method, javaEvent);
        env->DeleteLocalRef(javaEvent);
    });

    env->ReleaseStringUTFChars(type, eventType);
}

// Animation Implementation

JNIEXPORT jobject JNICALL Java_lowl_java_graphics_Animation_create(JNIEnv* env, jclass cls) {
    auto animation = lowl::animation::Animation::create();
    return createJavaAnimation(env, animation);
}

JNIEXPORT void JNICALL Java_lowl_java_graphics_Animation_addKeyframe(JNIEnv* env, jobject obj, jfloat time, jstring property, jobject valueBox) {

    auto animation = getNativeHandle<lowl::animation::Animation>(env, obj);    const char* prop = env->GetStringUTFChars(property, nullptr);
    Box value = JavaBoxToLowlBox(env, valueBox);

    animation->add_keyframe(time, prop, value);

    env->ReleaseStringUTFChars(property, prop);
}

JNIEXPORT void JNICALL Java_lowl_java_graphics_Animation_start(JNIEnv* env, jobject obj) {

    auto animation = getNativeHandle<lowl::animation::Animation>(env, obj);    animation->start();
}

JNIEXPORT void JNICALL Java_lowl_java_graphics_Animation_onComplete(JNIEnv* env, jobject obj, jobject runnable) {

    auto animation = getNativeHandle<lowl::animation::Animation>(env, obj);    jobject globalRunnable = env->NewGlobalRef(runnable);
    animation->on_complete([env, globalRunnable]() {
        jclass runnableClass = env->GetObjectClass(globalRunnable);

        jmethodID method = env->GetMethodID(runnableClass, "run", "()V");        env->CallVoidMethod(globalRunnable, method);
        env->DeleteGlobalRef(globalRunnable);
    });
}

// Font Implementation
JNIEXPORT jobject JNICALL

Java_lowl_java_font_FontDescriptor_loadFromFile(JNIEnv* env, jclass cls, jstring path) {
    const char* fontPath = env->GetStringUTFChars(path, nullptr);

    auto font = lowl::font::FontDescriptorRecord::load_from_file(fontPath);    env->ReleaseStringUTFChars(path, fontPath);
    return createJavaFont(env, font);
}

JNIEXPORT void JNICALL

Java_lowl_java_font_FontDescriptor_setKerningMode(JNIEnv* env, jobject obj, jobject modeObj) {

    auto font = getNativeHandle<lowl::font::FontDescriptorRecord>(env, obj);    jclass modeClass = env->GetObjectClass(modeObj);

    jfieldID ordinalField = env->GetFieldID(modeClass, "ordinal", "I");    jint ordinal = env->GetIntField(modeObj, ordinalField);

    font->set_kerning_mode(static_cast<lowl::font::KerningMode>(ordinal));}

JNIEXPORT void JNICALL

Java_lowl_java_font_FontDescriptor_addKerningPair(JNIEnv* env, jobject obj, jchar left, jchar right, jfloat adjustment) {

    auto font = getNativeHandle<lowl::font::FontDescriptorRecord>(env, obj);    font->add_kerning_pair(left, right, adjustment);
}

// Layout Manager Implementation

JNIEXPORT jobject JNICALL Java_lowl_java_font_LayoutManager_create(JNIEnv* env, jclass cls, jobject fontObj) {

    auto font = getNativeHandle<lowl::font::FontDescriptorRecord>(env, fontObj);
    auto layout = std::make_shared<lowl::font::LayoutManager>(font);
    return createJavaLayoutManager(env, layout);
}

JNIEXPORT void JNICALL

Java_lowl_java_font_LayoutManager_setShapeOval(JNIEnv* env, jobject obj, jint cx, jint cy, jint rx, jint ry) {

    auto layout = getNativeHandle<lowl::font::LayoutManager>(env, obj);    layout->set_shape_oval({static_cast<float>(cx), static_cast<float>(cy)},
                            static_cast<float>(rx), static_cast<float>(ry));}

JNIEXPORT void JNICALL

Java_lowl_java_font_LayoutManager_layoutTextInShape(JNIEnv* env, jobject obj, jstring text) {

    auto layout = getNativeHandle<lowl::font::LayoutManager>(env, obj);    const char* utf8Text = env->GetStringUTFChars(text, nullptr);
    layout->layout_text_in_shape(utf8Text);
    env->ReleaseStringUTFChars(text, utf8Text);
}

// Input Implementation
JNIEXPORT void JNICALL

Java_lowl_java_input_Keyboard_registerShortcut(JNIEnv* env, jobject obj, jstring name, jstring combination, jobject runnable) {

    const char* shortcutName = env->GetStringUTFChars(name, nullptr);    const char* shortcutCombo = env->GetStringUTFChars(combination,
nullptr);

    jobject globalRunnable = env->NewGlobalRef(runnable);

    lowl::input::Keyboard::get_instance().register_shortcut(
        shortcutName, shortcutCombo,
        [env, globalRunnable]() {

            jclass runnableClass = env->GetObjectClass(globalRunnable);            jmethodID method = env->GetMethodID(runnableClass, "run", "()V");
            env->CallVoidMethod(globalRunnable, method);
        }
    );

    env->ReleaseStringUTFChars(name, shortcutName);
    env->ReleaseStringUTFChars(combination, shortcutCombo);
}

JNIEXPORT jboolean JNICALL

Java_lowl_java_input_Keyboard_isKeyPressed(JNIEnv* env, jobject obj, jint keyCode) {

    return lowl::input::Keyboard::get_instance().is_key_pressed(keyCode);}

// Cleanup

JNIEXPORT void JNICALL Java_lowl_java_types_Box_close(JNIEnv* env, jobject obj) {
    delete getNativeHandle<Box>(env, obj);
    clearNativeHandle(env, obj);
}
JNIEXPORT void JNICALL Java_lowl_java_desktop_VisualContainer_close(JNIEnv* env, jobject obj) {

    auto container = getNativeHandle<lowl::ui::VisualContainer>(env, obj);    container->close();
    delete container;
    clearNativeHandle(env, obj);
}
```

## Build Configuration

```xml
<!-- pom.xml for Maven project -->
<project>
    <groupId>com.lowl</groupId>
    <artifactId>lowl-java</artifactId>
    <version>1.0.0</version>

    <dependencies>
        <dependency>
            <groupId>org.bytedeco</groupId>
            <artifactId>javacpp</artifactId>
            <version>1.5.9</version>
        </dependency>
    </dependencies>

    <build>
        <plugins>
            <plugin>
                <groupId>org.apache.maven.plugins</groupId>
                <artifactId>maven-compiler-plugin</artifactId>
                <version>3.11.0</version>
                <configuration>
                    <source>17</source>
                    <target>17</target>
                </configuration>
            </plugin>

            <plugin>
                <groupId>org.codehaus.mojo</groupId>
                <artifactId>native-maven-plugin</artifactId>
                <version>1.0-alpha-11</version>
                <executions>
                    <execution>
                        <id>javah</id>
                        <phase>generate-sources</phase>
                        <goals>
                            <goal>javah</goal>
                        </goals>
                    </execution>
                </executions>
            </plugin>
        </plugins>
    </build>
</project>
```

## CMake Build for JNI Library

```cmake
# CMakeLists.txt for LOWL JNI library
cmake_minimum_required(VERSION 3.20)
project(lowl_jni)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JNI
find_package(JNI REQUIRED)

# LOWL includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${JNI_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/lowl_jni.cpp
    src/box_conversion.cpp
    src/event_conversion.cpp
)

# Shared library
add_library(lowl_jni SHARED ${SOURCES})

target_link_libraries(lowl_jni
    ${JNI_LIBRARIES}
    lowl_core
    lowl_desktop
    lowl_dom
    lowl_font
    lowl_input
    lowl_graphics
    lowl_animation
)

set_target_properties(lowl_jni PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Install
install(TARGETS lowl_jni DESTINATION lib)
```

## Summary

The Java-LOWL integration layer provides:

```text
Component                  Java Package          Native Implementation        Key Features
Core Types                 lowl.java.types       Box conversion
```

Primitive types, arrays, maps,

```arithmetic
System
Process, memory, hardware,
```

```namespace
Desktop
Shapes, animations, VisualContainer

DOM
```

HTML parsing, CSS styling, event

```handling
Graphics
2D drawing, GPU acceleration, effects

Animation
Keyframes, easing, particle systems

Input
```

```gamepad
Font
```

### Kerning, shape LayoutManager

The system allows Java applications to:

- Create and manipulate LOWL desktop containers with custom shapes

- Build DOM documents with full CSS styling and event handling

- Render 2D graphics with GPU acceleration

- Create animations and particle effects

- Handle keyboard, mouse, touchpad, and gamepad input

- Use advanced font features including kerning and shape-aware layout

- Access system hardware and kernel objects

- Participate fully in the LOWL desktop environment

All objects are automatically converted between Java and LOWL types with minimal overhead. The JNI layer is optimized for performance, with direct memory access and reference counting to prevent leaks.
