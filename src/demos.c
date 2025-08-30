#include "demos.h"

/* Forward declare your demo entrypoints */
bool RunDemo_Basic(bool one_frame);

/* Register demos here */
Demo g_demos[] = {
    {"Basic Demo", "Trivial print-only demo", RunDemo_Basic},
};

int g_num_demos = sizeof(g_demos) / sizeof(g_demos[0]);
