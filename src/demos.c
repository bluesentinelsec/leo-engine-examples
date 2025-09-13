#include "demos.h"

// Forward declare your demo entrypoints
// Note that 'oneFrame' means the demo
// will run for 1 frame then exit.
// This is intended to provide an escape
// hatch so we can test the demos in CICD
bool BasicDemo(bool oneFrame);
bool ResizableDemo(bool oneFrame);
bool FullscreenDemo(bool oneFrame);
bool ImageDemo(bool oneFrame);
bool GraphicsDemo(bool oneFrame);
bool AudioDemo(bool oneFrame);
bool VFSDemo(bool oneFrame);
bool FontDemo(bool oneFrame);

/* Register demos here */
Demo gDemos[] = {
    {"Basic Demo", "Trivial print-only demo", BasicDemo},
    {"Resizable Window", "How to create a resizable window", ResizableDemo},
    {"Fullscreen Window", "How to create a fullscreen window", FullscreenDemo},
    {"Render Image", "How to load and render an image from file", ImageDemo},
    {"Graphics Demo", "How to work with Leo Engine drawing primitives", GraphicsDemo},
    {"Audio Demo", "How to work with Leo Engine music and sound primitives", AudioDemo},
    {"VFS Demo", "How to work with Leo Engine virtual filesystem", VFSDemo},
    {"Font Demo", "Font rendering", FontDemo},
};

int gNumDemos = sizeof(gDemos) / sizeof(gDemos[0]);
