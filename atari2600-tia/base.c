#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

int outfd0, outfd1, err;

unsigned long master_clock;
uint8_t freqdiv[2]; // bit for frequency division
uint8_t audc[2]; // distortion type (4-bit)
uint8_t audf[2]; // frequency division (5-bit)
uint8_t audv[2]; // volume (4-bit)
uint16_t lfsr[2]; // 9-bit LFSR, whose last bit is output
uint16_t clock[2]; // 2-bit LFSR as clock
uint8_t bit[2]; // bit for alternating waveform
char c[2]; // output bit for audio
uint16_t r1, rb; // temporary registers for updating LFSR

// to keep track of real time, so as to sync up with 31.44kHz audio output
long elapsed;
struct timeval tvstart, tvcur;

void update_clock(int i) {
  rb = (clock[i] ^ (clock[i] >> 1)) & 1;
  clock[i] = (rb << 1) | (clock[i] >> 1);
}

void update_lfsr_9bit(int i) {
  rb = (lfsr[i] ^ (lfsr[i] >> 4)) & 1;
  lfsr[i] = (rb << 8) | (lfsr[i] >> 1);
}

void update_lfsr_5bit(int i) {
  r1 = (lfsr[i] >> 4) & 0b11111;
  rb = (r1 ^ (r1 >> 2)) & 1;
  r1 = (rb << 4) | (r1 >> 1);
  lfsr[i] = (lfsr[i] & 0b000001111) ^ (r1 << 4);
}

void update_lfsr_4bit(int i) {
  r1 = lfsr[i] & 0b1111;
  rb = (r1 ^ (r1 >> 1)) & 1;
  r1 = (rb << 3) | (r1 >> 1);
  lfsr[i] = (lfsr[i] & 0b111110000) ^ r1;
}

void update(int i) {
  switch (audc[i]) {
    case 0: case 11: /* continuous high */
      c[i] = 1; break;
    case 1: /* 4-bit LFSR */
      update_lfsr_4bit(i);
      c[i] = lfsr[i] & 1 ? audv[i] : 0;
      break;
    case 2: /* 4-bit LFSR clocked to Distortion 6 */
      update_lfsr_5bit(i);
      if ((lfsr[i] >> 4) == 1 | (lfsr[i] >> 4) == 0b11110)
        update_lfsr_4bit(i);
      c[i] = lfsr[i] & 1 ? audv[i] : 0;
      break;
    case 3: /* 4-bit LFSR, updates when 5-bit LFSR outputs high */
      if ((lfsr[i] >> 4) & 1)
        update_lfsr_4bit(i);
      update_lfsr_5bit(i);
      c[i] = lfsr[i] & 1 ? audv[i] : 0;
      break;
    case 4: case 5: /* alternating low-high */
      bit[i] ^= 1;
      c[i] = bit[i] ? audv[i] : 0;
      break;
    case 6: case 10: /* alternating low-high, update when 5-bit LFSR is 00001 or 11110*/
      update_lfsr_5bit(i);
      if ((lfsr[i] >> 4) == 1 | (lfsr[i] >> 4) == 0b11110)
        bit[i] ^= 1;
      c[i] = bit[i] ? audv[i] : 0;
      break;
    case 7: case 9: /* 5-bit LFSR */
      update_lfsr_5bit(i);
      c[i] = (lfsr[i] >> 4) & 1 ? audv[i] : 0;
      break;
    case 8: /* 9-bit LFSR */
      update_lfsr_9bit(i);
      c[i] = lfsr[i] & 1 ? audv[i] : 0;
      break;
    case 12: case 13: /* alternating low-high, update every 3 ticks */
      if (!freqdiv) break;
      update_clock(i);
      if (!(clock[i] & 1))
        bit[i] ^= 1;
      c[i] = bit[i] ? audv[i] : 0;
      break;
    case 14: /* alternating low-high, update when 4 bits in 5-bit LFSR equal, every 3 ticks */
      update_clock(i);
      if (!(clock[i] & 1)) {
        update_lfsr_5bit(i);
        if ((lfsr[i] >> 4) == 1 | (lfsr[i] >> 4) == 0b11110)
          bit[i] ^= 1;
      }
      c[i] = bit[i] ? audv[i] : 0;
      break;
    case 15: /* 5-bit LFSR, update every 3 ticks */
      update_clock(i);
      if (!(clock[i] & 1))
        update_lfsr_5bit(i);
      c[i] = (lfsr[i] >> 4) & 1 ? audv[i] : 0;
      break;
    }
}

void tick() {
  master_clock++;

  audc[0] &= 0b1111; audc[1] &= 0b1111;
  audf[0] &= 0b11111; audf[1] &= 0b11111;
  audv[0] &= 0b1111; audv[1] &= 0b1111;
  if (master_clock % (audf[0]+1) == 0)
    freqdiv[0] = 1;
  if (master_clock % (audf[1]+1) == 0)
    freqdiv[1] = 1;

  if (freqdiv[0]) update(0);
  if (freqdiv[1]) update(1);

  // Wait until 31.8 microseconds have elapsed (it won't wait exactly, but it's good enough)
  // TODO: fix the non-syncing over time
  while (1) {
    gettimeofday(&tvcur, NULL);
    elapsed = 1000000 * (tvcur.tv_sec - tvstart.tv_sec) +
      (tvcur.tv_usec - tvstart.tv_usec);
    if (elapsed % 32 >= 28)
      break;
  }

  write(outfd0, &c[0], 1);
  write(outfd1, &c[1], 1);
  freqdiv[0] = 0; freqdiv[1] = 0;
}

void cleanup() {
  close(outfd0);
  close(outfd1);
}

void setup() {
  err = unlink("/tmp/sound0");
  printf("unlink0 %d\n", err);
  err = unlink("/tmp/sound1");
  printf("unlink1 %d\n", err);
  err = mkfifo("/tmp/sound0", S_IWUSR|S_IRUSR);
  printf("mkfifo0 %d\n", err);
  err = mkfifo("/tmp/sound1", S_IWUSR|S_IRUSR);
  printf("mkfifo1 %d\n", err);
  err = system("play -q --rate 31440 --type raw --bits 8 --encoding signed-integer /tmp/sound0 remix 1 0 &");
  printf("system0 %d\n", err);
  err = system("play -q --rate 31440 --type raw --bits 8 --encoding signed-integer /tmp/sound1 remix 0 1 &");
  printf("system1 %d\n", err);
  outfd0 = open("/tmp/sound0", O_WRONLY);
  printf("open0 %d\n", errno);
  outfd1 = open("/tmp/sound1", O_WRONLY);
  printf("open1 %d\n", errno);

  master_clock = 0;
  lfsr[0] = 0b000010001; lfsr[1] = 0b000010001;
  clock[0] = 0b001; clock[1] = 0b001;
  audf[0] = 0; audc[0] = 0; audv[0] = 0xf;
  audf[1] = 0; audc[1] = 0; audv[1] = 0xf;
  gettimeofday(&tvstart, NULL);

  atexit(cleanup);
}