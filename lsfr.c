#include <stdio.h>
#include <stdint.h>

/*
Minimal implementation of a linear feedback shift register
`len` is an integer <= 16 specifying the LSFG length
`taps` is a list of position of taps, encoded as a 16-bit int
`start` is the starting state

e.g. for a LSFG with length 4 and taps 3,4
len = 4, taps = 0x0003
*/
void lfsr(unsigned len, uint16_t taps, uint16_t start_state) {
  uint16_t lfsr = start_state;  // Current state
  uint16_t bit;  // The first bit of the LFSR, used as output

  while (1) {
    putchar((lfsr & 1) ? 255 : 0);

    // XOR the bits of the LFSR at the tap positions
    bit = 0;
    for (int i=0; i<len; i++) {
      // For debugging, if needed
      //printf("%d %x %u %u %u\n", i, lfsr, lfsr >> i, (taps >> i) & 1u, bit);
      bit ^= (lfsr >> i) & ((taps >> i) & 1u);
    }
    bit &= 1u;

    // Update LFSR with the bit
    lfsr = (lfsr >> 1) | (bit << len-1);
  }
}

int main() {
  //lfsr(2, 0x0003, 0x0001);
  //lfsr(5, 0x0005, 0x0001);
  //lfsr(6, 0x0003, 0x0001);
  //lfsr(7, 0x0003, 0x0001);
  //lfsr(8, 0x001d, 0x0001);
  //lfsr(9, 0x0011, 0x0001);
  //lfsr(10, 0x0009, 0x0001);
  //lfsr(11, 0x0005, 0x0001);
  //lfsr(16, 0x002d, 0x0001);
}