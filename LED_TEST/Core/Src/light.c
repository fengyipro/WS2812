#include "light.h"

void Light_Init(void) {
}

static void Light_SelectChannel(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = LIGHT_ADC_CHANNEL;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

uint32_t Light_GetValue(void) {
    Light_SelectChannel();
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        return HAL_ADC_GetValue(&hadc1);
    }
    return 0;
}

uint32_t Light_GetAverage(uint16_t samples) {
    uint32_t sum = 0;
    if (samples == 0) {
        return 0;
    }
    for (uint16_t i = 0; i < samples; i++) {
        sum += Light_GetValue();
    }
    return sum / samples;
}
