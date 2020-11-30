#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "msp432_boostxl_init.h"
#include "msp432_arm_dsp.h"
#include <math.h>
#include <stdio.h>

// samples per symbol
#define SS 20

// AFSK specs
#define CENTER  1700.0
#define GAP     500.0
#define FS_HZ   FS_24000_HZ
#define FS      24000.0

#define CFS       (int) ((CENTER / FS) * (1 << 15))
#define GFS       (int) ((GAP / FS) * (1 << 15))
#define FX_2PI    (int) ((2.0*M_PI) * (1 << 15))
#define FX_1_2PI  (int) ((1.0/2.0/M_PI) * (1 << 15))

#define TRAININGLEN 1024

int coslu[256];
void initcoslu() {
    int i;
    for (i=0; i<256; i++)
        coslu[i] = (int) (0.1 * cosf(2 * M_PI * i / 256.0) * (1 << 15));
}

int rand_symbol() {
    return (2*(rand() %2) - 1);
}

int next_symbol(int start, int stop) {
    enum {IDLE, TRAINING, ACTIVE};
    static int state = IDLE;
    static int trainingctr = 0;
    switch (state) {
    case IDLE:
        if (start != 0) {
            state = TRAINING;
            trainingctr = 0;
        }
        return -1;
        break;
    case TRAINING:
        trainingctr = trainingctr + 1;
        if (trainingctr < TRAININGLEN)
            return 1;
        else if (trainingctr < TRAININGLEN)
            return ((trainingctr - TRAININGLEN/2) % 2) * 2 - 1;
        else {
            state = ACTIVE;
            return ((trainingctr - TRAININGLEN/2) % 2) * 2 - 1;
        }
        break;
    case ACTIVE:
        if (stop != 0) {
            state = IDLE;
        } else if (start != 0) {
            state = TRAINING;
        }
        return rand_symbol();
        break;
    }
}

static int samplectr = 0;
static int currentsymbol = 1;
static int angle = 0;

int next_sample(int start, int stop) {
    samplectr = samplectr + 1;

    if (samplectr == SS) {
        currentsymbol = next_symbol(start, stop);
        samplectr = 0;
    }

    angle = angle + 2*(CFS + GFS * currentsymbol);
    angle = (angle > (1 << 16)) ? angle - (1 << 16) : angle;

    int mycos = coslu[angle >> 8];
}

uint16_t processSample(uint16_t s) {
    int start = 0, stop = 0;

    if ((start == 0) && pushButtonLeftDown()) {
        start = 1;
    } else
        start = 0;

    if ((stop == 0) && pushButtonRightDown()) {
        stop = 1;
    } else
        stop = 0;

    return q15_to_dac14(next_sample(start, stop));

}

int main(void) {
    WDT_A_hold(WDT_A_BASE);
    int k, c, s;
    unsigned i;

    initcoslu();

    msp432_boostxl_init_intr(FS_HZ, BOOSTXL_J1_2_IN, processSample);

    msp432_boostxl_run();

    return 1;
}
