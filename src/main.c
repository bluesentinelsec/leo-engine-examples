#include <leo/leo.h>
#include <getopt/getopt.h>

int main(int argc, char *argv[])
{
    if (!leo_InitWindow(1280, 740, "Test"))
    {
        return 1;
    }

    leo_CloseWindow();

    return 0;
}
