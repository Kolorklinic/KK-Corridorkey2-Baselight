// ---------------------------------------------------------------------------
// CorridorKey 2 for Resolve - host-free helpers (pixels, names, job events)
//
// From Creative Video Tips (CVT)  |  https://creativevideotips.com
// Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
// ---------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace ck2 {

constexpr const char* kVersion = "1.01";
constexpr int kVersionMajor = 1;        // OFX major stays 1 so saved comps keep the node
constexpr int kVersionMinor = 101;      // 0.30 -> 30, ..., 1.00 -> 100, 1.01 -> 101

enum class Output { Premultiplied = 0, MatteOnly = 1, Foreground = 2, SourceWithMatte = 3 };

// What the picture coming in is. CorridorKey works in linear light with
// sRGB/Rec.709 primaries; ACEScg (AP1 primaries) is converted on the way in
// and back on the way out, so the output stays in the input's space.
enum class InputSpace { Display = 0, SrgbLinear = 1, ACEScg = 2 };

// ACEScg (AP1, D60) <-> linear sRGB/Rec.709 (D65), Bradford adaptation.
constexpr float kAcescgToSrgb[9] = {1.7050509927f, -0.6217921207f, -0.0832588720f,
                                    -0.1302564175f, 1.1408047366f, -0.0105483191f,
                                    -0.0240033568f, -0.1289689761f, 1.1529723329f};
constexpr float kSrgbToAcescg[9] = {0.6130973669f, 0.3395229331f, 0.0473796999f,
                                    0.0701937090f, 0.9163538791f, 0.0134524119f,
                                    0.0206155576f, 0.1095697729f, 0.8698146695f};

inline void applyMatrix(const float* m, float& r, float& g, float& b) {
    const float r2 = m[0] * r + m[1] * g + m[2] * b;
    const float g2 = m[3] * r + m[4] * g + m[5] * b;
    const float b2 = m[6] * r + m[7] * g + m[8] * b;
    r = r2; g = g2; b = b2;
}
enum class Screen { Green = 0, Blue = 1 };

inline float srgbEncode(float v) {
    if (v <= 0.0031308f) return 12.92f * v;
    return 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
}

// Takes the screen colour's excess over the average of the other two channels
// and shares it out to them, so brightness stays about the same.
inline void despill(float& r, float& g, float& b, float strength, Screen screen) {
    if (strength <= 0.0f) return;
    float& s = (screen == Screen::Green) ? g : b;
    float& o1 = r;
    float& o2 = (screen == Screen::Green) ? b : g;
    float excess = s - 0.5f * (o1 + o2);
    if (excess <= 0.0f) return;
    excess *= strength;
    s -= excess;
    o1 += 0.5f * excess;
    o2 += 0.5f * excess;
}

struct Look {
    Output output = Output::Premultiplied;
    float despill = 0.5f;
    Screen screen = Screen::Green;
    InputSpace input = InputSpace::Display;
};

// One output pixel from the cached foreground (linear, straight), the matte
// and the source pixel (as the host gave it).
inline void composePixel(const Look& look, const float* fgLinear, float alpha,
                         const float* src, float* dst) {
    float a = std::fmin(std::fmax(alpha, 0.0f), 1.0f);
    switch (look.output) {
        case Output::MatteOnly:
            dst[0] = dst[1] = dst[2] = a; dst[3] = 1.0f;
            return;
        case Output::SourceWithMatte:
            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = a;
            return;
        default: break;
    }
    float r = fgLinear[0], g = fgLinear[1], b = fgLinear[2];
    despill(r, g, b, look.despill, look.screen);
    if (look.input == InputSpace::Display) {
        r = srgbEncode(std::fmax(r, 0.0f));
        g = srgbEncode(std::fmax(g, 0.0f));
        b = srgbEncode(std::fmax(b, 0.0f));
    } else if (look.input == InputSpace::ACEScg) {
        applyMatrix(kSrgbToAcescg, r, g, b);
    }
    if (look.output == Output::Premultiplied) { r *= a; g *= a; b *= a; }
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

enum class HintFrom { None = 0, HintInput = 1, SourceAlpha = 2 };

// Settings that change the matte itself (not the look): part of the cache name.
// hintFrom is what was actually used (None when the Hint input is empty).
inline uint32_t settingsHash(InputSpace input, int window, int precision,
                             HintFrom hintFrom = HintFrom::None, bool hintEveryFrame = true,
                             double hintStrength = 0.5) {
    uint32_t h = 2166136261u;
    const int hs = hintFrom == HintFrom::None ? 0 : static_cast<int>(std::lround(hintStrength * 1000));
    const int hf = hintFrom == HintFrom::None ? 0 : (hintEveryFrame ? 1 : 2);
    for (int v : {static_cast<int>(input) + 10, window, precision, 2 /* cache format: DWAB */,
                  static_cast<int>(hintFrom), hf, hs}) {
        h ^= static_cast<uint32_t>(v);
        h *= 16777619u;
    }
    return h;
}

inline std::string cacheFileName(long frame, uint32_t settings) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "f%07ld_%08x.exr", frame, settings);
    return buf;
}

