#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "msp432_boostxl_init.h"
#include "msp432_arm_dsp.h"
#include <math.h>
#include <stdio.h>

// sets the phase resolution (2^P with P ~ number of bits in phase)
#define PHASEQ 32

// sets the frequency step (2^N with N ~ number of bits in accumulator)
#define STEPQ 1024

// sets the ouput resolution (M ~ number of bits in output, 1 .. 15)
#define COSBITS 15

int coslu[PHASEQ];
void initcoslu() {
    int i;
    for (i=0; i<PHASEQ; i++)
        coslu[i] = ((int) (0.1 * cosf(2 * M_PI * i / PHASEQ) * (1 << COSBITS))) << (15 - COSBITS);
}

int k=0;
uint16_t processSample(uint16_t s) {
    float32_t phase = (2.0 * M_PI / STEPQ) * k;
    k = (k + 100) & (STEPQ - 1);
    return q15_to_dac14(coslu[k / (STEPQ/PHASEQ)]);
}

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    initcoslu();

    msp432_boostxl_init_intr(FS_16000_HZ, BOOSTXL_J1_2_IN, processSample);
    msp432_boostxl_run();

    return 1;
}
