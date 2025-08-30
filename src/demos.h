#pragma once
#include <stdbool.h>

/* Metadata + entrypoint for a demo */
typedef struct Demo
{
    const char *name;
    const char *description;
    bool (*run)(bool oneFrameOnly);
} Demo;

/* Registry (defined in demos.c) */
extern Demo gDemos[];
extern int gNumDemos;
