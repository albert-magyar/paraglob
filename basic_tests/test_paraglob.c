#include <stdio.h>
#include "paraglob.h"

int main(int argc, char **argv) {
  paraglob_t pg = paraglob_create(PARAGLOB_ASCII, NULL);
  printf("Created PG.\n");
  printf("Attempting to add \"test\" (0 expected): %d\n", paraglob_insert(pg, 5, "test", NULL));
  printf("Attempting to add \"he*\" (0 expected): %d\n", paraglob_insert(pg, 5, "he*", NULL));
  printf("Attempting to find \"hello\" (1 expected): %d\n", paraglob_match(pg, 5, "hello"));
  printf("Attempting to find \"he\" (1 expected): %d\n", paraglob_match(pg, 5, "he"));
  printf("Attempting to find \"ww\" (0 expected): %d\n", paraglob_match(pg, 5, "ww"));
  printf("Attempting to add \"g*gle\" (0 expected): %d\n", paraglob_insert(pg, 5, "g*gle", NULL));
  printf("Attempting to find \"google\" (1 expected): %d\n", paraglob_match(pg, 5, "google"));
  printf("Attempting to find \"ggle\" (1 expected): %d\n", paraglob_match(pg, 5, "ggle"));
  printf("Attempting to find \"go\" (0 expected): %d\n", paraglob_match(pg, 5, "go"));
  printf("Attempting to add \"*book\" (0 expected): %d\n", paraglob_insert(pg, 5, "*book", NULL));
  printf("Attempting to find \"facebook\" (1 expected): %d\n", paraglob_match(pg, 5, "facebook"));
  return 0;
}

