#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "msp432_boostxl_init.h"
#include "msp432_arm_dsp.h"
#include <math.h>
#include <stdio.h>

#define FS    8000
#define FS_HZ FS_8000_HZ
#define PTHRESHOLD 1000
#define FRACBITS 8
#define FIXMUL(A, B) ((A * B) >> FRACBITS)

typedef struct {
    int coef;
    int s2, s1;
} Gtap;

Gtap t697;
Gtap t770;
Gtap t852;
Gtap t1209;
Gtap t1336;
Gtap t1477;

void initgtap() {
    t697.coef  = (int) (cosf(2 * M_PI *   697 / FS) * 2 * (1 << FRACBITS));
    t770.coef  = (int) (cosf(2 * M_PI *   770 / FS) * 2 * (1 << FRACBITS));
    t852.coef  = (int) (cosf(2 * M_PI *   852 / FS) * 2 * (1 << FRACBITS));
    t1209.coef = (int) (cosf(2 * M_PI *  1209 / FS) * 2 * (1 << FRACBITS));
    t1336.coef = (int) (cosf(2 * M_PI *  1336 / FS) * 2 * (1 << FRACBITS));
    t1477.coef = (int) (cosf(2 * M_PI *  1477 / FS) * 2 * (1 << FRACBITS));
}

void samplegoertzel(Gtap *t, int x) {
    int s0;
    s0    = x + FIXMUL(t->s1, t->coef) - t->s2;
    t->s2 = t->s1;
    t->s1 = s0;
}

void resetgoertzel(Gtap *t) {
    t->s1 = 0;
    t->s2 = 0;
}

int powergoertzel(Gtap *t) {
    return (FIXMUL(t->s1, t->s1) +
            FIXMUL(t->s2, t->s2) -
            FIXMUL(FIXMUL(t->coef, t->s1), t->s2));
}

void dtmfreset() {
    resetgoertzel( &t697);
    resetgoertzel( &t770);
    resetgoertzel( &t852);
    resetgoertzel(&t1209);
    resetgoertzel(&t1336);
    resetgoertzel(&t1477);
}

void dtmfaddsample(int x) {
    samplegoertzel( &t697, x);
    samplegoertzel( &t770, x);
    samplegoertzel( &t852, x);
    samplegoertzel(&t1209, x);
    samplegoertzel(&t1336, x);
    samplegoertzel(&t1477, x);
}

// #define DTMFDEBUG

int dtmfdecode() {
    unsigned p697  = powergoertzel(  &t697);
    unsigned p770  = powergoertzel(  &t770);
    unsigned p852  = powergoertzel(  &t852);
    unsigned p1209 = powergoertzel( &t1209);
    unsigned p1336 = powergoertzel( &t1336);
    unsigned p1477 = powergoertzel( &t1477);

#ifdef DTMFDEBUG
    printf("%5d %5d %5d %5d %5d %5d\n", p697, p770, p852, p1209, p1336, p1477);
#endif

    int d697  = (p697  > PTHRESHOLD);
    int d770  = (p770  > PTHRESHOLD);
    int d852  = (p852  > PTHRESHOLD);
    int d1209 = (p1209 > PTHRESHOLD);
    int d1336 = (p1336 > PTHRESHOLD);
    int d1477 = (p1477 > PTHRESHOLD);

    // - if three or more thresholds are exceeded,
    //   we will reject this as valid dtmf, it's
    //   noise or a background sound
    // - if two thresholds are exceeded, we try to
    //   decoded as a valid dtmf combination
    // - in all other cases, we reject this tone

    if ((d697 + d770 + d852 + d1209 + d1336 + d1477) >= 3)
        return -1;
    else if (d852 && d1477)
        return 9;
    else if (d852 && d1336)
        return 8;
    else if (d852 && d1209)
        return 7;
    else if (d770 && d1477)
        return 6;
    else if (d770 && d1336)
        return 5;
    else if (d770 && d1209)
        return 4;
    else if (d697 && d1477)
        return 3;
    else if (d697 && d1336)
        return 2;
    else if (d697 && d1209)
        return 1;

    return -1;
}


int samplectr = 0;
#define GWINDOW 256
int lastdtmf = -1;

uint16_t processSample(uint16_t s) {
    int x = adc14_to_q15(s);

    samplectr = (samplectr + 1) % GWINDOW;

    if (samplectr == 0) {
        int newdtmf = dtmfdecode();
        if (newdtmf != lastdtmf)
            printf("%d\n", newdtmf);
        lastdtmf = newdtmf;
        dtmfreset();
    } else
        dtmfaddsample(x >> (15 - FRACBITS));

    return q15_to_dac14(x); // silence
}

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    initgtap();
    dtmfreset();

    msp432_boostxl_init_intr(FS_HZ, BOOSTXL_MIC_IN, processSample);
    msp432_boostxl_run();

    return 1;
}
