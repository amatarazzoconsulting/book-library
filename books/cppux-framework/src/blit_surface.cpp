// MIT License - (c) Anthony Matarazzo 2026
//
// blit_surface.cpp - clipped copy and composite for 32-bit surfaces.

#include "blit_surface.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace cppux {

namespace {

inline uint8_t sat_add(uint8_t a, uint8_t b) {
    unsigned s = static_cast<unsigned>(a) + static_cast<unsigned>(b);
    return static_cast<uint8_t>(s > 255u ? 255u : s);
}

inline uint8_t mul255(uint8_t a, uint8_t b) {
    unsigned t = static_cast<unsigned>(a) * static_cast<unsigned>(b) + 128u;
    return static_cast<uint8_t>((t + (t >> 8)) >> 8);
}

inline uint8_t lerp255(uint8_t a, uint8_t b, unsigned w) {
    // w in 0..256
    return static_cast<uint8_t>((static_cast<unsigned>(a) * (256u - w) +
                                 static_cast<unsigned>(b) * w) >> 8);
}

} // namespace

const char* to_string(copy_mode m) {
    switch (m) {
        case copy_mode::copy: return "copy";
        case copy_mode::over: return "over";
        case copy_mode::add: return "add";
        case copy_mode::color_key: return "color_key";
    }
    return "unknown";
}

const char* to_string(blend_mode m) {
    switch (m) {
        case blend_mode::nearest: return "nearest";
        case blend_mode::bilinear: return "bilinear";
        case blend_mode::bilinear_alpha: return "bilinear_alpha";
    }
    return "unknown";
}

rect rect::intersect(const rect& a, const rect& b) {
    int x0 = std::max(a.x, b.x);
    int y0 = std::max(a.y, b.y);
    int x1 = std::min(a.right(), b.right());
    int y1 = std::min(a.bottom(), b.bottom());
    if (x1 <= x0 || y1 <= y0) return rect{0, 0, 0, 0};
    return rect{x0, y0, x1 - x0, y1 - y0};
}

rect rect::unite(const rect& a, const rect& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    int x0 = std::min(a.x, b.x);
    int y0 = std::min(a.y, b.y);
    int x1 = std::max(a.right(), b.right());
    int y1 = std::max(a.bottom(), b.bottom());
    return rect{x0, y0, x1 - x0, y1 - y0};
}

surface::surface(int width, int height, color32 fill_color) {
    resize(width, height, fill_color);
}

void surface::resize(int width, int height, color32 fill_color) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    data_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), fill_color.v);
}

color32 surface::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return color32{0};
    return color32{row(y)[x]};
}

void surface::set(int x, int y, color32 c) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    row(y)[x] = c.v;
}

void surface::fill(color32 c) {
    std::fill(data_.begin(), data_.end(), c.v);
}

void surface::fill_rect(const rect& r, color32 c) {
    rect a = rect::intersect(r, bounds());
    for (int y = a.y; y < a.bottom(); ++y) {
        uint32_t* p = row(y) + a.x;
        std::fill(p, p + a.w, c.v);
    }
}

size_t surface::diff(const surface& other, const rect* clip) const {
    rect a = rect::intersect(bounds(), other.bounds());
    if (clip) a = rect::intersect(a, *clip);
    size_t n = 0;
    for (int y = a.y; y < a.bottom(); ++y) {
        const uint32_t* p = row(y) + a.x;
        const uint32_t* q = other.row(y) + a.x;
        for (int x = 0; x < a.w; ++x) {
            if (p[x] != q[x]) ++n;
        }
    }
    return n;
}

uint64_t surface::checksum() const {
    uint64_t h = 1469598103934665603ull;
    for (uint32_t v : data_) {
        h ^= v;
        h *= 1099511628211ull;
    }
    return h;
}