inline std::string jsonString(const std::string& s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') { o += '\\'; o += c; }
        else if (static_cast<unsigned char>(c) < 0x20) { char b[8]; std::snprintf(b, 8, "\\u%04x", c); o += b; }
        else o += c;
    }
    return o + "\"";
}

// Everything the worker needs for one Process (see load_job() in the worker).
struct JobSpec {
    std::string modelDir, framesDir, workDir, outputDir, hintDir;
    bool inputLinear = false;
    int window = 0;                         // 0 = auto
    std::string precision = "auto";
    HintFrom hintFrom = HintFrom::None;
    bool hintEveryFrame = true;
    double hintStrength = 0.5;
    std::vector<std::string> outputNames;
    bool privateFiles = true;               // results readable only by this user
};

inline std::string jobJson(const JobSpec& j) {
    std::ostringstream js;
    js.imbue(std::locale::classic());        // always 0.5, never 0,5
    js << "{\n  \"model_dir\": " << jsonString(j.modelDir)
       << ",\n  \"frames_dir\": " << jsonString(j.framesDir)
       << ",\n  \"work_dir\": " << jsonString(j.workDir)
       << ",\n  \"output_dir\": " << jsonString(j.outputDir)
       << ",\n  \"input_linear\": " << (j.inputLinear ? "true" : "false")
       << ",\n  \"window\": " << (j.window == 0 ? std::string("\"auto\"") : std::to_string(j.window))
       << ",\n  \"precision\": " << jsonString(j.precision)
       << ",\n  \"hint\": {\"mode\": "
       << (j.hintFrom == HintFrom::None ? "\"none\"" : j.hintEveryFrame ? "\"every\"" : "\"first\"")
       << ", \"dir\": " << jsonString(j.hintDir)
       << ", \"strength\": " << j.hintStrength << ", \"carry\": true}"
       << ",\n  \"private\": " << (j.privateFiles ? "true" : "false")
       << ",\n  \"output_names\": [";
    for (size_t i = 0; i < j.outputNames.size(); ++i)
        js << (i ? ", " : "") << jsonString(j.outputNames[i]);
    js << "]\n}\n";
    return js.str();
}

// --- Thumbnails: tell whether the picture still matches the stored matte ---

constexpr int kThumbW = 64, kThumbH = 36;

// Box-averaged RGB thumbnail of an RGBA float picture (top row first, rows
// `rowStride` floats apart; negative for bottom-up). Values clamped to 0..1.
inline std::vector<float> makeThumb(const float* top, int w, int h, long rowStride) {
    std::vector<double> acc(kThumbW * kThumbH * 3, 0.0);
    std::vector<int> cnt(kThumbW * kThumbH, 0);
    for (int y = 0; y < h; ++y) {
        const float* row = top + rowStride * y;
        const int ty = y * kThumbH / h;
        for (int x = 0; x < w; ++x) {
            const int t = ty * kThumbW + x * kThumbW / w;
            for (int c = 0; c < 3; ++c) {
                float v = row[x * 4 + c];
                acc[t * 3 + c] += v < 0 ? 0 : (v > 1 ? 1 : v);
            }
            ++cnt[t];
        }
    }
    std::vector<float> out(acc.size(), 0.0f);
    for (int t = 0; t < kThumbW * kThumbH; ++t)
        for (int c = 0; c < 3; ++c)
            out[t * 3 + c] = cnt[t] ? static_cast<float>(acc[t * 3 + c] / cnt[t]) : 0.0f;
    return out;
}

