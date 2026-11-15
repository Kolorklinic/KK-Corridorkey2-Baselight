// ---------------------------------------------------------------------------
// CorridorKey 2 for Resolve - tests for the host-free parts
//
// From Creative Video Tips (CVT)  |  https://creativevideotips.com
// Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
//
//   ck2_tests                     unit tests
//   ck2_tests write <out.exr>     writes a 7x5 test pattern (for the Python check)
//   ck2_tests read <in.exr>       prints size and a few pixels of an EXR
// ---------------------------------------------------------------------------
#include "ck2_core.h"
#include "ck2_exr.h"
#include "ck2_job.h"

#include <cmath>
#include <limits>
#include <condition_variable>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

static bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

static std::vector<float> pattern(int w, int h) {
    std::vector<float> px(static_cast<size_t>(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float* p = &px[(static_cast<size_t>(y) * w + x) * 4];
            p[0] = x * 0.1f; p[1] = y * 0.2f; p[2] = 1.5f + x - y; p[3] = 1.0f;
        }
    return px;
}

static void unitTests() {
    // Despill: neutral grey untouched, green spill removed, brightness kept.
    float r = 0.5f, g = 0.5f, b = 0.5f;
    ck2::despill(r, g, b, 1.0f, ck2::Screen::Green);
    CHECK(near(r, 0.5f) && near(g, 0.5f) && near(b, 0.5f));
    r = 0.2f; g = 0.8f; b = 0.2f;
    ck2::despill(r, g, b, 1.0f, ck2::Screen::Green);
    CHECK(near(g, 0.2f) && near(r, 0.5f) && near(b, 0.5f));
    CHECK(near(r + g + b, 1.2f));
    r = 0.2f; g = 0.8f; b = 0.2f;
    ck2::despill(r, g, b, 0.5f, ck2::Screen::Green);
    CHECK(near(g, 0.5f));
    r = 0.1f; g = 0.1f; b = 0.9f;
    ck2::despill(r, g, b, 1.0f, ck2::Screen::Blue);
    CHECK(near(b, 0.1f) && near(r, 0.5f) && near(g, 0.5f));

    // sRGB encode matches the standard curve.
    CHECK(near(ck2::srgbEncode(0.0f), 0.0f));
    CHECK(near(ck2::srgbEncode(1.0f), 1.0f));
    CHECK(near(ck2::srgbEncode(0.18f), 0.46135f, 1e-4f));

    // Compose.
    ck2::Look look;
    look.despill = 0.0f;
    float fg[3] = {0.18f, 0.18f, 0.18f}, src[4] = {0.3f, 0.4f, 0.5f, 1.0f}, out[4];
    ck2::composePixel(look, fg, 0.5f, src, out);
    CHECK(near(out[0], 0.46135f * 0.5f, 1e-4f) && near(out[3], 0.5f));
    look.output = ck2::Output::MatteOnly;
    ck2::composePixel(look, fg, 0.25f, src, out);
    CHECK(near(out[0], 0.25f) && near(out[3], 1.0f));
    look.output = ck2::Output::SourceWithMatte;
    ck2::composePixel(look, fg, 0.25f, src, out);
    CHECK(near(out[1], 0.4f) && near(out[3], 0.25f));
    look.output = ck2::Output::Foreground;
    look.input = ck2::InputSpace::SrgbLinear;
    ck2::composePixel(look, fg, 0.25f, src, out);
    CHECK(near(out[0], 0.18f) && near(out[3], 0.25f));
    // ACEScg: neutral grey stays grey (both whites map to each other), and the
    // matrices undo each other.
    look.input = ck2::InputSpace::ACEScg;
    ck2::composePixel(look, fg, 1.0f, src, out);
    CHECK(near(out[0], 0.18f, 1e-3f) && near(out[1], 0.18f, 1e-3f) && near(out[2], 0.18f, 1e-3f));
    float mr = 0.7f, mg = 0.2f, mb = 0.05f;
    ck2::applyMatrix(ck2::kAcescgToSrgb, mr, mg, mb);
    ck2::applyMatrix(ck2::kSrgbToAcescg, mr, mg, mb);
    CHECK(near(mr, 0.7f, 1e-4f) && near(mg, 0.2f, 1e-4f) && near(mb, 0.05f, 1e-4f));
    look.input = ck2::InputSpace::Display;

    // Names and JSON.
    CHECK(ck2::cacheFileName(12, 0xabcdef01u) == "f0000012_abcdef01.exr");
    using I = ck2::InputSpace;
    CHECK(ck2::settingsHash(I::Display, 0, 0) != ck2::settingsHash(I::SrgbLinear, 0, 0));
    CHECK(ck2::settingsHash(I::SrgbLinear, 0, 0) != ck2::settingsHash(I::ACEScg, 0, 0));
    CHECK(ck2::settingsHash(I::Display, 16, 0) != ck2::settingsHash(I::Display, 24, 0));
    using H = ck2::HintFrom;
    CHECK(ck2::settingsHash(I::Display, 0, 0, H::None, true, 0.5) == ck2::settingsHash(I::Display, 0, 0, H::None, false, 0.9));
    CHECK(ck2::settingsHash(I::Display, 0, 0, H::HintInput, true, 0.5) != ck2::settingsHash(I::Display, 0, 0, H::None));
    CHECK(ck2::settingsHash(I::Display, 0, 0, H::HintInput, true, 0.5) != ck2::settingsHash(I::Display, 0, 0, H::HintInput, false, 0.5));
    CHECK(ck2::settingsHash(I::Display, 0, 0, H::HintInput, true, 0.5) != ck2::settingsHash(I::Display, 0, 0, H::HintInput, true, 0.3));
    CHECK(ck2::settingsHash(I::Display, 0, 0, H::HintInput, true, 0.5) != ck2::settingsHash(I::Display, 0, 0, H::SourceAlpha, true, 0.5));
    std::string line = R"({"stage": "keying", "done": 12, "total": 84, "event": "progress", "t": 1.5})";
    CHECK(ck2::jsonField(line, "event") == "progress");
    CHECK(ck2::jsonField(line, "done") == "12");
    CHECK(ck2::jsonField(line, "t") == "1.5");
    CHECK(ck2::jsonField(R"({"message": "a \"b\"", "event": "error"})", "message") == "a \"b\"");

    // EXR round trip: DWAB 45 is lossy for colour (small error), exact for alpha.
    const int w = 96, h = 64;
    std::vector<float> px(static_cast<size_t>(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float* p = &px[(static_cast<size_t>(y) * w + x) * 4];
            p[0] = 0.1f + 0.8f * x / w; p[1] = 0.2f + 0.6f * y / h; p[2] = 1.5f - x * 0.01f;
            p[3] = ((x / 8 + y / 8) % 3) / 2.0f;                       // a matte with hard steps
        }
    std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/ck2_rt.exr";
    std::string err;
    auto meanColourError = [&](const ck2::ImageRGBA& im) {
        double d = 0;
        for (size_t i = 0; i < px.size(); ++i) if (i % 4 != 3) d += std::fabs(im.pixels[i] - px[i]);
        return d / (px.size() * 3 / 4);
    };
    CHECK(ck2::writeExr(path, px.data(), w, h, w * 4, err));
    ck2::ImageRGBA img;
    CHECK(ck2::readExrRGBA(path, img, err));
    CHECK(img.width == w && img.height == h);
    CHECK(meanColourError(img) < 0.01);
    CHECK(img.pixels[3] == 1.0f);                                     // no alpha written: 1
    CHECK(ck2::writeExr(path, px.data(), w, h, w * 4, err, true));
    CHECK(ck2::readExrRGBA(path, img, err));
    bool alphaExact = true;
    for (size_t i = 3; i < px.size(); i += 4) alphaExact = alphaExact && img.pixels[i] == px[i];
    CHECK(alphaExact);
    CHECK(meanColourError(img) < 0.01);

    // Bottom-up source (negative stride) ends up top row first.
    std::vector<float> flipped(px.size());
    for (int y = 0; y < h; ++y)
        std::copy(&px[static_cast<size_t>(y) * w * 4], &px[static_cast<size_t>(y + 1) * w * 4],
                  &flipped[static_cast<size_t>(h - 1 - y) * w * 4]);
    CHECK(ck2::writeExr(path, &flipped[static_cast<size_t>(h - 1) * w * 4], w, h, -w * 4, err, true));
    CHECK(ck2::readExrRGBA(path, img, err));
    CHECK(meanColourError(img) < 0.01);
    std::remove(path.c_str());

    // Thumbnails: same picture ~0, a shifted picture clearly changed.
    std::vector<float> big(static_cast<size_t>(640) * 360 * 4);
    for (int y = 0; y < 360; ++y)
        for (int x = 0; x < 640; ++x) {
            float* p = &big[(static_cast<size_t>(y) * 640 + x) * 4];
            p[0] = ((x / 40 + y / 40) % 2) ? 0.9f : 0.1f; p[1] = x / 640.0f; p[2] = 0.5f; p[3] = 1;
        }
    auto t1 = ck2::makeThumb(big.data(), 640, 360, 640 * 4);
    CHECK(ck2::thumbDiff(t1, t1) == 0.0);
    std::vector<float> shifted(big.size());
    for (int y = 0; y < 360; ++y)
        for (int x = 0; x < 640; ++x)
            for (int c = 0; c < 4; ++c)
                shifted[(static_cast<size_t>(y) * 640 + x) * 4 + c] = big[(static_cast<size_t>(y) * 640 + (x + 40) % 640) * 4 + c];
    CHECK(ck2::thumbDiff(t1, ck2::makeThumb(shifted.data(), 640, 360, 640 * 4)) > ck2::kThumbChanged);
    std::vector<float> noisy = big;
    for (size_t i = 0; i < noisy.size(); i += 4) noisy[i] += ((i / 4) % 7) * 0.002f;   // decode noise
    CHECK(ck2::thumbDiff(t1, ck2::makeThumb(noisy.data(), 640, 360, 640 * 4)) < ck2::kThumbChanged);
    std::string tp = path + ".thumb";
    std::vector<float> back;
    CHECK(ck2::writeThumb(tp, t1) && ck2::readThumb(tp, back) && back == t1);
    std::remove(tp.c_str());

    // Range.
    long f0, f1;
    CHECK(ck2::processRange(0, 99, 0, 99, false, 5, 10, f0, f1) && f0 == 0 && f1 == 99);
    CHECK(ck2::processRange(0, 99, 0, 99, true, 5, 10, f0, f1) && f0 == 5 && f1 == 10);
    CHECK(ck2::processRange(0, 99, 0, 99, true, -3, 200, f0, f1) && f0 == 0 && f1 == 99);
    CHECK(!ck2::processRange(0, 99, 0, 99, true, 50, 40, f0, f1));
    // Timeline bounds past the clip never reach beyond the clip's frames.
    CHECK(ck2::processRange(1001, 1023, 0, 86400, false, 0, 0, f0, f1) && f0 == 1001 && f1 == 1023);
    CHECK(ck2::processRange(1001, 1023, 1010, 1100, false, 0, 0, f0, f1) && f0 == 1010 && f1 == 1023);
    CHECK(ck2::processRange(0, 99, 10, 50, false, 0, 0, f0, f1) && f0 == 10 && f1 == 51);   // one frame of slack
    // Mixed frame rates: Resolve reported the timeline end one short (sh280).
    CHECK(ck2::processRange(1001, 1094.9999, 1001, 1093, false, 0, 0, f0, f1) && f0 == 1001 && f1 == 1094);
    // ... but never past the clip.
    CHECK(ck2::processRange(1001, 1023.9999, 1001, 1023, false, 0, 0, f0, f1) && f1 == 1023);
    // Fusion's fractional clip end: [1001, 1023.9999] is frames 1001 to 1023.
    CHECK(ck2::processRange(1001, 1023.9999, 0, 2000, false, 0, 0, f0, f1) && f0 == 1001 && f1 == 1023);
    CHECK(ck2::processRange(0, 22, 0, 22, false, 0, 0, f0, f1) && f0 == 0 && f1 == 22);
    // No timeline bounds (0, 0): the clip's frames.
    CHECK(ck2::processRange(1001, 1023, 0, 0, false, 0, 0, f0, f1) && f0 == 1001 && f1 == 1023);
    CHECK(!ck2::processRange(1001, 1023, 0, 500, false, 0, 0, f0, f1));
    // Baselight: no clip length (empty, infinite or huge range).
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(ck2::processRange(0, -1, 100, 199, false, 0, 0, f0, f1) && f0 == 100 && f1 == 199);
    CHECK(ck2::processRange(-inf, inf, 100, 199, true, 120, 150, f0, f1) && f0 == 120 && f1 == 150);
    CHECK(ck2::processRange(-1e9, 1e9, 0, 0, true, 10, 20, f0, f1) && f0 == 10 && f1 == 20);
    CHECK(!ck2::processRange(-inf, inf, 0, 0, false, 0, 0, f0, f1));   // nothing to go on
    CHECK(!ck2::processRange(0, -1, 0, 0, true, 20, 10, f0, f1));
}

// Runs a real job through ck2::Job, as the plugin does, and prints each update.
static int runJob(const char* python, const char* worker, const char* job, const char* log) {
    std::mutex m;
    std::condition_variable cv;
    bool finished = false, ok = false;
    std::string err, last;
    auto j = ck2::Job::start(python, worker, job, log, [&](const ck2::JobUpdate& u) {
        if (u.status != last) { std::printf("  %s\n", u.status.c_str()); last = u.status; }
        if (u.finished) { std::lock_guard<std::mutex> l(m); finished = true; ok = u.ok; cv.notify_all(); }
    }, err);
    if (!j) { std::printf("start failed: %s\n", err.c_str()); return 1; }
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [&] { return finished; });
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "jobjson") {      // for the Python check in build.sh
        ck2::JobSpec j;
        j.modelDir = "/m \"q\""; j.framesDir = "/f"; j.workDir = "/w"; j.outputDir = "/o";
        j.hintDir = "/h"; j.window = 24; j.precision = "fp16"; j.inputLinear = true;
        j.hintFrom = ck2::HintFrom::HintInput; j.hintEveryFrame = false; j.hintStrength = 0.3;
        j.outputNames = {"a.exr", "b.exr"};
        std::fputs(ck2::jobJson(j).c_str(), stdout);
        return 0;
    }
    if (argc == 6 && std::string(argv[1]) == "job") return runJob(argv[2], argv[3], argv[4], argv[5]);
    if (argc == 3 && std::string(argv[1]) == "write") {
        auto px = pattern(7, 5);
        std::string err;
        if (!ck2::writeExr(argv[2], px.data(), 7, 5, 7 * 4, err)) { std::puts(err.c_str()); return 1; }
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "read") {
        ck2::ImageRGBA img;
        std::string err;
        if (!ck2::readExrRGBA(argv[2], img, err)) { std::puts(err.c_str()); return 1; }
        std::printf("%d %d\n", img.width, img.height);
        for (int i : {0, img.width * img.height / 2, img.width * img.height - 1})
            std::printf("%d %.5f %.5f %.5f %.5f\n", i, img.pixels[i * 4], img.pixels[i * 4 + 1],
                        img.pixels[i * 4 + 2], img.pixels[i * 4 + 3]);
        return 0;
    }
    unitTests();
    std::printf(failures ? "%d test(s) FAILED\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
