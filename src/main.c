#include "demos.h"
#include <getopt/getopt.h>
#include <leo/leo.h>
#include <stdio.h>
#include <stdlib.h>

static void print_help(const char *program_name) {
  printf("Leo Engine Showcase — run example demos and games\n");
  printf("\n");
  printf("Usage:\n");
  printf("  %s [OPTIONS]\n", program_name);
  printf("\n");
  printf("Options:\n");
  printf("  -h, --help            Show this help text and exit\n");
  printf("  -l, --list-demos      List all available demos\n");
  printf("  -i, --index N         Run demo with index N\n");
  printf("  -a, --all             Run all demos sequentially\n");
  printf("  -c, --cicd            Run in CI/CD mode (1 frame only)\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s --list-demos\n", program_name);
  printf("  %s --index 0\n", program_name);
  printf("  %s --index 1 --cicd\n", program_name);
  printf("  %s --all --cicd\n", program_name);
  printf("\n");
}

int main(int argc, char *argv[]) {
  int option_index = 0;
  static struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"list-demos", no_argument, NULL, 'l'},
      {"index", required_argument, NULL, 'i'},
      {"cicd", no_argument, NULL, 'c'},
      {"all", no_argument, NULL, 'a'},
      {NULL, 0, NULL, 0}};

  bool show_help = false;
  bool list_demos = false;
  int selected_index = -1;
  bool cicd_mode = false;
  bool run_all = false;

  int c;
  while ((c = getopt_long(argc, argv, "hli:ca", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'h':
      show_help = true;
      break;
    case 'l':
      list_demos = true;
      break;
    case 'i':
      selected_index = atoi(optarg);
      break;
    case 'c':
      cicd_mode = true;
      break;
    case 'a':
      run_all = true;
      break;
    case '?':
      return 1;
    default:
      abort();
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Unknown arguments: ");
    while (optind < argc)
      fprintf(stderr, "%s ", argv[optind++]);
    fprintf(stderr, "\n");
    return 1;
  }

  int action_count =
      (list_demos ? 1 : 0) + (selected_index >= 0 ? 1 : 0) + (run_all ? 1 : 0);

  if (show_help || action_count == 0) {
    print_help(argv[0]);
    return 0;
  }
  if (action_count > 1) {
    fprintf(stderr, "Conflicting options: cannot combine --list-demos, "
                    "--index, and/or --all.\n");
    return 1;
  }

  if (list_demos) {
    printf("Available demos:\n");
    for (int i = 0; i < gNumDemos; i++) {
      printf("  %d: %s — %s\n", i, gDemos[i].name, gDemos[i].description);
    }
    return 0;
  }

  if (run_all) {
    printf("Running all demos%s...\n", cicd_mode ? " in CI/CD mode" : "");
    for (int i = 0; i < gNumDemos; i++) {
      printf("\n[%d] %s\n", i, gDemos[i].name);
      gDemos[i].run(cicd_mode);
    }
    return 0;
  }

  if (selected_index >= 0) {
    if (selected_index < 0 || selected_index >= gNumDemos) {
      fprintf(stderr, "Invalid demo index: %d (must be between 0 and %d).\n",
              selected_index, gNumDemos - 1);
      return 1;
    }
    printf("Running demo %d: %s\n", selected_index,
           gDemos[selected_index].name);
    gDemos[selected_index].run(cicd_mode);
    return 0;
  }

  return 0;
}