color32 blend_over(color32 dst, color32 src, uint8_t global_alpha) {
    unsigned sa = src.a();
    if (global_alpha != 255) sa = mul255(static_cast<uint8_t>(sa), global_alpha);
    if (sa == 0) return dst;
    if (sa == 255) return src;

    unsigned inv = 255u - sa;
    unsigned r = (static_cast<unsigned>(src.r()) * sa + static_cast<unsigned>(dst.r()) * inv + 127u) / 255u;
    unsigned g = (static_cast<unsigned>(src.g()) * sa + static_cast<unsigned>(dst.g()) * inv + 127u) / 255u;
    unsigned b = (static_cast<unsigned>(src.b()) * sa + static_cast<unsigned>(dst.b()) * inv + 127u) / 255u;
    unsigned da = dst.a();
    unsigned out_a = sa + (da * inv + 127u) / 255u;
    if (out_a > 255u) out_a = 255u;
    return color32{(out_a << 24) | (r << 16) | (g << 8) | b};
}

blit_stats blit(surface& dst, const surface& src, int dx, int dy, const blit_options& opt) {
    return blit_rect(dst, src, src.bounds(), dx, dy, opt);
}

blit_stats blit_rect(surface& dst, const surface& src, const rect& src_rect, int dx, int dy,
                     const blit_options& opt) {
    blit_stats st;
    rect s = rect::intersect(src_rect, src.bounds());
    if (s.empty() || dst.empty()) return st;

    rect target{dx, dy, s.w, s.h};
    rect clip = opt.clip.empty() ? dst.bounds() : rect::intersect(opt.clip, dst.bounds());
    rect out = rect::intersect(target, clip);

    st.pixels_in = static_cast<uint64_t>(s.w) * static_cast<uint64_t>(s.h);
    st.pixels_clipped = st.pixels_in - static_cast<uint64_t>(out.w) * static_cast<uint64_t>(out.h);
    if (out.empty()) return st;

    for (int y = out.y; y < out.bottom(); ++y) {
        int local_y = y - dy;
        if (opt.flip_y) local_y = s.h - 1 - local_y;
        const uint32_t* sp = src.row(s.y + local_y);
        uint32_t* dp = dst.row(y);

        for (int x = out.x; x < out.right(); ++x) {
            int local_x = x - dx;
            if (opt.flip_x) local_x = s.w - 1 - local_x;
            color32 sc{sp[s.x + local_x]};
            uint32_t& dref = dp[x];

            if (opt.mode == copy_mode::color_key && sc == opt.key) {
                ++st.pixels_skipped;
                continue;
            }
            switch (opt.mode) {
                case copy_mode::copy:
                    dref = sc.v;
                    break;
                case copy_mode::over:
                    dref = blend_over(color32{dref}, sc, opt.global_alpha).v;
                    break;
                case copy_mode::add: {
                    color32 d{dref};
                    unsigned alpha = sc.a();
                    if (opt.global_alpha != 255) {
                        alpha = mul255(static_cast<uint8_t>(alpha), opt.global_alpha);
                    }
                    uint8_t r = sat_add(d.r(), static_cast<uint8_t>(mul255(sc.r(), static_cast<uint8_t>(alpha))));
                    uint8_t g = sat_add(d.g(), static_cast<uint8_t>(mul255(sc.g(), static_cast<uint8_t>(alpha))));
                    uint8_t b = sat_add(d.b(), static_cast<uint8_t>(mul255(sc.b(), static_cast<uint8_t>(alpha))));
                    dref = color32::rgba(r, g, b, std::max(d.a(), sc.a())).v;
                    break;
                }
                case copy_mode::color_key:
                    dref = sc.v;
                    break;
            }
            ++st.pixels_written;
        }
    }
    st.bytes_moved = st.pixels_written * sizeof(uint32_t);
    return st;
}

