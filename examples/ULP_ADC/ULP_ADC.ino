/*
 * @Description: ULP检测ADC
 *                      适用arduino_esp32版本：3.0.0-rc3
 * @version: V1.0.0
 * @Author: None
 * @Date: 2024-04-29 19:02:41
 * @LastEditors: LILYGO_L
 * @LastEditTime: 2024-05-25 14:53:31
 * @License: GPL 3.0
 */
#include <Arduino.h>
#include "driver/rtc_io.h"
#include "sdkconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "esp32s3/ulp.h"

void ULP_Initialization()
{
    memset(RTC_SLOW_MEM, 0, 100);

    const ulp_insn_t program[] = {
        I_MOVI(R0, 0),
        I_MOVI(R1, 0),
        I_MOVI(R2, 0),

        M_LABEL(1), // do {

        // delay(1000);
        I_MOVI(R0, 200), // R0 = n * 1000 / 5, where n is the number of seconds to delay, 200 = 1 s
        M_LABEL(2),      // do {
                         // delay (5);
                         // since ULP runs at 8 MHz
                         // 40000 cycles correspond to 5 ms (max possible delay is 65535 cycles or 8.19 ms)
        I_DELAY(40000),
        I_SUBI(R0, R0, 1), // R0 --;
        M_BG(2, 1),        // } while (R0 >= 1); ... jump to label 2 if R0 > 0

        I_ADC(R2, 0, 0),   // R2 ≈ adc1_get_raw (ADC1_CHANNEL_0);  // GPIO 1
        I_ADDI(R1, R1, 1), // R1 ++
        // store sample count to RTC_SLOW_MEM [0]
        I_ST(R1, 0, 0), // RTC_SLOW_MEM [1] = R1;
        I_ST(R2, 0, 1), // RTC_SLOW_MEM [2] = R3;

        I_MOVR(R0, R1), // while (R1 < 100);
        M_BL(1, 1000),  // ... jump to label 1

        I_END(),
    };

    size_t size = sizeof(program) / sizeof(ulp_insn_t);
    ulp_process_macros_and_load(32, program, &size);
    ulp_run(32);

    // in which it also prints the debug output mentioned earlier
    // Serial.printf("ULP Mem in main app: %d\n", CONFIG_ESP32S3_ULP_COPROC_RESERVE_MEM);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Ciallo");

    // pinMode(7, INPUT_PULLDOWN);
    // analogReadResolution(12);

    // Serial.printf("IO7 ADC Voltage: %.03f V\n", ((float)analogReadMilliVolts(7)) / 1000.0);

    ULP_Initialization();
}

void loop()
{
    for (int i = 0; i < 10; i++)
    {
        Serial.printf("RTC_SLOW_MEM[%d]: %d\n", i, RTC_SLOW_MEM[i] & 0xFFF);
    }

    Serial.printf("\n");
    delay(1000);
}