#include "base.c"

#define SQ BEATTIME/4
#define Q BEATTIME/2
#define C BEATTIME
#define M BEATTIME*2
#define SB BEATTIME*4
#define PAUSETIME BEATTIME/8
#include "pressure-cooker.notes" // Note data

int n0, n1;
int i0, i1;
int nexttime0, nexttime1;
int pause0, pause1;
int done0, done1;

void song_setup() {
  n0 = sizeof(f0)/sizeof(char);
  n1 = sizeof(f1)/sizeof(char);
  nexttime0 = 0; nexttime1 = 0;
  done0 = 0; done1 = 0;
  i0 = 0; i1 = 0;
}

int main() {
  setup();
  song_setup();
  while (1) {
    if (master_clock == nexttime0) {
      if (i0 >= n0) {
	done0 = 1;
	audc[0] = 0;
      }
      else {
	if (pause0) {
	  audc[0] = 0;
	  pause0 = 0;
	  nexttime0 += PAUSETIME;
	  i0++;
	}
	else {
	  audf[0] = f0[i0];
	  audc[0] = c0[i0];
	  if (i0 < n0-1 && f0[i0+1]==f0[i0] && c0[i0+1]==c0[i0]) {
	    if (t0[i0] > PAUSETIME) {
		nexttime0 += t0[i0] - PAUSETIME;
		pause0 = 1;
	    }
	  }
	  else {
	    nexttime0 += t0[i0];
	    i0++;
	  }
	}
      }
    }

    if (master_clock == nexttime1) {
      if (i1 >= n1) {
	done1 = 1;
	audc[1] = 0;
      }
      else {
	if (pause1) {
	  audc[1] = 0;
	  pause1 = 0;
	  nexttime1 += PAUSETIME;
	  i1++;
	}
	else {
	  audf[1] = f1[i1];
	  audc[1] = c1[i1];
	  if (i1 < n1-1 && f1[i1+1]==f1[i1] && c1[i1+1]==c1[i1]) {
	    if (t1[i1] > PAUSETIME) {
		nexttime1 += t1[i1] - PAUSETIME;
		pause1 = 1;
	    }
	  }
	  else {
	    nexttime1 += t1[i1];
	    i1++;
	  }
	}
      }
    }

    if (done0 && done1)
      break;

    tick();
  }
}