blit_stats composite(surface& dst, const surface& src, const rect& dst_rect, blend_mode mode,
                     const rect* clip) {
    blit_stats st;
    if (dst.empty() || src.empty() || dst_rect.empty()) return st;

    rect area = clip ? rect::intersect(dst_rect, *clip) : dst_rect;
    area = rect::intersect(area, dst.bounds());
    if (area.empty()) return st;

    st.pixels_in = static_cast<uint64_t>(dst_rect.w) * static_cast<uint64_t>(dst_rect.h);

    const double sx_scale = static_cast<double>(src.width()) / static_cast<double>(dst_rect.w);
    const double sy_scale = static_cast<double>(src.height()) / static_cast<double>(dst_rect.h);

    for (int y = area.y; y < area.bottom(); ++y) {
        uint32_t* dp = dst.row(y);
        double v = (static_cast<double>(y - dst_rect.y) + 0.5) * sy_scale - 0.5;

        for (int x = area.x; x < area.right(); ++x) {
            double u = (static_cast<double>(x - dst_rect.x) + 0.5) * sx_scale - 0.5;
            color32 c;

            if (mode == blend_mode::nearest) {
                int ix = static_cast<int>(u + 0.5);
                int iy = static_cast<int>(v + 0.5);
                ix = std::min(std::max(ix, 0), src.width() - 1);
                iy = std::min(std::max(iy, 0), src.height() - 1);
                c = src.at(ix, iy);
                dp[x] = blend_over(color32{dp[x]}, c).v;
            } else {
                int x0 = static_cast<int>(std::floor(u));
                int y0 = static_cast<int>(std::floor(v));
                int x1 = x0 + 1;
                int y1 = y0 + 1;
                unsigned fx = static_cast<unsigned>((u - x0) * 256.0);
                unsigned fy = static_cast<unsigned>((v - y0) * 256.0);
                x0 = std::min(std::max(x0, 0), src.width() - 1);
                y0 = std::min(std::max(y0, 0), src.height() - 1);
                x1 = std::min(std::max(x1, 0), src.width() - 1);
                y1 = std::min(std::max(y1, 0), src.height() - 1);

                color32 c00 = src.at(x0, y0);
                color32 c10 = src.at(x1, y0);
                color32 c01 = src.at(x0, y1);
                color32 c11 = src.at(x1, y1);

                if (mode == blend_mode::bilinear) {
                    uint8_t r = lerp255(lerp255(c00.r(), c10.r(), fx), lerp255(c01.r(), c11.r(), fx), fy);
                    uint8_t g = lerp255(lerp255(c00.g(), c10.g(), fx), lerp255(c01.g(), c11.g(), fx), fy);
                    uint8_t b = lerp255(lerp255(c00.b(), c10.b(), fx), lerp255(c01.b(), c11.b(), fx), fy);
                    uint8_t a = lerp255(lerp255(c00.a(), c10.a(), fx), lerp255(c01.a(), c11.a(), fx), fy);
                    c = color32::rgba(r, g, b, a);
                    dp[x] = blend_over(color32{dp[x]}, c).v;
                } else {
                    // Premultiply, interpolate, then unpremultiply for correct edges.
                    auto pre = [](color32 p) {
                        unsigned a = p.a();
                        return std::array<unsigned, 3>{
                            static_cast<unsigned>(p.r()) * a,
                            static_cast<unsigned>(p.g()) * a,
                            static_cast<unsigned>(p.b()) * a};
                    };
                    auto p00 = pre(c00), p10 = pre(c10), p01 = pre(c01), p11 = pre(c11);
                    unsigned a = lerp255(lerp255(c00.a(), c10.a(), fx), lerp255(c01.a(), c11.a(), fx), fy);
                    unsigned r = lerp255(lerp255(static_cast<uint8_t>(p00[0] / 255), static_cast<uint8_t>(p10[0] / 255), fx),
                                         lerp255(static_cast<uint8_t>(p01[0] / 255), static_cast<uint8_t>(p11[0] / 255), fx), fy);
                    unsigned g = lerp255(lerp255(static_cast<uint8_t>(p00[1] / 255), static_cast<uint8_t>(p10[1] / 255), fx),
                                         lerp255(static_cast<uint8_t>(p01[1] / 255), static_cast<uint8_t>(p11[1] / 255), fx), fy);
                    unsigned b = lerp255(lerp255(static_cast<uint8_t>(p00[2] / 255), static_cast<uint8_t>(p10[2] / 255), fx),
                                         lerp255(static_cast<uint8_t>(p01[2] / 255), static_cast<uint8_t>(p11[2] / 255), fx), fy);
                    if (a > 0) {
                        r = std::min(255u, (r * 255u + a / 2) / a);
                        g = std::min(255u, (g * 255u + a / 2) / a);
                        b = std::min(255u, (b * 255u + a / 2) / a);
                    }
                    c = color32::rgba(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                      static_cast<uint8_t>(b), static_cast<uint8_t>(a));
                    dp[x] = blend_over(color32{dp[x]}, c).v;
                }
            }
            ++st.pixels_written;
        }
    }
    st.bytes_moved = st.pixels_written * sizeof(uint32_t);
    return st;
}

} // namespace cppux
