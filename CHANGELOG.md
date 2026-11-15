# CorridorKey 2 for Baselight: changelog

- 1.01 (2026-11-15): the worker also writes its plan (GPU memory free,
  frames per chunk, low-VRAM mode), its result and any error to
  worker.log, to show why processing is slow. First user test: the plugin
  loads and appears in Baselight.
- 1.00 (2026-11-15): first experimental release for Baselight on Linux
  with NVIDIA GPUs: Linux build, CorridorKey on CUDA (PyTorch 2.9.1 for
  CUDA 12.8), frame range without a clip length, Rec.709 from Baselight.

Earlier versions: CorridorKey 2 for Resolve by Creative Video Tips.


- 1.00 (2026-11-13): first public release. Delete Cache says "Nothing to
  delete" when the node has no stored results, instead of reporting that it
  deleted them.
- 0.30 (2026-11-13): ready for a public repository. The plugin no longer
  carries the builder's home folder in its compiled source paths (they read
  "~"). LICENSE has the full CorridorKey Licence and CC BY-NC-SA 4.0 text, and
  THIRD_PARTY_NOTICES.md lists the included components and the models
  Install Model downloads; both ship inside the plugin.
- 0.29 (2026-11-13): security fixes before going public, so a comp from
  someone else can't do harm. The node ID (saved in the comp) must be 16 hex
  digits before it is used in a folder name, so a crafted comp can't make
  Process or Delete Cache delete a folder outside the cache. The plugin only
  changes permissions on folders it creates, never on the Cache Folder or
  anything that already existed, and the cache limit only removes its own
  node folders. Python and Model Folder always come from this Mac
  (settings.json or Install Model's folders) and are no longer saved in
  comps, so a comp can't make Process run another program. The frames handed
  to the worker get the same permissions as the results.
- 0.28 (2026-11-13): on the Edit page, the Done status ends with "move the
  playhead to see it": the Edit page viewer doesn't redraw by itself after
  Process.
- 0.27 (2026-11-13): clearer Output names: Foreground + Matte Alpha is now
  Unpremultiplied, and Source + Matte Alpha is Source + Matte
  (Unpremultiplied). Same positions in the list, so existing nodes keep
  their output. The tooltip says when to use each.
- 0.26 (2026-11-13): no flash in the top-left corner when Process starts:
  the progress window stays hidden until it is built and centred (Tk had
  drawn it in the corner for a moment first).
- 0.25 (2026-11-13): the installer's licence page wraps as paragraphs, like
  the Read Me (it showed LICENSE's hard line breaks). It is made from LICENSE
  when the package is built (installer/make_license_html.py).
- 0.24 (2026-11-13): the last frame is keyed when the clip's frame rate
  differs from the timeline's (Resolve reported the comp one frame short, so
  sh280 at 29.97 on a 23.976 timeline missed frame 1094 on the Fusion page).
  Switching Frames to First to Last fills in the shot's own first and last
  frame and sets the sliders to that range; the two fields are greyed out
  while Frames is Whole Shot. The installer's Read Me is laid out as proper
  paragraphs. Tested by Chadwick before this version: ACEScg input, and the
  M1 Max and M2 Max (Install Model and Process on sh280).
- 0.23 (2026-11-13): the effect shows the Creative Video Tips logo in the
  Effects library, like LTX Alpha Gen and LTX Inpaint Still. The version is in
  the Setup section's title ("Setup · Version 0.23"); the Version and About
  lines are gone, because Resolve shows a label line's name on both sides
  and never its text. The credit is in the Setup tooltip, the progress window
  and the installer.
- 0.22 (2026-11-13): Setup shows Version and About once each ("Version
  0.22", "About OFX by Creative Video Tips, model by Corridor Digital"); 0.21
  showed each text twice. The progress window's Cancel button is light text
  on dark grey, easy to read (macOS had drawn it white with light text).
- 0.21 (2026-11-13): Status and the progress window always show the
  temporal chunks: "Starting: 93 frames in 2 chunks of 80", "Keying frame 12
  of 93 (chunk 1 of 2)", "Done: 93 frames in 88 s (2 chunks of 80 frames)".
  Status is saved with the comp, so the last result is still there after
  reopening. Setup: Install Model only shows while CorridorKey isn't
  installed; Version and the About line sit together at the bottom.
- 0.20 (2026-11-13): a clearer Inspector order: Input Is and Temporal Chunks
  first, then Process and Status, then Output, Hint, Range, Cache and Setup.
  Refresh moved into Cache, beside Delete Cache (was Clear Results) and the
  new Delete All Cache (press twice within 5 seconds). Version and About are
  plain grey text, not boxes. A Cache Folder outside the home folder (a media
  drive, a shared network folder) is now readable by other Macs and users, so
  they can use the results; inside the home folder it stays private.
- 0.19 (2026-11-13): Status, Version and About now show their text: Resolve
  and Fusion draw read-only label lines with their name, so they are
  greyed-out text fields (as in LTX Alpha Gen). About reads "OFX by Creative
  Video Tips, model by Corridor Digital". Window is now Temporal Chunks, in
  the Output section. Precision is no longer shown: each Mac gets the right
  one by itself.
- 0.18 (2026-11-13): when a processed frame is drawn as stripes, plugin.log
  says why (result missing or unreadable, a different picture size, or how
  much the picture changed), to find a one-frame pop Chadwick saw during
  playback in Fusion Studio.
- 0.17 (2026-11-13): Process never asks for a frame past the clip's end:
  Fusion reports a 23-frame clip as frames 1001 to 1023.9999, which was
  rounded up to 1024. When the clip and the comp don't overlap, Status says
  which frames each has. Edit page and Fusion Studio tested on 0.16 (Fusion
  Studio: Loader at 1001-1023, 23 frames read in 2 s, Done in 21.8 s).
- 0.16 (2026-11-13): a tidier Inspector: Input Is sits near the top on its
  own (no one-control section); Window and Precision (was Speed and Quality)
  moved into Setup, which now starts with the plugin's Version. The progress
  window's footer says "OFX by Creative Video Tips", so it's clear the model
  is Corridor Digital's and only the plugin is ours. 0.15's progress window
  tested in Resolve (sh280, 93 frames, 88 s).
- 0.15 (2026-11-13): a progress window for every Process: the stage, a
  progress bar, the time left and Cancel, in the CVT colours; it shows the
  result and closes itself. Input Is: the first choice is now just "Rec.709".
  0.14's Hint input tested in Resolve: a garbage box at strength 0.5 removed
  the stands and lights on sh280.
- 0.14 (2026-11-13): the Hint input is back on the Fusion page (General
  context), now that the Process freeze is understood. Hint From: None, Hint
  Input or Source Alpha.
- 0.13 (2026-11-13): Process changes nothing on the node (Status, node ID)
  until every frame has been read, as LTX Alpha Gen does. Changing a
  parameter during the button press made Fusion defer a notification and the
  frame read wait forever. A new node gets its ID when it is created. Still
  Filter context only.
- 0.12 (2026-11-13): Process covers only frames the source clip has (its own
  range, within the timeline bounds), as LTX Alpha Gen does. Asking Resolve's
  Fusion page for a frame outside the clip never returns: the likely cause of
  every Process freeze so far. The ranges and the first frame read are logged
  in ~/.cache/cvt-corridorkey2/logs/plugin.log. Still Filter context only.
- 0.11 (2026-11-13): test build for the Process freeze: the node now offers
  only the Filter context, like LTX Alpha Gen, so it has no Hint input. Hint
  From is None or Source Alpha (put Magic Mask or a roto in the alpha before
  the node).
- 0.10 (2026-11-13): no control can be keyframed any more (Despill and the
  text fields could). The installer shows the Creative Video Tips logo in its
  lower-left corner (white in Dark Mode), like LTX Alpha Gen's.
- 0.09 (2026-11-13): every EXR is half float with DWAB compression at level
  45 (the frames handed to the worker and the stored results; lossless for
  the matte), read and written with OpenEXR built into the plugin. Cache
  Folder setting; new nodes take it from ~/.config/cvt-corridorkey2/settings.json
  (for example a fast media drive). Input Is: Display,
  sRGB Linear (was Scene Linear) or ACEScg, for CG renders: converted to linear
  sRGB for CorridorKey and back, so the output stays ACEScg. Output Keyed is
  now called Premultiplied.
- 0.08 (2026-11-13): fixes Process freezing the Fusion page (0.07's progress
  bar did not). The node now declares temporal clip access for its inputs:
  without it, Fusion never delivers a frame other than the one being
  rendered, so reading the shot from the Process button waited forever.
- 0.07 (2026-11-13): fixes the Fusion page freezing when Process is pressed.
  The node asked Fusion for frames from the button press with no progress
  bar, and Fusion waited for Resolve's main thread while the main thread
  waited for the frame. Frames are now read with the host's progress bar up
  (with Cancel), the last frame first. The folders in ~/.cache/cvt-corridorkey2
  are now private to you (Resolve had made them writable by everyone).
- 0.06 (2026-11-13): fixes Install Model stopping at "package install
  failed": one file in the plugin (the utils3d package) could only be read by
  its owner, which the installer makes root. Every file in the plugin is now
  readable, and the build checks it.
- 0.05 (2026-11-13): Setup > Install Model opens Terminal and sets up
  CorridorKey 2.0 in ~/CorridorKey: asks you to accept Corridor's licence,
  downloads the code at the tested commit and checks every file, builds a
  private Python 3.12 with pinned packages (uv comes with the plugin),
  downloads the weights (about 5 GB) and checks them against the tested ones,
  then loads the model once. Running it again repairs or skips. The node's
  Setup now points at ~/CorridorKey, and Process says when the model isn't
  installed.
  Installer: package.sh makes a signed .pkg (Apple Silicon only, with a
  Read Me) for notarization.
- 0.04 (2026-11-13): ready for the Edit page. Range: Whole Shot or First to
  Last (Edit page frames count from 0). Each processed frame keeps a small
  thumbnail; if the picture coming in no longer matches (a slipped clip, a
  new grade upstream), the frame shows the source in stripes with a yellow
  frame instead of a wrong matte. A copied node gets its own results when it
  is processed. Cache: Cache Limit (GB, default 100; oldest results removed
  first) and Clear Results. The frames handed to the worker are half float
  and are removed when the job ends.
- 0.03 (2026-11-13): Hint. An optional Hint input on the Fusion page (or the
  source's own alpha) gives CorridorKey a rough matte of the subject: Magic
  Mask, roto, even a garbage box. Hint Strength defaults to 0.5: at 0.3 to
  0.5 it removes what's outside the hint and keeps CorridorKey's own edges
  inside (tested on sh280: stands and lights gone, edges unchanged); near 1
  the result follows the hint's edges. Hint Frames: Every Frame or First
  Frame Only. The job file is now written by tested code.
- 0.02 (2026-11-13): the first OFX node (Fusion page and Fusion Studio).
  Process hands the shot's frames to the worker and Status follows it frame
  by frame; press Process again to stop. The node draws from the stored
  results with Output (Keyed, Matte Only, Foreground + Matte Alpha, Source +
  Matte Alpha), Despill and Screen changing live; frames not yet processed
  show the source dimmed in stripes with a red frame. Input Is, Window and
  Precision. The cache is now uncompressed EXR (read by the plugin's own small
  reader).
- 0.01 (2026-11-13): the worker. Reads a job file and a folder of frames,
  runs CorridorKey 2.0 on the Apple GPU (window size and precision chosen for
  each Mac), and writes one RGBA EXR per frame to the cache with progress
  reported as it goes. Tested on two green-screen shots against the earlier
  Mac test.
