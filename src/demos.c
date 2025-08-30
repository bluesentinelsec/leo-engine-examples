#include "demos.h"

// Forward declare your demo entrypoints
// Note that 'oneFrame' means the demo
// will run for 1 frame then exit.
// This is intended to provide an escape
// hatch so we can test the demos in CICD
bool BasicDemo(bool oneFrame);
bool ResizableDemo(bool oneFrame);

/* Register demos here */
Demo gDemos[] = {
    {"Basic Demo", "Trivial print-only demo", BasicDemo},
    {"Resizable Window", "How to create a resizable window", ResizableDemo},
};

int gNumDemos = sizeof(gDemos) / sizeof(gDemos[0]);
