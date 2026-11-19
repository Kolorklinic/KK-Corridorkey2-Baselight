// ---------------------------------------------------------------------------
// CorridorKey 2 for Resolve - EXR reading and writing (OpenEXR)
//
// From Creative Video Tips (CVT)  |  https://creativevideotips.com
// Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
// Uses OpenEXR and Imath (BSD-3-Clause), built into the plugin.
// ---------------------------------------------------------------------------
#include "ck2_exr.h"

#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfThreading.h>
#include <Imath/half.h>

#include <cstddef>
#include <cstdio>
#include <exception>
#include <mutex>

#include "ck2_core.h"

namespace ck2 {

namespace {
// OpenEXR decodes and encodes DWAB blocks on its own thread pool, but only
// once it is given threads (it has none by default): a 4K frame reads in about
// 14 ms instead of 84.
void useThreads() {
    static std::once_flag once;
    std::call_once(once, [] { Imf::setGlobalThreadCount(workerCount()); });
}
}  // namespace

bool writeExr(const std::string& path, const float* rgba, int width, int height,
              long rowStride, std::string& error, bool withAlpha) {
    if (width <= 0 || height <= 0) { error = "empty image"; return false; }
    useThreads();
    const std::string tmp = path + ".part.exr";
    try {
        const int nch = withAlpha ? 4 : 3;
        std::vector<half> px(static_cast<size_t>(width) * height * nch);
        for (int y = 0; y < height; ++y) {
            const float* row = rgba + rowStride * y;
            half* out = &px[static_cast<size_t>(y) * width * nch];
            for (int x = 0; x < width; ++x)
                for (int c = 0; c < nch; ++c) {
                    float v = row[x * 4 + c];
                    v = v > 65504.0f ? 65504.0f : (v < -65504.0f ? -65504.0f : v);
                    out[x * nch + c] = half(v);
                }
        }
        Imf::Header header(width, height);
        header.compression() = Imf::DWAB_COMPRESSION;
        header.dwaCompressionLevel() = kDwaLevel;
        Imf::FrameBuffer fb;
        const char* names[] = {"R", "G", "B", "A"};
        const size_t xs = sizeof(half) * nch, ys = xs * width;
        for (int c = 0; c < nch; ++c) {
            header.channels().insert(names[c], Imf::Channel(Imf::HALF));
            fb.insert(names[c], Imf::Slice(Imf::HALF, reinterpret_cast<char*>(px.data() + c), xs, ys));
        }
        {
            Imf::OutputFile file(tmp.c_str(), header);
            file.setFrameBuffer(fb);
            file.writePixels(height);
        }
        if (std::rename(tmp.c_str(), path.c_str()) != 0) {
            std::remove(tmp.c_str());
            error = "could not rename " + tmp;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::remove(tmp.c_str());
        error = std::string("EXR write failed: ") + e.what();
        return false;
    }
}

bool readExrRGBA(const std::string& path, ImageRGBA& out, std::string& error) {
    try {
        useThreads();
        Imf::InputFile file(path.c_str());
        const Imath::Box2i dw = file.header().dataWindow();
        const int w = dw.max.x - dw.min.x + 1, h = dw.max.y - dw.min.y + 1;
        if (w <= 0 || h <= 0) { error = "empty EXR"; return false; }
        out.width = w;
        out.height = h;
        // Reused between frames (no 140 MB clear per 4K frame); channels a
        // file lacks are filled by the slices' fill values below.
        if (out.pixels.size() != static_cast<size_t>(w) * h * 4)
            out.pixels.resize(static_cast<size_t>(w) * h * 4);
        const size_t xs = sizeof(float) * 4, ys = xs * w;
        // Slices are addressed by data-window coordinates.
        char* base = reinterpret_cast<char*>(out.pixels.data()) -
                     static_cast<std::ptrdiff_t>(dw.min.x) * static_cast<std::ptrdiff_t>(xs) -
                     static_cast<std::ptrdiff_t>(dw.min.y) * static_cast<std::ptrdiff_t>(ys);
        Imf::FrameBuffer fb;
        const char* names[] = {"R", "G", "B", "A"};
        for (int c = 0; c < 4; ++c)
            fb.insert(names[c], Imf::Slice(Imf::FLOAT, base + c * sizeof(float), xs, ys, 1, 1,
                                           c == 3 ? 1.0 : 0.0));
        file.setFrameBuffer(fb);
        file.readPixels(dw.min.y, dw.max.y);
        return true;
    } catch (const std::exception& e) {
        error = std::string("EXR read failed: ") + e.what();
        return false;
    }
}

}  // namespace ck2
