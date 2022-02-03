#include "base.c"

int main() {
  setup();
  audc[0] = 1; audc[1] = 1;
  while (1) {
    tick();
    if (master_clock % 10000 == 0) {
      audf[0]++; audf[1]++;
      if (master_clock % 320000 == 0) {
	audf[0] = 0; audf[1] = 0;
	audc[0]++; audc[1]++;
      }
    }
  }
}