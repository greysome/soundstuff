#include <stdio.h>
#include "sintable"  // defines the 2000-element int array sintable

#define F 8000
#define sin(freq) T=t%F; putchar(sintable[T]); t += freq;
#define square(freq,dc) T=t%F;	     \
  if (0<=T && T<dc*F) putchar(255);  \
  else putchar(0);		     \
  t += freq;

int main() {
  int T;
  for (int t=0;;) {
    sin(1000);
  }
}
