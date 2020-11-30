#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "msp432_boostxl_init.h"
#include "msp432_arm_dsp.h"
#include <math.h>
#include <stdio.h>

int k=0;
uint16_t processSample(uint16_t s) {
    float32_t phase = (2.0 * M_PI / 1024.0) * k;
    k = (k + 1) & 1023;
    return f32_to_dac14(0.1 * cos(phase));
}

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    uint32_t c = measurePerfSample(processSample);
    printf("exectime: %d\n", c);

    msp432_boostxl_init_intr(FS_16000_HZ, BOOSTXL_J1_2_IN, processSample);
    msp432_boostxl_run();

    return 1;
}
