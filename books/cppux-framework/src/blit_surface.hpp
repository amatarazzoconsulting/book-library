// MIT License - (c) Anthony Matarazzo 2026
//
// blit_surface.hpp - 32-bit surface, clipped BLIT copy and alpha composite.
//
// The desktop manager's compositor takes the client framebuffer (buffer_update)
// and blits it into the screen surface. This is that path: a tightly packed
// 0xAARRGGBB surface with clipping, four copy modes and Porter-Duff "over"
// compositing at three quality levels.
//
// Copy modes:
//   copy      - straight replacement, ignores source alpha
//   over      - alpha blend source over destination
//   add       - saturating add of channels
//   color_key - pixels equal to the key colour are skipped
//
// Blend modes for composite():
//   nearest          - fastest, aliased edges
//   bilinear         - 4 tap weighted sample
//   bilinear_alpha   - bilinear plus premultiplied alpha correct blending

#ifndef CPPUX_BLIT_SURFACE_HPP
#define CPPUX_BLIT_SURFACE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cppux {

// Packed 0xAARRGGBB, row major, no padding between rows.
struct color32 {
    uint32_t v = 0;

    static constexpr color32 rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return color32{(static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
                       (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b)};
    }
    uint8_t a() const { return static_cast<uint8_t>((v >> 24) & 0xFF); }
    uint8_t r() const { return static_cast<uint8_t>((v >> 16) & 0xFF); }
    uint8_t g() const { return static_cast<uint8_t>((v >> 8) & 0xFF); }
    uint8_t b() const { return static_cast<uint8_t>(v & 0xFF); }

    bool operator==(const color32& o) const { return v == o.v; }
    bool operator!=(const color32& o) const { return v != o.v; }
};

struct rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    int right() const { return x + w; }
    int bottom() const { return y + h; }
    bool empty() const { return w <= 0 || h <= 0; }
    bool contains(int px, int py) const { return px >= x && px < right() && py >= y && py < bottom(); }
    static rect intersect(const rect& a, const rect& b);
    static rect unite(const rect& a, const rect& b);
    static rect from_size(int w, int h) { return rect{0, 0, w, h}; }
};

enum class copy_mode : uint8_t { copy = 0, over, add, color_key };
enum class blend_mode : uint8_t { nearest = 0, bilinear, bilinear_alpha };

const char* to_string(copy_mode m);
const char* to_string(blend_mode m);

class surface {
public:
    surface() = default;
    surface(int width, int height, color32 fill = color32{0});

    void resize(int width, int height, color32 fill = color32{0});

    int width() const { return width_; }
    int height() const { return height_; }
    size_t pixels() const { return data_.size(); }
    size_t bytes() const { return data_.size() * sizeof(uint32_t); }
    bool empty() const { return width_ <= 0 || height_ <= 0; }
    rect bounds() const { return rect{0, 0, width_, height_}; }

    uint32_t* row(int y) { return data_.data() + static_cast<size_t>(y) * static_cast<size_t>(width_); }
    const uint32_t* row(int y) const {
        return data_.data() + static_cast<size_t>(y) * static_cast<size_t>(width_);
    }

    color32 at(int x, int y) const;
    void set(int x, int y, color32 c);
    void fill(color32 c);
    void fill_rect(const rect& r, color32 c);

    // Number of pixels whose value differs from other, within clip.
    size_t diff(const surface& other, const rect* clip = nullptr) const;

    uint64_t checksum() const;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<uint32_t> data_;
};

struct blit_options {
    copy_mode mode = copy_mode::copy;
    rect clip{0, 0, 0, 0};       // empty means "whole destination"
    color32 key{0};              // used by color_key
    uint8_t global_alpha = 255;  // extra multiplier on source alpha
    bool flip_x = false;
    bool flip_y = false;
};

struct blit_stats {
    uint64_t pixels_in = 0;     // pixels the source rect covered
    uint64_t pixels_written = 0;
    uint64_t pixels_clipped = 0;
    uint64_t pixels_skipped = 0; // color_key hits
    uint64_t bytes_moved = 0;
};

// Copy src (at sx,sy,sw,sh) onto dst at (dx,dy).
blit_stats blit(surface& dst, const surface& src, int dx, int dy, const blit_options& opt = {});
blit_stats blit_rect(surface& dst, const surface& src, const rect& src_rect, int dx, int dy,
                     const blit_options& opt = {});

// Scaled copy with nearest, bilinear or alpha correct bilinear sampling.
blit_stats composite(surface& dst, const surface& src, const rect& dst_rect, blend_mode mode,
                     const rect* clip = nullptr);

// Convenience: single pixel blend used by composite and exposed for teaching.
color32 blend_over(color32 dst, color32 src, uint8_t global_alpha = 255);

} // namespace cppux

#endif // CPPUX_BLIT_SURFACE_HPP
