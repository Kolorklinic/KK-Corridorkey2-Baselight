# Third-party notices

CorridorKey 2 for Resolve is by Creative Video Tips, licensed under CC BY-NC-SA 4.0 with the CorridorKey additional terms (see `LICENSE`). It is an unofficial plugin; CorridorKey 2.0 is by Corridor Digital LLC.

## Included in the plugin

Each of these keeps its own licence. Their licence files are inside the plugin, in `CorridorKey2.ofx.bundle/Contents/Resources`.

| Component | Use | Licence |
|---|---|---|
| [OpenFX](https://github.com/AcademySoftwareFoundation/openfx) 1.5.1 headers and support library | the OFX plugin interface | BSD 3-Clause |
| [OpenEXR](https://github.com/AcademySoftwareFoundation/openexr) 3.5.2 | reading and writing EXR files | BSD 3-Clause |
| [Imath](https://github.com/AcademySoftwareFoundation/Imath) 3.2.3 | used by OpenEXR | BSD 3-Clause |
| [OpenJPH](https://github.com/aous72/OpenJPH) (bundled with OpenEXR) | used by OpenEXR | BSD 2-Clause |
| [libdeflate](https://github.com/ebiggers/libdeflate) (bundled with OpenEXR) | used by OpenEXR | MIT |
| [uv](https://github.com/astral-sh/uv) | installs the plugin's private Python during Install Model | MIT or Apache 2.0 |
| [utils3d](https://github.com/EasternJournalist/utils3d) 1.7 (wheel) | a dependency of CorridorKey 2.0 | MIT |

## Downloaded by Install Model, not included

Install Model downloads these onto the user's computer from their publishers' own pages, after the user accepts the CorridorKey licence. They are not part of this repository or the plugin package, and each stays under its own licence.

| Component | From | Licence |
|---|---|---|
| CorridorKey 2.0 code and weights, by Corridor Digital LLC | [GitHub](https://github.com/nikopueringer/CorridorKey2.0), [Hugging Face](https://huggingface.co/nikopueringer/CorridorKey2.0) | CorridorKey Licence 2.0 (CC BY-NC-SA 4.0 with additional terms) |
| C-RADIOv4-SO400M, by NVIDIA | [Hugging Face](https://huggingface.co/nvidia/C-RADIOv4-SO400M) | NVIDIA Open Model License. Licensed by NVIDIA Corporation under the NVIDIA Open Model License. |
| MoGe-2 ViT-B, by Microsoft | [Hugging Face](https://huggingface.co/Ruicheng/moge-2-vitb-normal) | MIT |
| RVM small, by Google DeepMind | Google Cloud Storage | CC BY 4.0 (weights) |
| Python 3.12 and packages such as PyTorch (CUDA 12.8 build) | uv's Python builds, PyPI and download.pytorch.org | their own licences |
