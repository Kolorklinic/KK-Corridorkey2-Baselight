# CorridorKey 2 for Baselight — Linux + NVIDIA (experimental)

**Corridor Digital's AI green-screen keyer as an OpenFX plugin for FilmLight Baselight on Linux,** running [CorridorKey 2.0](https://github.com/nikopueringer/CorridorKey2.0) by Corridor Digital on an NVIDIA GPU. Hair, fur, motion blur and uneven screens key cleanly with no keyer settings, and a rough garbage matte on its Hint input removes anything outside the screen.

By KolorKlinic. Free and unofficial: not made or endorsed by Corridor Digital or FilmLight.

> [!WARNING]
> **Experimental: this plugin has not been tested in Baselight.** It is built and checked automatically on Linux (it compiles, its tests pass, it loads as an OFX plugin, and its Python packages install), but nobody has run it inside Baselight yet. Some of what it relies on — Baselight letting a plugin read every frame of a clip when you press **Process**, and starting a Python process from the plugin — comes from FilmLight's published notes, not from a test. If you try it, please [open an issue](../../issues) with what happened, your Baselight version and the logs (see *Troubleshooting*).

![A man in front of a green screen, and the same frame keyed by CorridorKey 2 over a checkerboard](corridorkey2_example.jpg)

## What you need

- **Linux x86_64**, RHEL/Rocky 8 or later (glibc 2.28+), with a desktop session (Install Model opens a terminal window; Process shows a progress window).
- **An NVIDIA GPU** with an NVIDIA driver 570 or later (`nvidia-smi` shows it). CorridorKey runs with PyTorch 2.9.1 for CUDA 12.8, the versions Corridor Digital tests it with. GPU memory sets how many frames are keyed together; 12 GB or more is a sensible minimum at HD.
- **Baselight** with OFX support. Which versions work is unknown so far.
- **About 15 GB of disk** for CorridorKey, its weights and its Python, plus about 2.5 MB for each processed HD frame.
- **Internet access once,** for Install Model.

## Install

1. Quit Baselight.
2. Download `CorridorKey2-Baselight-…-linux-x86_64.tar.gz` from [Releases](../../releases) and unpack it:
   ```bash
   tar -xzf CorridorKey2-Baselight-*-linux-x86_64.tar.gz
   ```
3. Install the plugin into `/usr/OFX/Plugins` (asks for your password):
   ```bash
   cd CorridorKey2-Baselight-*-linux-x86_64 && ./install.sh
   ```
   On **Baselight TWO/X**, do this on the node; FilmLight shares `/usr/OFX/Plugins` with the UI host.
4. Start Baselight, add **CorridorKey 2** (in the **KolorKlinic** group) as an OFX effect, open its **Setup** section and press **Install Model**. A terminal window asks you to accept the CorridorKey licence, then downloads CorridorKey 2.0's code from Corridor's GitHub and its weights from Hugging Face (about 5 GB), checks every file against the tested version, and loads the model once on the GPU. If no terminal opens, run it yourself:
   ```bash
   bash /usr/OFX/Plugins/CorridorKey2.ofx.bundle/Contents/Resources/setup/install_model.sh
   ```

Install Model puts everything in `~/CorridorKey` of the user who runs it; do it as the user who runs Baselight.

## Using it

Add CorridorKey 2 to your shot and press **Process**. A progress window shows the frame, the chunk and the time left; Status says the same. The effect then draws from the stored results, so playback is instant and **Output** and **Despill** change live.

- **Range:** Baselight may not tell plugins how long a clip is. If Process says so, set **Range > Frames** to **First to Last** and enter the frames.
- **Input Is:** the plugin asks Baselight for Rec.709 (display) pictures, which is what **Rec.709** expects. If the key looks wrong, try **sRGB Linear**.
- **Temporal Chunks:** how many frames CorridorKey keys together. Auto picks the most that fits in the GPU memory Baselight leaves free, using CorridorKey's own NVIDIA figures; a chunk too long for the memory runs in CorridorKey's low-memory mode (same result, slower).
- **Output:** Premultiplied, Matte Only, Unpremultiplied, or Source + Matte (Unpremultiplied).
- **Hint:** a rough matte of the subject on the Hint input, or in the source's alpha. At a strength of 0.3 to 0.5 it removes anything outside it and keeps CorridorKey's own edges.
- **Frames that aren't processed** show the picture with a red frame and **NOT PROCESSED - PRESS PROCESS**; a yellow frame and **PICTURE CHANGED - PRESS PROCESS AGAIN** mean the picture changed since Process.

### Baselight TWO/X (separate node)

Baselight renders on the node, while **Process** runs where you press it. The node must be able to read the stored results: set the effect's **Cache Folder** to a folder both machines see at the same path (for example on shared storage), or set `"cache_folder"` in `~/.config/cvt-corridorkey2/settings.json`. Process also needs the NVIDIA GPU and `~/CorridorKey` on the machine where it runs.

## Known unknowns

These are the things a first test in Baselight will answer:

- whether Baselight lets the plugin read all the clip's frames when **Process** is pressed, and start the worker process;
- whether Status and the redraw after Process update while Baselight is open;
- whether CorridorKey gets enough GPU memory while Baselight holds its own (the worker uses what is free when the job starts);
- how Baselight handles the colour space the plugin asks for.

## Troubleshooting

Logs are in `~/.cache/cvt-corridorkey2/logs/` (`plugin.log` and `worker.log`). To check the model on its own:

```bash
~/CorridorKey/.venv/bin/python /usr/OFX/Plugins/CorridorKey2.ofx.bundle/Contents/Resources/worker/ck2_worker.py check ~/CorridorKey/CorridorKey2.0
```

## What's where

| | |
|---|---|
| `/usr/OFX/Plugins/CorridorKey2.ofx.bundle` | the plugin |
| `~/CorridorKey` | CorridorKey 2.0, its weights and its Python (Install Model) |
| `~/.cache/cvt-corridorkey2` | stored results and logs (the effect's Cache Folder can move the results) |

To uninstall, delete those three.

## Building from source

GitHub Actions builds every push inside Rocky Linux 8 (see `.github/workflows/build.yml`), and a `v*` tag publishes a release. To build by hand on Linux you need a C++17 compiler (GCC 12 on Rocky 8: `gcc-toolset-12`), cmake 3.20+, curl and [uv](https://github.com/astral-sh/uv); `./build.sh` builds OpenEXR and Imath once (pinned and checksummed), then the plugin into `build/CorridorKey2.ofx.bundle`, and runs the tests.

## Credits and licence

Based on [CorridorKey 2 for Resolve](https://github.com/creativevideotips/cvt-corridorkey2) by Creative Video Tips, ported to Linux, NVIDIA and Baselight. The CorridorKey keyer and its weights are by Corridor Digital.

Licensed under CC BY-NC-SA 4.0 with the CorridorKey additional terms; see [LICENSE](LICENSE). It is free and must stay free. CorridorKey 2.0 and its weights are downloaded from Corridor Digital's own pages under their licence. Organisations with more than US $20 million in annual revenue need a CorridorKey seat for each user from [corridorkey.com](https://corridorkey.com). The components included in the plugin, and the models CorridorKey uses, are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
