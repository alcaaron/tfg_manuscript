#include "Battery.hh"

namespace lib::power
{
    Battery::Battery()
    {
        pinMode(BATTERY_VOLTAGE_PIN, INPUT);
    }

    uint16_t Battery::read_raw_adc()
    {
        return analogRead(BATTERY_VOLTAGE_PIN);
    }

    uint8_t Battery::get_battery_percentage()
    {
        uint16_t raw = read_raw_adc();
        // Convert 12-bit ADC reading to actual battery voltage.
        // ADC range 0–4095 maps to 0–3.3 V; BATTERY_ADC_FACTOR corrects the
        // resistor divider on the hardware (typically 2.0 for a 50% divider).
        float voltage_v = (raw / 4095.0f) * 3.3f * BATTERY_ADC_FACTOR;
        return voltage_to_percent(voltage_v);
    }

    uint8_t Battery::voltage_to_percent(float voltage_v)
    {
        if (voltage_v >= LUT[0].voltage)
            return LUT[0].percent;

        if (voltage_v <= LUT[LUT_SIZE - 1].voltage)
            return LUT[LUT_SIZE - 1].percent;

        for (size_t i = 0; i < LUT_SIZE - 1; ++i)
        {
            const battery_point_t &high = LUT[i];
            const battery_point_t &low  = LUT[i + 1];

            if (voltage_v <= high.voltage && voltage_v >= low.voltage)
            {
                float t = (voltage_v - low.voltage) / (high.voltage - low.voltage);
                float percent = low.percent + t * (float)(high.percent - low.percent);
                return static_cast<uint8_t>(percent + 0.5f);
            }
        }

        return 0;
    }
}
