#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char *result, *expected;
  int isSame;
  result = stateToString(SEARCHING);
  expected = "SEARCHING";
  isSame = strcmp(expected, result) == 0;
  assert(isSame);
}
