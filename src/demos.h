#pragma once
#include <stdbool.h>

/* Metadata + entrypoint for a demo */
typedef struct Demo {
  const char *name;
  const char *description;
  bool (*run)(bool one_frame_only);
} Demo;

/* Registry (defined in demos.c) */
extern Demo g_demos[];
extern int g_num_demos;
