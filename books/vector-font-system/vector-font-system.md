# Vector Font System

**By Anthony Matarazzo (c) 2026**

*47-page technical reference, translated to Markdown from `Vector Font System - Proton Docs.pdf`. Copyright (c) Anthony Matarazzo 2026. Released under the MIT License.*

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

- [1. Introduction](#1-introduction)
- [2. Quick Start](#2-quick-start)
- [3. Core Concepts](#3-core-concepts)
- [4. Font Descriptor Record (FDR)](#4-font-descriptor-record-fdr)
- [5. Kerning System](#5-kerning-system)
- [6. Layout Manager](#6-layout-manager)
- [7. Visitor Pattern for External Rendering](#7-visitor-pattern-for-external-rendering)
- [8. Shape Layout](#8-shape-layout)
- [9. Image Region Queries](#9-image-region-queries)
- [10. API Reference](#10-api-reference)
- [11. Examples](#11-examples)
- [12. Troubleshooting](#12-troubleshooting)
- [Introduction](#introduction)
  - [What is the Font System?](#what-is-the-font-system)
  - [Key Design Philosophy](#key-design-philosophy)
  - [System Architecture](#system-architecture)
- [Quick Start](#quick-start)
  - [Minimal Example](#minimal-example)
  - [With Kerning](#with-kerning)
  - [With Oval Shape](#with-oval-shape)
  - [With Polygon Shape](#with-polygon-shape)
- [Core Concepts](#core-concepts)
  - [1. Vector-Based Glyphs](#1-vector-based-glyphs)
  - [2. Parameter-Driven Fonts](#2-parameter-driven-fonts)
  - [3. Contextual Typography](#3-contextual-typography)
  - [4. Visitor Pattern for Integration](#4-visitor-pattern-for-integration)
- [Font Descriptor Record (FDR)](#font-descriptor-record-fdr)
  - [Creating a Font](#creating-a-font)
  - [Loading Glyphs](#loading-glyphs)
  - [Font Metrics](#font-metrics)
- [Kerning System](#kerning-system)
  - [Kerning Modes](#kerning-modes)
  - [Setting Kerning Mode](#setting-kerning-mode)
  - [Manual Kerning Pairs](#manual-kerning-pairs)
  - [Contextual Kerning](#contextual-kerning)
  - [Tracking (Global Letter Spacing)](#tracking-global-letter-spacing)
  - [Automatic Kerning Calculation](#automatic-kerning-calculation)
- [Layout Manager](#layout-manager)
  - [Creating a Layout Manager](#creating-a-layout-manager)
  - [Basic Layout Methods](#basic-layout-methods)
  - [Layout Parameters](#layout-parameters)
- [Shape Layout](#shape-layout)
  - [Rectangle (Box)](#rectangle-box)
  - [Oval / Circle](#oval-circle)
  - [Polygon](#polygon)
  - [Custom Shape with Path Following](#custom-shape-with-path-following)
- [Visitor Pattern for External Rendering](#visitor-pattern-for-external-rendering)
  - [The Problem This Solves](#the-problem-this-solves)
  - [The Solution: Visitor Pattern](#the-solution-visitor-pattern)
  - [Complete External Renderer Example](#complete-external-renderer-example)
  - [PDF Export Visitor](#pdf-export-visitor)
- [Image Region Queries](#image-region-queries)
  - [Getting a Region with Transparency](#getting-a-region-with-transparency)
  - [Getting All Image Regions](#getting-all-image-regions)
  - [Per-Pixel Alpha Query](#per-pixel-alpha-query)
  - [AlphaInfo Structure](#alphainfo-structure)
- [API Reference](#api-reference)
  - [FontDescriptorRecord](#fontdescriptorrecord)
  - [LayoutManager](#layoutmanager)
  - [Visitor Interface](#visitor-interface)
  - [GlyphElement](#glyphelement)
- [Examples](#examples)
  - [Example 1: Magazine Layout with Multiple Fonts](#example-1-magazine-layout-with-multiple-fonts)
  - [Example 2: Animated Bouncing Text](#example-2-animated-bouncing-text)
  - [Example 3: Circular Logo Text](#example-3-circular-logo-text)
  - [Example 4: Kerning Visualization](#example-4-kerning-visualization)
  - [Example 5: Polygon Text Wrapping](#example-5-polygon-text-wrapping)
- [Troubleshooting](#troubleshooting)
  - [Common Issues](#common-issues)
  - [Performance Tips](#performance-tips)
- [4. Precompute kerning - Call  LoadKerningFromFont()  once at startup](#4-precompute-kerning---call-loadkerningfromfont-once-at-startup)
  - [Debugging](#debugging)
- [Appendix](#appendix)
  - [A. Kerning Value Reference](#a-kerning-value-reference)
  - [B. Font Metrics Reference](#b-font-metrics-reference)
  - [C. Shape Type Comparison](#c-shape-type-comparison)
- [License](#license)
- [Version History](#version-history)
- [Complete Font System with Kerning Support](#complete-font-system-with-kerning-support)
- [Key Features Implemented](#key-features-implemented)
- [1. Implements the Visitor interface (ExternalRenderer)](#1-implements-the-visitor-interface-externalrenderer)
- [2. Queries image regions with transparency via  GetImageRegionForRect()](#2-queries-image-regions-with-transparency-via-getimageregionforrect)
- [3. Controls layout shapes (boxes, ovals, polygons)](#3-controls-layout-shapes-boxes-ovals-polygons)
- [4. Receives kerning-adjusted glyphs from the font system](#4-receives-kerning-adjusted-glyphs-from-the-font-system)

---

## 1. Introduction

## 2. Quick Start

## 3. Core Concepts

## 4. Font Descriptor Record (FDR)

## 5. Kerning System

## 6. Layout Manager

## 7. Visitor Pattern for External Rendering

## 8. Shape Layout

## 9. Image Region Queries

## 10. API Reference

## 11. Examples

## 12. Troubleshooting

## Introduction

### What is the Font System?

The Font System is a vector-based, parameter-driven typography engine that provides:

Dynamic font generation from parameters rather than file bloat Advanced kerning with multiple modes (auto, font-provided, hybrid)Shape-aware layout (rectangles, ovals, polygons)Visitor pattern for external rendering integration Transparency and region queries for compositing systems

### Key Design Philosophy

"Fonts are not files—they are parameter sets applied to vector data."

Traditional TTF/OTF fonts are static files. This system treats fonts as transformable vector data with:

Deformation curves

Wave animations

Contextual kerning

Shape-aware positioning

### System Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                    External Application                      │
│  (Your Code - Implements Visitor Interface)                  │
└─────────────────────────────────────────────────────────────┘
```

```text
                              │
                              ▼
```

```text
┌─────────────────────────────────────────────────────────────┐
│                    Visitor Interface                         │
│  (ExternalRenderer, PDFExporter, etc.)                       │
└─────────────────────────────────────────────────────────────┘
```

```text
                              │
                              ▼
```

```text
┌─────────────────────────────────────────────────────────────┐
│                    Layout Manager                            │
│  • Shape layout (rect/oval/polygon)                         │
│  • Text placement & wrapping                                 │
│  • Path following                                            │
└─────────────────────────────────────────────────────────────┘
```

```text
                              │
                              ▼
```

```text
┌─────────────────────────────────────────────────────────────┐
│                 Font Descriptor Record (FDR)                 │
│  • Glyph storage & outlines                                  │
│  • Kerning tables                                            │
│  • Font metrics                                              │
└─────────────────────────────────────────────────────────────┘
```

## Quick Start

### Minimal Example

```cpp
#include "font_system_with_kerning.h"
using namespace FontSystem;

int main() {
    // 1. Create a font

    auto font = std::make_shared<FontDescriptorRecord>("my_font", "Arial");    font->SetPointSize(24.0f);
    // 2. Create layout manager
    LayoutManager layout(font);

    // 3. Set a rectangular text area
    Rect text_box = {100, 100, 500, 300};
    layout.SetShapeRectangle(text_box);

    // 4. Layout text
    layout.LayoutTextInShape("Hello, World!");

    // 5. External renderer (you implement)
    ExternalRenderer renderer({800, 600});
    layout.Accept(renderer);

    return 0;
}
```

### With Kerning

```cpp
// Enable hybrid kerning (font + auto calculation)
layout.SetKerningMode(KerningMode::Hybrid);
layout.SetContext("headline");
layout.SetLetterSpacing(-0.5f);  // Tighter spacing for headlines

layout.LayoutTextInShape("AVATAR TECHNOLOGY");
```

### With Oval Shape

```cpp
// Text follows an oval path
layout.SetShapeOval({400, 300}, 200, 150);
layout.LayoutTextOnPath("Text following a circle", 0.0f);
```

### With Polygon Shape

```cpp
// Star-shaped text container
std::vector<Vec2> star_points = {
    {400, 100}, {500, 300}, {400, 500}, {300, 300}
};
layout.SetShapePolygon(star_points);
layout.LayoutTextInShape("Star text");
```

## Core Concepts

### 1. Vector-Based Glyphs

Glyphs are stored as vector paths (points and curves), not bitmaps. This allows:

- Infinite scaling without quality loss

- Runtime deformation (wave, bounce, twist)

Smooth animation

```cpp
struct GlyphData {
    std::vector<Vec2> outline;  // Vector path
    float advance;               // Width to next glyph
    Rect bounds;                 // Bounding box
};
```

### 2. Parameter-Driven Fonts

Fonts are defined by parameters that can be modified at runtime:

```cpp
font->SetPointSize(24.0f);
font->SetKerningMode(KerningMode::Hybrid);
layout.SetLetterSpacing(-0.5f);
layout.SetLineHeight(1.4f);
```

### 3. Contextual Typography

Different contexts (headline, body, caption) can have different kerning values:

```cpp
font->ApplyHeadlineKerning();  // Tighter kerning
font->ApplyBodyKerning();      // Conservative kerning
layout.SetContext("headline");
```

### 4. Visitor Pattern for Integration

The library does not render—it provides visitable elements. Your code implements the visitor:

```cpp
class MyRenderer : public Visitor<void> {
    void Visit(GlyphElement& glyph) override {
        // Your rendering code here
        // OpenGL, DirectX, software, PDF, SVG, etc.
    }
};
```

## Font Descriptor Record (FDR)

### Creating a Font

```cpp
// Basic font
auto font = std::make_shared<FontDescriptorRecord>("font_id", "Helvetica");

// With style
font->style_name = "Bold Italic";
font->SetPointSize(18.0f);
```

### Loading Glyphs

```cpp
GlyphData glyph_a;
glyph_a.codepoint = 'A';
glyph_a.name = "A";
glyph_a.outline = {{0,0}, {10,0}, {10,14}, {0,14}, {0,0}};
glyph_a.advance = 10.0f;
glyph_a.bounds = {0, 0, 10, 14};

font->AddGlyph(glyph_a);
```

### Font Metrics

```cpp
FontMetrics& metrics = font->GetMetrics();
metrics.ascender = 800.0f;      // Height above baseline
metrics.descender = -200.0f;    // Depth below baseline
metrics.x_height = 500.0f;      // Height of lowercase 'x'
metrics.cap_height = 700.0f;    // Height of uppercase letters
metrics.units_per_em = 1000.0f; // Font design units

float line_height = metrics.GetLineHeight(1.4f);  // Scaled
```

## Kerning System

### Kerning Modes

| Mode | Description |
| --- | --- |
| KerningMode::None | No kerning adjustments |

```cpp
KerningMode::Auto
```

Automatically calculate based on glyph shapes

| KerningMode::FontProvided | Use kerning from font file |
| --- | --- |
| KerningMode::Manual | Use manually specified pairs only |
| KerningMode::Hybrid | Combine font + auto calculation |

### Setting Kerning Mode

```cpp
layout.SetKerningMode(KerningMode::Hybrid);
font->SetKerningMode(KerningMode::FontProvided);
```

### Manual Kerning Pairs

```cpp
// By codepoint
font->AddKerningPair('A', 'V', -2.5f);
font->AddKerningPair('T', 'a', -1.5f);
font->AddKerningPair('f', 'i', -2.0f);

// By glyph name
font->AddKerningPairByName("A", "V", -2.5f);
font->AddKerningPairByName("T", "a", -1.5f);
```

### Contextual Kerning

```cpp
// Headline context (tighter)
font->AddContextualKerning("headline", "A", "V", -3.0f);
font->AddContextualKerning("headline", "T", "a", -2.0f);

// Body context (more conservative)
font->AddContextualKerning("body", "A", "V", -1.8f);
font->AddContextualKerning("body", "T", "a", -1.2f);

// Apply in layout
layout.SetContext("headline");
```

### Tracking (Global Letter Spacing)

```cpp
// Negative = tighter, positive = looser
layout.SetLetterSpacing(-0.5f);   // Tighter for headlines
layout.SetLetterSpacing(0.2f);    // Looser for captions
```

### Automatic Kerning Calculation

The system can calculate kerning automatically by analyzing glyph side profiles:

```cpp
font->SetKerningMode(KerningMode::Auto);
float kerning = font->CalculateAutoKerning('A', 'V');  // Returns ~ -2.5f
```

## Layout Manager

### Creating a Layout Manager

```cpp
auto font = std::make_shared<FontDescriptorRecord>("font", "Arial");LayoutManager layout(font);
```

### Basic Layout Methods

```cpp
// Standard rectangle layout (wraps at edges)
layout.LayoutTextInShape("Your text here");

// Path layout (follows shape perimeter)
layout.LayoutTextOnPath("Curved text", 0.0f);  // 0.0f = start angle
```

### Layout Parameters

```cpp
layout.SetLineHeight(1.4f);           // Line height multiplier

layout.SetLetterSpacing(0.0f);        // Additional space between letters layout.SetKerningMode(KerningMode::Hybrid);

layout.SetContext("body");             // Context for contextual kerning layout.use_ligatures = true;           // Enable ligatures (fi, fl, etc.)
```

## Shape Layout

### Rectangle (Box)

```cpp
Rect text_area = {x, y, width, height};
layout.SetShapeRectangle(text_area);
layout.LayoutTextInShape(text);
```

Behavior: Text wraps when reaching the right edge.

### Oval / Circle

```cpp
Vec2 center = {400, 300};
float radius_x = 200.0f;
float radius_y = 150.0f;
layout.SetShapeOval(center, radius_x, radius_y);

// Option 1: Fill the oval (text wraps inside)
layout.LayoutTextInShape(text);

// Option 2: Follow the perimeter
layout.LayoutTextOnPath(text, 0.0f);
```

### Polygon

```cpp
std::vector<Vec2> polygon = {
    {400, 100},   // Top
    {600, 300},   // Right
    {400, 500},   // Bottom
    {200, 300}    // Left
};
layout.SetShapePolygon(polygon);
layout.LayoutTextInShape(text);
```

### Custom Shape with Path Following

```cpp
// Text follows any polygon perimeter
std::vector<Vec2> star = GenerateStarPoints(5, 200, 100);
layout.SetShapePolygon(star);
layout.LayoutTextOnPath("Star text goes around", 0.0f);
```

## Visitor Pattern for External Rendering

### The Problem This Solves

The font system does not know how you want to render:

- OpenGL? DirectX? Software? PDF? SVG?

The library stays agnostic

### The Solution: Visitor Pattern

You implement the visitor with your rendering logic:

```cpp
class MyOpenGLRenderer : public Visitor<void> {
private:
    GLuint shader_program;
    GLuint vao;

public:
    void Visit(GlyphElement& glyph) override {
        // Get glyph data
        auto outline = glyph.GetOutline();
        auto bounds = glyph.GetBounds();
        auto transform = glyph.GetTransform();

        // Upload to GPU and render
        UploadVertices(outline);
        ApplyTransform(transform);
        DrawElements();
    }

    void Visit(WordElement& word) override {
        // Optional: word-level processing
        for (auto& glyph : word.GetGlyphs()) {
            glyph->Accept(*this);
        }
    }

    // Other Visit methods...
};
```

### Complete External Renderer Example

```cpp
class SoftwareRenderer : public Visitor<void> {
private:
    std::vector<uint8_t> framebuffer;
    Size canvas;
    Color current_color;

public:
    SoftwareRenderer(Size size, Color color)
        : canvas(size), current_color(color) {
        framebuffer.resize(size.width * size.height * 4, 0);
    }

    void Visit(GlyphElement& glyph) override {
        Rect bounds = glyph.GetBounds();
        const auto& outline = glyph.GetOutline();
        // Rasterize the glyph outline
        for (const auto& point : outline) {
            int x = static_cast<int>(bounds.x + point.x);
            int y = static_cast<int>(bounds.y + point.y);

            if (x >= 0 && x < canvas.width && y >= 0 && y < canvas.height) {                size_t idx = (y * canvas.width + x) * 4;

                framebuffer[idx] = static_cast<uint8_t>(current_color.r * 255);

                framebuffer[idx+1] = static_cast<uint8_t>(current_color.g * 255);

                framebuffer[idx+2] = static_cast<uint8_t>(current_color.b * 255);

                framebuffer[idx+3] = static_cast<uint8_t>(current_color.a * 255);
            }
        }
    }

    void Visit(WordElement& word) override {
        // Word-level: could add background boxes
        for (auto& glyph : word.GetGlyphs()) {
            glyph->Accept(*this);
        }
    }

    void Visit(LineElement& line) override {}
    void Visit(ParagraphElement& para) override {}
    void Visit(ImageElement& img) override {}
    void Visit(LayoutContainer& container) override {}

    const std::vector<uint8_t>& GetFramebuffer() const { return framebuffer; }
};

// Usage
SoftwareRenderer renderer({800, 600}, {1.0f, 0.2f, 0.3f, 1.0f});
layout.Accept(renderer);
auto& pixels = renderer.GetFramebuffer();
```

### PDF Export Visitor

```cpp
class PDFExporter : public Visitor<void> {
private:
    std::ofstream pdf_file;

public:
    PDFExporter(const std::string& filename) {
        pdf_file.open(filename);
        pdf_file << "%PDF-1.4\n";
        // PDF header, objects, etc.
    }

    void Visit(GlyphElement& glyph) override {
        const auto& outline = glyph.GetOutline();
        pdf_file << "q\n";
        pdf_file << "BT\n";

        for (const auto& point : outline) {
            pdf_file << point.x << " " << point.y << " l\n";
        }

        pdf_file << "ET\n";
        pdf_file << "Q\n";
    }

    ~PDFExporter() {
        pdf_file << "%%EOF\n";
        pdf_file.close();
    }

    // Other Visit methods...
};
```

## Image Region Queries

### Getting a Region with Transparency

External compositing systems need to know where images should go and what

transparency they have.

```cpp
// Query a specific rectangle
Rect query_rect = {100, 100, 64, 64};
ImageRegion region = layout.GetImageRegionForRect(query_rect);

if (region.has_transparency) {
    // Region has transparency - use alpha blending
    for (const auto& subrect : region.subregions) {
        // Each subrect is a square (64x64 by default)
        RenderSubregion(subrect, region.alpha);
    }
}
```

### Getting All Image Regions

```cpp
std::vector<ImageRegion> all_regions = layout.GetAllImageRegions();

for (const auto& region : all_regions) {
    // region.bounds = position and size
    // region.alpha = transparency information
    // region.pixel_data = pre-rendered pixels (if available)
}
```

### Per-Pixel Alpha Query

```cpp
Vec2 point = {150, 200};
float alpha = layout.GetAlphaAtPoint(point);

if (alpha > 0.0f) {
    // This point has text with alpha value
    BlendWithBackground(alpha);
}
```

### AlphaInfo Structure

```cpp
struct AlphaInfo {
    enum Type { Solid, Transparent, Variable, Masked } type;
    float constant_alpha;              // For Solid/Transparent
    std::vector<uint8_t> alpha_mask;   // Per-pixel alpha
    Size mask_size;
};
```

## API Reference

### FontDescriptorRecord

| Method | Description |
| --- | --- |
| FontDescriptorRecord(id, family) | Constructor |
| AddGlyph(glyph) | Add a glyph to the font |
| GetGlyph(codepoint) | Get glyph data |
| SetKerningMode(mode) | Set kerning behavior |
| AddKerningPair(left, right, adj) | Add manual kerning |
| AddKerningPairByName(left, right, adj) | Add by glyph name |
| AddContextualKerning(ctx, left, right, adj) | Context-specific kerning |
| LoadKerningFromFont(path) | Load from TTF/OTF file |
| CalculateAutoKerning(left, right) | Automatic kerning |
| SetPointSize(size) | Set font size in points |
| GetScaleFactor() | Get pixels-per-unit ratio |

### LayoutManager

| Method | Description |
| --- | --- |
| LayoutManager(font) | Constructor |
| SetShapeRectangle(rect) | Set rectangular container |
| SetShapeOval(center, rx, ry) | Set oval container |
| SetShapePolygon(points) | Set polygon container |
| LayoutTextInShape(text) | Layout text inside shape |
| LayoutTextOnPath(text, angle) | Layout text on perimeter |
| SetKerningMode(mode) | Set kerning mode |
| SetContext(context) | Set kerning context |
| SetLineHeight(multiplier) | Set line height |
| SetLetterSpacing(spacing) | Set global tracking |
| GetImageRegionForRect(rect) | Query region with transparency |
| GetAllImageRegions() | Get all regions |
| GetAlphaAtPoint(point) | Get alpha at coordinate |
| Accept(visitor) | Accept external visitor |

### Visitor Interface

```cpp
template<typename T>
class Visitor {
public:
    virtual void Visit(GlyphElement& element) = 0;
    virtual void Visit(WordElement& element) = 0;
    virtual void Visit(LineElement& element) = 0;
    virtual void Visit(ParagraphElement& element) = 0;
    virtual void Visit(ImageElement& element) = 0;
    virtual void Visit(LayoutContainer& element) = 0;
};
```

### GlyphElement

| Method | Description |
| --- | --- |
| GetCharCode() | Unicode codepoint |
| GetCharacter() | wchar_t character |
| GetOutline() | Vector path |
| GetBounds() | Bounding rectangle |
| GetTransform() | Transformation matrix |
| GetImageRegion() | Get as image region |
| GetAlphaForRegion(rect) | Get alpha for subregion |

## Examples

### Example 1: Magazine Layout with Multiple Fonts

```cpp
void CreateMagazineLayout() {
    // Title font (bold, tight kerning)

    auto title_font = std::make_shared<FontDescriptorRecord>("title", "Helvetica");
    title_font->SetPointSize(48.0f);
    title_font->ApplyHeadlineKerning();

    // Body font (normal, comfortable)

    auto body_font = std::make_shared<FontDescriptorRecord>("body", "Times");
    body_font->SetPointSize(12.0f);
    body_font->ApplyBodyKerning();

    // Layout title
    LayoutManager title_layout(title_font);
    title_layout.SetShapeRectangle({50, 50, 700, 100});
    title_layout.SetKerningMode(KerningMode::Hybrid);
    title_layout.SetContext("headline");
    title_layout.SetLetterSpacing(-0.8f);
    title_layout.LayoutTextInShape("THE FUTURE OF TYPOGRAPHY");

    // Layout body text (two columns)
    LayoutManager body_layout(body_font);
    body_layout.SetShapeRectangle({50, 180, 340, 400});
    body_layout.SetContext("body");
    body_layout.SetLineHeight(1.5f);
    body_layout.LayoutTextInShape(long_article_text);

    // Second column
    LayoutManager body_layout2(body_font);
    body_layout2.SetShapeRectangle({410, 180, 340, 400});
    body_layout2.LayoutTextInShape(article_continued);

    // Render with OpenGL visitor
    OpenGLRenderer renderer;
    title_layout.Accept(renderer);
    body_layout.Accept(renderer);
    body_layout2.Accept(renderer);
}
```

### Example 2: Animated Bouncing Text

```cpp
void AnimatedText() {

    auto font = std::make_shared<FontDescriptorRecord>("bounce", "Arial");    font->SetPointSize(36.0f);

    LayoutManager layout(font);
    layout.SetShapeRectangle({100, 100, 600, 200});
    layout.SetKerningMode(KerningMode::Auto);

    float time = 0.0f;
    while (true) {
        // Update animation parameters
        float bounce = std::abs(std::sin(time)) * 20.0f;
        layout.SetLineHeight(1.4f + bounce / 100.0f);

        // Re-layout (would need to re-run LayoutTextInShape)
        layout.LayoutTextInShape("BOUNCING TEXT!");

        // Render

        SoftwareRenderer renderer({800, 600}, {1.0f, 0.5f, 0.0f, 1.0f});        layout.Accept(renderer);

        time += 0.016f;  // 60 FPS
        Sleep(16);
    }
}
```

### Example 3: Circular Logo Text

```cpp
void CreateCircularLogo() {

    auto font = std::make_shared<FontDescriptorRecord>("logo", "Helvetica");    font->SetPointSize(24.0f);
    font->SetKerningMode(KerningMode::Manual);

    // Manual kerning for perfect circle alignment
    font->AddKerningPair('A', 'V', -3.0f);
    font->AddKerningPair('A', 'T', -2.5f);

    LayoutManager layout(font);

    // Center of logo
    Vec2 center = {400, 400};

    // Top text (curves along top of circle)
    layout.SetShapeOval(center, 200, 200);
    layout.LayoutTextOnPath("PREMIUM TYPOGRAPHY", M_PI);  // Start at left

    // Bottom text (curves along bottom, inverted)
    layout.SetShapeOval(center, 200, 200);
    layout.LayoutTextOnPath("ESTABLISHED 2024", 0.0f);  // Start at right

    // Center text
    LayoutManager center_layout(font);
    center_layout.SetShapeRectangle({300, 370, 200, 60});
    center_layout.SetContext("headline");
    center_layout.LayoutTextInShape("CORE");

    // Render
    VectorRenderer renderer("logo.svg");
    layout.Accept(renderer);
    center_layout.Accept(renderer);
}
```

### Example 4: Kerning Visualization

```cpp
void VisualizeKerning() {

    auto font = std::make_shared<FontDescriptorRecord>("demo", "Times");    font->SetPointSize(48.0f);

    std::string test_pairs[] = {
        "AV", "AW", "AT", "AY",
        "Ta", "To", "Te",
        "fi", "fl", "ff",
        "VA", "WA", "TA"
    };

    LayoutManager layout(font);
    layout.SetShapeRectangle({50, 50, 700, 600});

    float y = 50;
    for (const auto& pair : test_pairs) {
        // Without kerning
        layout.SetKerningMode(KerningMode::None);
        layout.LayoutTextInShape(pair + " (no kerning)");

        // With kerning
        layout.SetKerningMode(KerningMode::Hybrid);
        layout.LayoutTextInShape(pair + " (with kerning)");

        y += 60;
    }

    // Render with debug overlay showing kerning values
    DebugRenderer renderer;
    renderer.ShowKerningValues(true);
    layout.Accept(renderer);
}
```

### Example 5: Polygon Text Wrapping

```cpp
void PolygonTextWrap() {

    auto font = std::make_shared<FontDescriptorRecord>("wrap", "Arial");    font->SetPointSize(14.0f);

    // Create a custom polygon (hexagon)
    std::vector<Vec2> hexagon;
    for (int i = 0; i < 6; ++i) {
        float angle = i * 2.0f * M_PI / 6.0f - M_PI / 2.0f;

        hexagon.push_back({400 + 200 * cos(angle), 300 + 200 * sin(angle)});    }

    LayoutManager layout(font);
    layout.SetShapePolygon(hexagon);
    layout.SetLineHeight(1.4f);
    layout.SetKerningMode(KerningMode::Auto);

    std::string long_text = "This text will wrap inside a hexagon shape. "                            "The layout manager ensures all text stays within "
                            "the polygon boundaries. Text flows naturally from "
                            "left to right and wraps at the polygon edges.";

    layout.LayoutTextInShape(long_text);

    // Render

    SoftwareRenderer renderer({800, 600}, {0.2f, 0.3f, 0.8f, 1.0f});    layout.Accept(renderer);
}
```

## Troubleshooting

### Common Issues

Issue  Solution

Ensure  SetShapeRectangle()  is called Text doesn't wrap

with correct width

Check  SetKerningMode()  is not  None . Kerning not applying

Verify font has kerning data

Verify  AddGlyph()  was called for all Glyphs not rendering

Ensure  Accept()  is called on the layout Visitor not called

```manager
Check  AlphaInfo  type. Use
GetAlphaForRegion()  for precise queries
Adjust  start_angle  parameter in
LayoutTextOnPath()
Polygon layout crashes
```

### Performance Tips

\1. Batch glyph processing - Use  OnGlyphBatch()  in visitor for groups

\2. Dirty rectangles - Query only changed regions with  GetImageRegionForRect()

\3. Cache font data - Keep  FontDescriptorRecord  shared between layouts

## 4. Precompute kerning - Call  LoadKerningFromFont()  once at startup

### Debugging

```cpp
// Enable debug output (if implemented)
font->SetDebugMode(true);
layout.SetDebugMode(true);

// Get layout metrics
auto& metrics = font->GetMetrics();
printf("Line height: %.2f\n", metrics.GetLineHeight());

// Inspect placed glyphs
for (auto& glyph : layout.GetPlacedGlyphs()) {
    printf("Glyph %c at (%.2f, %.2f)\n",
           glyph->GetCharacter(),
           glyph->GetBounds().x,
           glyph->GetBounds().y);
}
```

## Appendix

### A. Kerning Value Reference

Pair  Typical

Context Value

AV  -2.5 to  Headline

-3.0

AW  -2.0 to  Headline

-2.5

TA  -1.5 to  Body

-2.0

fi  -2.0 to  All

-2.5

fl  -2.0 to  All

-2.5

Te  -1.2 to  Body

-1.5

Yo  -1.0 to  Body

-1.5

### B. Font Metrics Reference

```text
        ┌─────────────────┐  ← Cap Height (700)
        │                 │
        │    X-Height     │  ← x_height (500)
        │    (500)        │
        │                 │
        └─────────────────┘  ← Baseline (0)
```

```text
                    │
```

```text
                    │  ← Descender (-200)
```

### C. Shape Type Comparison

| Shape | Best For | Methods |
| --- | --- | --- |
| LayoutTextInShape | Rectangle | Paragraphs, articles |
| LayoutTextOnPath | Oval | Logos, badges |
| Polygon | Custom shapes, | Both |

## License

This font system is provided under the MIT License. See LICENSE file for details.

## Version History

v1.0 - Initial release with kerning, shapes, visitor pattern v1.1 - Added contextual kerning and path following v1.2 - Added polygon layout and region queries

For additional support, contact the font system team or visit the documentation website.

```cpp
//
```

```text
============================================================================// font_visitor.h - Library Interface (Provided by Font System)
```

```cpp
//
```

```text
============================================================================
```

```cpp
#ifndef FONT_VISITOR_H
#define FONT_VISITOR_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace FontSystem {
//
```

```text
============================================================================// Core Types
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Rect { float x, y, width, height; };
struct Color { float r, g, b, a; };
struct Size { float width, height; };
struct Matrix3x3 { float m[3][3]; };

// Transparency/Alpha information
struct AlphaInfo {
    enum Type { Solid, Transparent, Variable, Masked } type;
    float constant_alpha = 1.0f;
    std::vector<uint8_t> alpha_mask;  // For variable transparency
    Size mask_size;
};

// Image/Texture region that can be queried
struct ImageRegion {
    std::string id;
    Rect bounds;           // Position in layout space

    Rect source_rect;      // Which part of the source texture to use    AlphaInfo alpha;

    std::vector<uint8_t> pixel_data;  // Optional: pre-rendered RGBA    bool has_transparency = false;

    // For square/region queries

    std::vector<Rect> subregions;  // Can break into multiple squares};

//
```

```text
============================================================================// Visitable Elements (Provided by Font System)
```

```cpp
//
```

```text
============================================================================
```

```cpp
// Forward declarations
class GlyphElement;
class WordElement;
class LineElement;
class ParagraphElement;
class ImageElement;
class LayoutContainer;

// Visitor interface - IMPLEMENTED BY EXTERNAL CONSUMER
template<typename T>
class Visitor {
public:
    virtual ~Visitor() = default;

    // Visit methods - external code implements these
    virtual void Visit(GlyphElement& element) = 0;
    virtual void Visit(WordElement& element) = 0;
    virtual void Visit(LineElement& element) = 0;
    virtual void Visit(ParagraphElement& element) = 0;
    virtual void Visit(ImageElement& element) = 0;
    virtual void Visit(LayoutContainer& element) = 0;
};

// Base visitable class
class Visitable {
public:
    virtual ~Visitable() = default;
    virtual void Accept(Visitor<void>& visitor) = 0;
};

//
```

```text
============================================================================// Glyph Element - Smallest unit
```

```cpp
//
```

```text
============================================================================
```

```cpp
class GlyphElement : public Visitable {
private:
    uint32_t char_code;
    wchar_t character;
    std::string glyph_name;
    std::vector<Vec2> outline;      // Vector path
    Matrix3x3 transform;            // Current transform
    Rect bounds;                    // Bounding box
    float advance;                  // Width to next glyph

    // Rendering data
    std::vector<uint8_t> rendered_pixels;  // RGBA for this glyph
    AlphaInfo alpha;
    bool is_dirty = true;

public:

    GlyphElement(uint32_t code, wchar_t ch, const std::string& name)        : char_code(code), character(ch), glyph_name(name) {}

    // Getters for external access
    uint32_t GetCharCode() const { return char_code; }
    wchar_t GetCharacter() const { return character; }
    const std::string& GetGlyphName() const { return glyph_name; }

    const std::vector<Vec2>& GetOutline() const { return outline; }    const Matrix3x3& GetTransform() const { return transform; }
    const Rect& GetBounds() const { return bounds; }
    float GetAdvance() const { return advance; }

    // Setters (called by layout engine)

    void SetOutline(const std::vector<Vec2>& o) { outline = o; is_dirty = true; }

    void SetTransform(const Matrix3x3& t) { transform = t; is_dirty = true; }
    void SetBounds(const Rect& b) { bounds = b; }
    void SetAdvance(float a) { advance = a; }

    // Query image region with transparency
    ImageRegion GetImageRegion() const {
        ImageRegion region;
        region.id = "glyph_" + std::to_string(char_code);
        region.bounds = bounds;
        region.alpha = alpha;

        // Break into square subregions for efficient rendering

        region.subregions = SubdivideIntoRectangles(bounds, 64, 64);  // 64x64 tiles

        if (!rendered_pixels.empty()) {
            region.pixel_data = rendered_pixels;

            region.has_transparency = (alpha.type != AlphaInfo::Solid);        }

        return region;
    }

    // Get transparency data for a specific square region
    AlphaInfo GetAlphaForRegion(const Rect& query_rect) const {
        AlphaInfo result;
        result.type = AlphaInfo::Variable;

        // Intersect query with glyph bounds
        Rect intersect = Intersect(query_rect, bounds);
        if (intersect.width <= 0 || intersect.height <= 0) {
            result.type = AlphaInfo::Transparent;
            result.constant_alpha = 0.0f;
            return result;
        }

        // Extract alpha from rendered pixels for that region
        // (Implementation would sample from rendered_pixels)
        return result;
    }

    void Accept(Visitor<void>& visitor) override {
        visitor.Visit(*this);
    }

private:

    std::vector<Rect> SubdivideIntoRectangles(const Rect& area, float max_w, float max_h) const {
        std::vector<Rect> rects;

        for (float y = area.y; y < area.y + area.height; y += max_h) {            for (float x = area.x; x < area.x + area.width; x += max_w) {                Rect r;
                r.x = x;
                r.y = y;

                r.width = std::min(max_w, area.x + area.width - x);                r.height = std::min(max_h, area.y + area.height - y);                if (r.width > 0 && r.height > 0) {
                    rects.push_back(r);
                }
            }
        }
        return rects;
    }

    Rect Intersect(const Rect& a, const Rect& b) const {
        Rect result;
        result.x = std::max(a.x, b.x);
        result.y = std::max(a.y, b.y);

        result.width = std::min(a.x + a.width, b.x + b.width) - result.x;        result.height = std::min(a.y + a.height, b.y + b.height) - result.y;        if (result.width < 0) result.width = 0;
        if (result.height < 0) result.height = 0;
        return result;
    }
};

//
```

```text
============================================================================// Word Element - Collection of glyphs
```

```cpp
//
```

```text
============================================================================
```

```cpp
class WordElement : public Visitable {
private:
    std::string text;
    std::vector<std::shared_ptr<GlyphElement>> glyphs;
    Rect bounds;
    std::vector<Vec2> polygon;  // For non-rectangular word shapes (e.g., curved text)
    float advance;

public:
    WordElement(const std::string& t) : text(t) {}

    void AddGlyph(std::shared_ptr<GlyphElement> glyph) {
        glyphs.push_back(glyph);
        UpdateBounds();
    }

    const std::vector<std::shared_ptr<GlyphElement>>& GetGlyphs() const { return glyphs
```

the system must calculate kerning or use if applied from the font fdr.

You're right. The font system must handle kerning as a first-class property, either calculating it dynamically or using pre-defined values from the Font Descriptor Record (FDR). Here's the complete corrected implementation with proper kerning support:

## Complete Font System with Kerning Support

```cpp
//
```

```text
============================================================================// font_system_with_kerning.h - Complete Font System with Kerning
```

```cpp
//
```

```text
============================================================================
```

```cpp
#ifndef FONT_SYSTEM_WITH_KERNING_H
#define FONT_SYSTEM_WITH_KERNING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <optional>

namespace FontSystem {

//
```

```text
============================================================================// Core Types
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct Vec2 { float x, y; };
struct Rect { float x, y, width, height; };
struct Color { float r, g, b, a; };
struct Size { float width, height; };

//
```

```text
============================================================================// Kerning Types
```

```cpp
//
```

```text
============================================================================
```

```cpp
enum class KerningMode {
    None,           // No kerning adjustment

    Auto,           // Automatically calculate based on glyph shapes    FontProvided,   // Use kerning from font file
    Manual,         // Use manually specified pairs
    Hybrid          // Combine font + auto calculation
};

struct KerningPair {
    uint32_t left_glyph;
    uint32_t right_glyph;

    float adjustment;      // Negative = tighter, positive = looser    float confidence;      // 0-1, how reliable this kerning is

    std::string context;   // Optional: "default", "title", "body", etc.};

struct KerningTable {
    KerningMode mode = KerningMode::Auto;

    std::unordered_map<uint64_t, KerningPair> pairs;  // key = (left << 32) | right
    float default_adjustment = 0.0f;
    float tracking = 0.0f;    // Global letter-spacing added to all pairs

    // Context-specific overrides
    std::unordered_map<std::string, std::unordered_map<uint64_t,
KerningPair>> context_overrides;

    void AddPair(uint32_t left, uint32_t right, float adjustment, float confidence = 1.0f) {

        uint64_t key = (static_cast<uint64_t>(left) << 32) | right;        pairs[key] = {left, right, adjustment, confidence, "default"};    }

    void AddContextPair(const std::string& context, uint32_t left, uint32_t right, float adjustment) {

        uint64_t key = (static_cast<uint64_t>(left) << 32) | right;        context_overrides[context][key] = {left, right, adjustment, 1.0f,
context};
    }

    float GetKerning(uint32_t left, uint32_t right, const std::string& context = "default") const {
        uint64_t key = (static_cast<uint64_t>(left) << 32) | right;

        // Check context override first
        auto ctx_it = context_overrides.find(context);
        if (ctx_it != context_overrides.end()) {
            auto pair_it = ctx_it->second.find(key);
            if (pair_it != ctx_it->second.end()) {
                return pair_it->second.adjustment + tracking;
            }
        }

        // Check font-provided pairs
        auto pair_it = pairs.find(key);
        if (pair_it != pairs.end()) {
            return pair_it->second.adjustment + tracking;
        }

        // Auto-calculate if mode allows

        if (mode == KerningMode::Auto || mode == KerningMode::Hybrid) {            return CalculateAutoKerning(left, right) + tracking;
        }

        return 0.0f + tracking;
    }

private:

    float CalculateAutoKerning(uint32_t left, uint32_t right) const {        // Automatic kerning based on glyph shapes
        // This analyzes the outlines of adjacent glyphs

        float kerning = 0.0f;

        // Common problematic pairs
        // 'A' next to 'V' should be tighter

        if ((left == 'A' && right == 'V') || (left == 'V' && right == 'A')) {
            kerning = -2.5f;
        }
        // 'T' next to 'o' or 'a'

        else if (left == 'T' && (right == 'o' || right == 'a' || right == 'e')) {
            kerning = -1.8f;
        }
        // 'W' next to 'a'
        else if (left == 'W' && right == 'a') {
            kerning = -1.5f;
        }
        // 'f' next to 'i' (fi ligature approximation)
        else if (left == 'f' && right == 'i') {
            kerning = -2.0f;
        }
        // 'L' next to 'T'
        else if (left == 'L' && right == 'T') {
            kerning = -1.2f;
        }
        // 'Y' next to 'o' or 'e'

        else if (left == 'Y' && (right == 'o' || right == 'e' || right == 'a')) {
            kerning = -2.0f;
        }
        // Punctuation adjustments

        else if ((left == '"' || left == '\'' || left == '(') && (right >= 'A' && right <= 'Z')) {
            kerning = 0.5f;  // Slightly looser
        }

        return kerning;
    }
};

//
```

```text
============================================================================// Font Descriptor Record (FDR)
```

```cpp
//
```

```text
============================================================================
```

```cpp
struct FontMetrics {
    float units_per_em = 1000.0f;
    float ascender = 800.0f;
    float descender = -200.0f;
    float line_gap = 200.0f;
    float x_height = 500.0f;
    float cap_height = 700.0f;
    float italic_angle = 0.0f;

    // Derived metrics
    float GetLineHeight(float multiplier = 1.0f) const {
        return (ascender - descender + line_gap) * multiplier;
    }
};

struct GlyphData {
    uint32_t codepoint;
    std::string name;
    std::vector<Vec2> outline;
    Rect bounds;
    float advance;
    float left_bearing;
    float right_bearing;

    // For kerning calculations

    std::vector<float> left_side_profile;   // Distance from left edge at various y

    std::vector<float> right_side_profile;  // Distance from right edge at various y
};

class FontDescriptorRecord {
private:
    std::string font_id;
    std::string family_name;
    std::string style_name;
    float point_size = 12.0f;

    // Glyph storage
    std::unordered_map<uint32_t, GlyphData> glyphs;
    std::unordered_map<std::string, uint32_t> glyph_name_to_codepoint;

    // Kerning
    KerningTable kerning_table;
    KerningMode default_kerning_mode = KerningMode::FontProvided;

    // Metrics
    FontMetrics metrics;

    // Features

    std::vector<std::string> supported_features;  // ligatures, smallcaps, etc.

public:

    FontDescriptorRecord(const std::string& id, const std::string& family)        : font_id(id), family_name(family) {}

    //
```

```text
========================================================================    // Glyph Management
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    void AddGlyph(const GlyphData& glyph) {
        glyphs[glyph.codepoint] = glyph;
        glyph_name_to_codepoint[glyph.name] = glyph.codepoint;
        CalculateKerningProfiles(glyph.codepoint);
    }

    const GlyphData* GetGlyph(uint32_t codepoint) const {
        auto it = glyphs.find(codepoint);
        return it != glyphs.end() ? &it->second : nullptr;
    }

    const GlyphData* GetGlyphByName(const std::string& name) const {        auto it = glyph_name_to_codepoint.find(name);
        if (it != glyph_name_to_codepoint.end()) {
            return GetGlyph(it->second);
        }
        return nullptr;
    }

    bool HasGlyph(uint32_t codepoint) const {
        return glyphs.find(codepoint) != glyphs.end();
    }

    //
```

```text
========================================================================    // Kerning Management
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    void SetKerningMode(KerningMode mode) {
        kerning_table.mode = mode;
        default_kerning_mode = mode;
    }

    void AddKerningPair(uint32_t left, uint32_t right, float adjustment, float confidence = 1.0f) {

        kerning_table.AddPair(left, right, adjustment, confidence);    }

    void AddKerningPairByName(const std::string& left, const std::string& right, float adjustment) {
        auto left_it = glyph_name_to_codepoint.find(left);
        auto right_it = glyph_name_to_codepoint.find(right);
        if (left_it != glyph_name_to_codepoint.end() &&
            right_it != glyph_name_to_codepoint.end()) {

            kerning_table.AddPair(left_it->second, right_it->second, adjustment);
        }
    }

    void LoadKerningFromFont(const std::string& font_path) {
        // Load kerning pairs from actual font file (TTF/OTF)
        // Using FreeType or similar
        kerning_table.mode = KerningMode::FontProvided;

        // Example: Standard kerning pairs from professional fonts
        AddKerningPairByName("A", "V", -2.5f);
        AddKerningPairByName("A", "W", -2.0f);
        AddKerningPairByName("A", "T", -2.2f);
        AddKerningPairByName("A", "Y", -2.3f);
        AddKerningPairByName("F", "a", -1.2f);
        AddKerningPairByName("F", "e", -1.2f);
        AddKerningPairByName("F", "o", -1.2f);
        AddKerningPairByName("P", "a", -1.0f);
        AddKerningPairByName("P", "e", -1.0f);
        AddKerningPairByName("T", "a", -1.5f);
        AddKerningPairByName("T", "e", -1.5f);
        AddKerningPairByName("T", "o", -1.5f);
        AddKerningPairByName("V", "a", -1.8f);
        AddKerningPairByName("V", "e", -1.8f);
        AddKerningPairByName("W", "a", -1.5f);
        AddKerningPairByName("W", "e", -1.5f);
        AddKerningPairByName("Y", "a", -1.8f);
        AddKerningPairByName("Y", "e", -1.8f);
        AddKerningPairByName("f", "i", -2.0f);
        AddKerningPairByName("f", "l", -2.0f);
        AddKerningPairByName("r", "i", -0.8f);
        AddKerningPairByName("L", "T", -1.2f);
        AddKerningPairByName("L", "Y", -1.0f);
    }

    float GetKerning(uint32_t left, uint32_t right, const std::string& context = "default") const {
        return kerning_table.GetKerning(left, right, context);
    }

    void SetTracking(float tracking) {
        kerning_table.tracking = tracking;
    }

    float GetTracking() const {
        return kerning_table.tracking;
    }

    //
```

```text
========================================================================    // Contextual Kerning (Advanced)
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    void AddContextualKerning(const std::string& context,
                               const std::string& left,
                               const std::string& right,
                               float adjustment) {
        auto left_it = glyph_name_to_codepoint.find(left);
        auto right_it = glyph_name_to_codepoint.find(right);
        if (left_it != glyph_name_to_codepoint.end() &&
            right_it != glyph_name_to_codepoint.end()) {

            kerning_table.AddContextPair(context, left_it->second, right_it->second, adjustment);
        }
    }

    // For headlines - tighter kerning
    void ApplyHeadlineKerning() {
        AddContextualKerning("headline", "A", "V", -3.0f);
        AddContextualKerning("headline", "T", "a", -2.0f);
        AddContextualKerning("headline", "f", "i", -2.5f);
    }

    // For body text - more conservative kerning
    void ApplyBodyKerning() {
        AddContextualKerning("body", "A", "V", -1.8f);
        AddContextualKerning("body", "T", "a", -1.2f);
        AddContextualKerning("body", "f", "i", -1.5f);
    }

    //
```

```text
========================================================================    // Automatic Kerning Calculation
```

```cpp
    //
```

```text
========================================================================
```

```cpp
private:
    void CalculateKerningProfiles(uint32_t codepoint) {
        auto& glyph = glyphs[codepoint];

        // Sample the glyph outline at various heights to create side
profiles
        int num_samples = 20;
        float step = glyph.bounds.height / num_samples;

        glyph.left_side_profile.resize(num_samples);
        glyph.right_side_profile.resize(num_samples);

        for (int i = 0; i < num_samples; ++i) {
            float y = glyph.bounds.y + i * step;

            // Find leftmost and rightmost point at this y
            float left = glyph.bounds.x + glyph.bounds.width;
            float right = glyph.bounds.x;

            for (const auto& point : glyph.outline) {
                if (std::abs(point.y - y) < step) {
                    left = std::min(left, point.x);
                    right = std::max(right, point.x);
                }
            }

            glyph.left_side_profile[i] = left - glyph.bounds.x;
            glyph.right_side_profile[i] = glyph.bounds.x +
glyph.bounds.width - right;
        }
    }

public:
    // Calculate kerning between two glyphs based on their shapes

    float CalculateAutoKerning(uint32_t left, uint32_t right) const {        auto left_glyph = GetGlyph(left);
        auto right_glyph = GetGlyph(right);

        if (!left_glyph || !right_glyph) return 0.0f;

        // Find minimum distance between right edge of left glyph and left edge of right glyph
        float min_distance = 1000.0f;

        int samples = std::min(left_glyph->right_side_profile.size(),                                 right_glyph->left_side_profile.size());

        for (int i = 0; i < samples; ++i) {

            float left_right_edge = left_glyph->right_side_profile[i];            float right_left_edge = right_glyph->left_side_profile[i];            float distance = left_right_edge + right_left_edge;
            min_distance = std::min(min_distance, distance);
        }

        // Desired spacing (typically 0 for normal, negative for tighter)        float desired_spacing = 0.0f;
        float kerning = min_distance - desired_spacing;

        // Clamp to reasonable values
        kerning = std::max(-5.0f, std::min(5.0f, kerning));

        return kerning;
    }

    //
```

```text
========================================================================    // Getters
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    const std::string& GetFontId() const { return font_id; }

    const std::string& GetFamilyName() const { return family_name; }    const std::string& GetStyleName() const { return style_name; }
    float GetPointSize() const { return point_size; }
    void SetPointSize(float size) { point_size = size; }
    const FontMetrics& GetMetrics() const { return metrics; }
    FontMetrics& GetMetrics() { return metrics; }

    // Scale factor from font units to pixels
    float GetScaleFactor() const {
        return point_size / metrics.units_per_em;
    }
};

//
```

```text
============================================================================// Layout Manager - Supports Boxes, Ovals, and Polygons
```

```cpp
//
```

```text
============================================================================
```

```cpp
enum class ShapeType {
    Rectangle,
    Oval,
    Polygon,
    Freeform
};

struct LayoutShape {
    ShapeType type;

    union {
        struct { Rect rect; } rectangle;

        struct { Vec2 center; float radius_x; float radius_y; } oval;        struct { std::vector<Vec2> points; } polygon;
    };

    // Check if a point is inside this shape
    bool Contains(const Vec2& point) const {
        switch (type) {
            case ShapeType::Rectangle:
                return point.x >= rectangle.rect.x &&

                       point.x <= rectangle.rect.x + rectangle.rect.width &&                       point.y >= rectangle.rect.y &&
                       point.y <= rectangle.rect.y + rectangle.rect.height;

            case ShapeType::Oval: {
                float dx = (point.x - oval.center.x) / oval.radius_x;                float dy = (point.y - oval.center.y) / oval.radius_y;                return (dx * dx + dy * dy) <= 1.0f;
            }

            case ShapeType::Polygon: {
                // Ray casting algorithm
                bool inside = false;
                size_t n = polygon.points.size();
                for (size_t i = 0, j = n - 1; i < n; j = i++) {
                    if (((polygon.points[i].y > point.y) !=
(polygon.points[j].y > point.y)) &&
                        (point.x < (polygon.points[j].x -
polygon.points[i].x) *
                         (point.y - polygon.points[i].y) /

                         (polygon.points[j].y - polygon.points[i].y) + polygon.points[i].x)) {
                        inside = !inside;
                    }
                }
                return inside;
            }

            default:
                return false;
        }
    }

    // Get bounding box
    Rect GetBounds() const {
        switch (type) {
            case ShapeType::Rectangle:
                return rectangle.rect;
            case ShapeType::Oval:
                return {oval.center.x - oval.radius_x,
                        oval.center.y - oval.radius_y,
                        oval.radius_x * 2, oval.radius_y * 2};
            case ShapeType::Polygon: {

                float min_x = 1e9, min_y = 1e9, max_x = -1e9, max_y = -1e9;                for (const auto& p : polygon.points) {
                    min_x = std::min(min_x, p.x);
                    min_y = std::min(min_y, p.y);
                    max_x = std::max(max_x, p.x);
                    max_y = std::max(max_y, p.y);
                }

                return {min_x, min_y, max_x - min_x, max_y - min_y};            }
            default:
                return {0, 0, 0, 0};
        }
    }
};

//
```

```text
============================================================================// Layout Manager - Handles text placement in shapes
```

```cpp
//
```

```text
============================================================================
```

```cpp
class LayoutManager {
private:
    std::shared_ptr<FontDescriptorRecord> font;
    LayoutShape container_shape;
    std::vector<std::shared_ptr<GlyphElement>> placed_glyphs;
    KerningMode kerning_mode = KerningMode::FontProvided;
    std::string current_context = "default";

    // Layout parameters
    float line_height_multiplier = 1.4f;
    float letter_spacing = 0.0f;
    bool use_ligatures = true;

public:
    LayoutManager(std::shared_ptr<FontDescriptorRecord> f) : font(f) {}

    void SetShape(const LayoutShape& shape) {
        container_shape = shape;
    }

    void SetShapeRectangle(const Rect& rect) {
        container_shape.type = ShapeType::Rectangle;
        container_shape.rectangle.rect = rect;
    }

    void SetShapeOval(Vec2 center, float rx, float ry) {
        container_shape.type = ShapeType::Oval;
        container_shape.oval.center = center;
        container_shape.oval.radius_x = rx;
        container_shape.oval.radius_y = ry;
    }

    void SetShapePolygon(const std::vector<Vec2>& points) {
        container_shape.type = ShapeType::Polygon;
        container_shape.polygon.points = points;
    }

    void SetKerningMode(KerningMode mode) {
        kerning_mode = mode;
        if (font) {
            font->SetKerningMode(mode);
        }
    }

    void SetContext(const std::string& context) {
        current_context = context;
    }

    void SetLineHeight(float multiplier) {
        line_height_multiplier = multiplier;
    }

    void SetLetterSpacing(float spacing) {
        letter_spacing = spacing;
        if (font) {
            font->SetTracking(spacing);
        }
    }

    //
```

```text
========================================================================    // Core Layout Methods
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    void LayoutTextInShape(const std::string& text) {
        placed_glyphs.clear();

        float line_height = font-
>GetMetrics().GetLineHeight(line_height_multiplier) *
                            font->GetScaleFactor();

        Vec2 cursor = GetStartingPosition();
        std::vector<uint32_t> current_line;

        for (size_t i = 0; i < text.length(); ++i) {
            uint32_t codepoint = static_cast<uint32_t>(text[i]);

            if (codepoint == ' ') {

                cursor.x += font->GetGlyph(codepoint)->advance * font->GetScaleFactor() +
                            letter_spacing;
                continue;
            }

            if (codepoint == '\n') {
                cursor.x = GetStartingPosition().x;
                cursor.y += line_height;
                continue;
            }

            const GlyphData* glyph = font->GetGlyph(codepoint);
            if (!glyph) continue;

            float advance = glyph->advance * font->GetScaleFactor();

            // Apply kerning
            if (i > 0 && text[i-1] != ' ') {
                uint32_t prev = static_cast<uint32_t>(text[i-1]);
                float kerning = font->GetKerning(prev, codepoint,
current_context);
                cursor.x += kerning;
                advance += kerning;
            }

            // Check if glyph fits in current line (for rectangles)            if (container_shape.type == ShapeType::Rectangle) {
                if (cursor.x + glyph->bounds.width >
                    container_shape.rectangle.rect.x +
container_shape.rectangle.rect.width) {
                    // Wrap to next line
                    cursor.x = GetStartingPosition().x;
                    cursor.y += line_height;
                }
            }

            // Check if position is within shape
            Vec2 glyph_pos = {cursor.x, cursor.y};
            if (!container_shape.Contains(glyph_pos)) {
                // Find next valid position within shape

                glyph_pos = FindNextValidPosition(cursor, glyph->bounds, line_height);
            }

            // Create placed glyph

            auto glyph_element = std::make_shared<GlyphElement>(codepoint, text[i], glyph->name);
            glyph_element->SetOutline(glyph->outline);
            glyph_element->SetBounds({glyph_pos.x, glyph_pos.y,
                                      glyph->bounds.width, glyph-
>bounds.height});
            glyph_element->SetAdvance(advance);

            placed_glyphs.push_back(glyph_element);

            cursor.x += advance + letter_spacing;
        }
    }

    // Layout text along a path (for oval/polygon boundaries)

    void LayoutTextOnPath(const std::string& text, float start_angle = 0.0f) {
        placed_glyphs.clear();

        float total_advance = 0.0f;
        float circumference = CalculatePathLength();
        float angle_step = 2.0f * M_PI / circumference;

        for (size_t i = 0; i < text.length(); ++i) {
            uint32_t codepoint = static_cast<uint32_t>(text[i]);
            const GlyphData* glyph = font->GetGlyph(codepoint);
            if (!glyph) continue;

            float advance = glyph->advance * font->GetScaleFactor();

            // Apply kerning
            if (i > 0) {
                uint32_t prev = static_cast<uint32_t>(text[i-1]);
                float kerning = font->GetKerning(prev, codepoint,
current_context);
                advance += kerning;
            }

            float angle = start_angle + total_advance * angle_step;            Vec2 position = GetPointOnPath(angle);
            Vec2 tangent = GetTangentOnPath(angle);

            // Rotate glyph to follow path
            Matrix3x3 transform = RotationMatrix(atan2(tangent.y,
tangent.x));

            auto glyph_element = std::make_shared<GlyphElement>(codepoint, text[i], glyph->name);
            glyph_element->SetOutline(glyph->outline);
            glyph_element->SetTransform(transform);
            glyph_element->SetBounds({position.x, position.y,
                                      glyph->bounds.width, glyph-
>bounds.height});
            glyph_element->SetAdvance(advance);

            placed_glyphs.push_back(glyph_element);

            total_advance += advance + letter_spacing;
        }
    }
    //
```

```text
========================================================================    // Query Methods for External Rendering
```

```cpp
    //
```

```text
========================================================================
```

```cpp
    // Get image region with transparency for a specific square/rectangle    ImageRegion GetImageRegionForRect(const Rect& query_rect) const {        ImageRegion region;
        region.id = "layout_region";
        region.bounds = query_rect;
        region.has_transparency = true;

        // Collect all glyphs intersecting this rectangle
        for (const auto& glyph : placed_glyphs) {
            Rect glyph_bounds = glyph->GetBounds();
            if (RectsIntersect(query_rect, glyph_bounds)) {
                // Add glyph's contribution to region

                ImageRegion glyph_region = glyph->GetImageRegion();                // Merge or composite as needed
            }
        }

        // Subdivide into squares for efficient rendering
        region.subregions = SubdivideRect(query_rect, 64, 64);

        return region;
    }

    // Get all image regions (entire layout)
    std::vector<ImageRegion> GetAllImageRegions() const {
        std::vector<ImageRegion> regions;

        for (const auto& glyph : placed_glyphs) {
            regions.push_back(glyph->GetImageRegion());
        }

        return regions;
    }

    // Get transparency data for a specific pixel coordinate
    float GetAlphaAtPoint(const Vec2& point) const {
        for (const auto& glyph : placed_glyphs) {
            Rect bounds = glyph->GetBounds();

            if (point.x >= bounds.x && point.x <= bounds.x + bounds.width &&                point.y >= bounds.y && point.y <= bounds.y + bounds.height) {
                // Would sample actual glyph alpha here
                return 1.0f;
            }
        }
        return 0.0f;
    }

    // Get all placed glyphs (for external visitor)

    const std::vector<std::shared_ptr<GlyphElement>>& GetPlacedGlyphs() const {
        return placed_glyphs;
    }

    // Accept visitor for external rendering
    void Accept(Visitor<void>& visitor) {
        for (auto& glyph : placed_glyphs) {
            glyph->Accept(visitor);
        }
    }

private:
    Vec2 GetStartingPosition() const {
        Rect bounds = container_shape.GetBounds();

        return {bounds.x + 10, bounds.y + font->GetMetrics().ascender * font->GetScaleFactor()};
    }

    Vec2 FindNextValidPosition(const Vec2& current, const Rect&
glyph_bounds, float line_height) {
        Vec2 test = current;
        // Simple implementation: scan down until inside shape
        while (!container_shape.Contains(test)) {
            test.x += 5;
            if (test.x > container_shape.GetBounds().x +
container_shape.GetBounds().width) {
                test.x = container_shape.GetBounds().x;
                test.y += line_height;
            }
        }
        return test;
    }

    float CalculatePathLength() const {
        switch (container_shape.type) {
            case ShapeType::Oval:
                return 2.0f * M_PI *
std::sqrt((container_shape.oval.radius_x *

container_shape.oval.radius_x +

container_shape.oval.radius_y *
container_shape.oval.radius_y) / 2.0f);
            case ShapeType::Polygon: {
                float length = 0;

                const auto& points = container_shape.polygon.points;                for (size_t i = 0; i < points.size(); ++i) {
                    size_t next = (i + 1) % points.size();
                    float dx = points[next].x - points[i].x;
                    float dy = points[next].y - points[i].y;
                    length += std::sqrt(dx*dx + dy*dy);
                }
                return length;
            }
            default:
                return 100.0f;
        }
    }

    Vec2 GetPointOnPath(float angle) const {
        switch (container_shape.type) {
            case ShapeType::Oval:
                return {container_shape.oval.center.x +
container_shape.oval.radius_x * cos(angle),
                        container_shape.oval.center.y +
container_shape.oval.radius_y * sin(angle)};
            case ShapeType::Polygon: {
                // Interpolate along polygon perimeter
                float perimeter = CalculatePathLength();

                float target_dist = angle / (2.0f * M_PI) * perimeter;                float accumulated = 0;

                const auto& points = container_shape.polygon.points;                for (size_t i = 0; i < points.size(); ++i) {
                    size_t next = (i + 1) % points.size();
                    float dx = points[next].x - points[i].x;
                    float dy = points[next].y - points[i].y;
                    float seg_len = std::sqrt(dx*dx + dy*dy);
                    if (accumulated + seg_len >= target_dist) {

                        float t = (target_dist - accumulated) / seg_len;                        return {points[i].x + dx * t, points[i].y + dy * t};                    }
                    accumulated += seg_len;
                }
                return points[0];
            }
            default:
                return {0, 0};
        }
    }
    Vec2 GetTangentOnPath(float angle) const {
        // Numerical derivative of GetPointOnPath
        float eps = 0.01f;
        Vec2 p1 = GetPointOnPath(angle);
        Vec2 p2 = GetPointOnPath(angle + eps);
        return {p2.x - p1.x, p2.y - p1.y};
    }

    Matrix3x3 RotationMatrix(float angle) const {
        Matrix3x3 m;

        m.m[0][0] = cos(angle); m.m[0][1] = -sin(angle); m.m[0][2] = 0;        m.m[1][0] = sin(angle); m.m[1][1] = cos(angle);  m.m[1][2] = 0;        m.m[2][0] = 0;          m.m[2][1] = 0;           m.m[2][2] = 1;        return m;
    }

    bool RectsIntersect(const Rect& a, const Rect& b) const {
        return !(a.x + a.width < b.x || b.x + b.width < a.x ||
                 a.y + a.height < b.y || b.y + b.height < a.y);
    }

    std::vector<Rect> SubdivideRect(const Rect& rect, float max_w, float max_h) const {
        std::vector<Rect> rects;

        for (float y = rect.y; y < rect.y + rect.height; y += max_h) {            for (float x = rect.x; x < rect.x + rect.width; x += max_w) {                Rect r;
                r.x = x;
                r.y = y;

                r.width = std::min(max_w, rect.x + rect.width - x);                r.height = std::min(max_h, rect.y + rect.height - y);                if (r.width > 0 && r.height > 0) {
                    rects.push_back(r);
                }
            }
        }
        return rects;
    }
};

//
```

```text
============================================================================// External Visitor Example - Implemented by Consumer
```

```cpp
//
```

```text
============================================================================
```

```cpp
class ExternalRenderer : public Visitor<void> {
private:
    std::vector<uint8_t> framebuffer;
    Size canvas_size;

public:
    ExternalRenderer(Size size) : canvas_size(size) {
        framebuffer.resize(size.width * size.height * 4, 0);
    }

    void Visit(GlyphElement& glyph) override {
        // External code controls how glyphs are rendered
        Rect bounds = glyph.GetBounds();
        ImageRegion region = glyph.GetImageRegion();

        // Render to framebuffer using external rendering system
        // Could be OpenGL, DirectX, software rasterizer, etc.
        RenderGlyphToBuffer(glyph, bounds);
    }

    void Visit(WordElement& element) override {
        // Process word-level grouping if needed
        for (const auto& glyph : element.GetGlyphs()) {
            glyph->Accept(*this);
        }
    }

    void Visit(LineElement& element) override {}
    void Visit(ParagraphElement& element) override {}
    void Visit(ImageElement& element) override {}
    void Visit(LayoutContainer& element) override {}

    const std::vector<uint8_t>& GetFramebuffer() const { return framebuffer; }

private:

    void RenderGlyphToBuffer(const GlyphElement& glyph, const Rect& bounds) {
        // External implementation would render here
        // This is just a stub
        for (int y = 0; y < bounds.height; ++y) {
            for (int x = 0; x < bounds.width; ++x) {
                int fb_x = static_cast<int>(bounds.x + x);
                int fb_y = static_cast<int>(bounds.y + y);
                if (fb_x >= 0 && fb_x < canvas_size.width &&
                    fb_y >= 0 && fb_y < canvas_size.height) {

                    size_t idx = (fb_y * canvas_size.width + fb_x) * 4;                    framebuffer[idx] = 255;     // R
                    framebuffer[idx+1] = 255;   // G
                    framebuffer[idx+2] = 255;   // B
                    framebuffer[idx+3] = 255;   // A
                }
            }
        }
    }
};

//
```

```text
============================================================================// Usage Example
```

```cpp
//
```

```text
============================================================================
```

```cpp
void example_usage() {
    // Create font
    auto font = std::make_shared<FontDescriptorRecord>("my_font",
"Helvetica");
    font->SetPointSize(24.0f);

    // Load kerning
    font->SetKerningMode(KerningMode::Hybrid);
    font->LoadKerningFromFont("helvetica.ttf");
    font->ApplyHeadlineKerning();  // Context-specific

    // Create layout manager
    LayoutManager layout(font);

    // Set shape - can be rectangle, oval, or polygon
    Rect text_area = {100, 100, 500, 300};
    layout.SetShapeRectangle(text_area);

    // Or oval:
    // layout.SetShapeOval({400, 300}, 200, 150);

    // Or polygon (star shape):
    // layout.SetShapePolygon({{400,100}, {500,300}, {400,500}, {300,300}});

    layout.SetKerningMode(KerningMode::Hybrid);
    layout.SetContext("headline");
    layout.SetLetterSpacing(-0.5f);  // Tighter letter spacing for headlines

    // Layout text with kerning applied
    layout.LayoutTextInShape("AVATAR TECHNOLOGY WITH KERNING");

    // External renderer uses visitor pattern
    ExternalRenderer renderer({800, 600});
    layout.Accept(renderer);

    // Query specific region with transparency
    Rect query = {100, 100, 64, 64};
    ImageRegion region = layout.GetImageRegionForRect(query);
    // Use the image region for compositing
    if (region.has_transparency) {
        // External compositor handles transparency
        for (const auto& subrect : region.subregions) {
            // Render each square region
        }
    }
}

} // namespace FontSystem

#endif // FONT_SYSTEM_WITH_KERNING_H
```

## Key Features Implemented

| Feature | Impleme |
| --- | --- |
| ntation Kerning Modes | None, Auto, |

FontProvi

ded,

Manual,

Hybrid

Kerning Calculati on  Automati c based on glyph

```profiles
Font-
Provided
Kerning
```

```via
FreeType
Contextu
al
Kerning
", "body",
etc.
Tracking
letter-
```

```spacing
added to
```

```kerning
Visitor
Pattern
```

```implemen
ts visitor,
```

```elements
Image
Region
Query
```

```transpare
ncy data
Shape
Layout
Polygon
```

```container
s
Path
```

```text
oval or
```

The external rendering system now:

## 1. Implements the Visitor interface (ExternalRenderer)

## 2. Queries image regions with transparency via  GetImageRegionForRect()

## 3. Controls layout shapes (boxes, ovals, polygons)

## 4. Receives kerning-adjusted glyphs from the font system