inline double thumbDiff(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 1.0;
    double d = 0;
    for (size_t i = 0; i < a.size(); ++i) d += std::fabs(a[i] - b[i]);
    return d / a.size();
}

// Mean difference above which the picture counts as changed since Process.
constexpr double kThumbChanged = 0.03;

inline bool writeThumb(const std::string& path, const std::vector<float>& t) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite("CK2T", 1, 4, f) == 4 &&
              std::fwrite(t.data(), sizeof(float), t.size(), f) == t.size();
    return (std::fclose(f) == 0) && ok;
}

inline bool readThumb(const std::string& path, std::vector<float>& t) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char magic[4];
    t.assign(kThumbW * kThumbH * 3, 0.0f);
    bool ok = std::fread(magic, 1, 4, f) == 4 && std::memcmp(magic, "CK2T", 4) == 0 &&
              std::fread(t.data(), sizeof(float), t.size(), f) == t.size();
    std::fclose(f);
    return ok;
}

// The frames Process covers: the frames the source clip has, within the
// host's timeline bounds (when it gives any), then First..Last within that.
// Asking Resolve's Fusion page for a frame the clip doesn't have never
// returns, so the range must never go past the clip.
inline bool processRange(double clipFirst, double clipLast, double hostFirst, double hostLast,
                         bool useFirstLast, int firstFrame, int lastFrame, long& first, long& last) {
    // Some hosts (Baselight) can't tell a plugin how long its clip is: they
    // report an empty, infinite or absurd range. The range then comes from
    // the timeline bounds, else from First/Last Frame, else there is none.
    const double kMaxSpan = 1e7;
    auto usable = [&](double a, double b) {
        return std::isfinite(a) && std::isfinite(b) && b >= a && b - a < kMaxSpan;
    };
    const bool clipKnown = usable(clipFirst, clipLast);
    const bool hostKnown = usable(hostFirst, hostLast) && hostLast > hostFirst;
    if (!clipKnown) {
        if (hostKnown) {
            first = std::lround(hostFirst);
            last = static_cast<long>(std::floor(hostLast + 1e-6));
        } else if (useFirstLast) {
            first = firstFrame;
            last = lastFrame;
            return last >= first;
        } else {
            return false;
        }
        if (useFirstLast) {
            first = std::max(first, static_cast<long>(firstFrame));
            last = std::min(last, static_cast<long>(lastFrame));
        }
        return last >= first;
    }
    // Fusion reports a clip's range as e.g. [1001, 1023.9999]: frames 1001 to
    // 1023. Rounding the end would ask for 1024, which the clip doesn't have.
    first = static_cast<long>(std::ceil(clipFirst - 1e-6));
    last = static_cast<long>(std::floor(clipLast + 1e-6));
    if (hostKnown) {
        // Resolve can report the timeline's end one frame short when the clip's
        // frame rate differs from the timeline's (a 29.97 clip on a 23.976
        // timeline: comp 1001-1094, bounds 1001-1093). Allow one frame past it,
        // as long as the clip has that frame.
        first = std::max(first, std::lround(hostFirst));
        last = std::min(last, static_cast<long>(std::floor(hostLast + 1e-6)) + 1);
    }
    if (useFirstLast) {
        first = std::max(first, static_cast<long>(firstFrame));
        last = std::min(last, static_cast<long>(lastFrame));
    }
    return last >= first;
}

// Pulls a field out of one of the worker's JSON event lines. Enough for the
// flat objects the worker writes; returns "" when missing.
inline std::string jsonField(const std::string& line, const std::string& key) {
    std::string pat = "\"" + key + "\": ";
    size_t i = line.find(pat);
    if (i == std::string::npos) return "";
    i += pat.size();
    if (i < line.size() && line[i] == '"') {
        std::string out;
        for (size_t j = i + 1; j < line.size(); ++j) {
            if (line[j] == '\\' && j + 1 < line.size()) { out += line[++j]; continue; }
            if (line[j] == '"') break;
            out += line[j];
        }
        return out;
    }
    size_t j = line.find_first_of(",}", i);
    return line.substr(i, j == std::string::npos ? std::string::npos : j - i);
}

}  // namespace ck2
