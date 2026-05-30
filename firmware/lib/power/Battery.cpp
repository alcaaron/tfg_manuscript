#include "battery.hh"

namespace lib::power
{
    Battery::Battery() 
    {
        pinMode(BATTERY_VOLTAGE_PIN, INPUT);
    }

    uint16_t Battery::read_battery_voltage()
    {
        return analogRead(BATTERY_VOLTAGE_PIN);
    }

    uint8_t Battery::get_battery_percentage()
    {
        uint16_t raw_voltage = read_battery_voltage();
        return voltage_to_percent(raw_voltage);
    }

    uint8_t Battery::voltage_to_percent(uint16_t &voltage)
    {
        if (voltage >= LUT[0].voltage)
            return LUT[0].percent;

        if (voltage <= LUT[LUT_SIZE - 1].voltage)
            return LUT[LUT_SIZE - 1].percent;

        for (size_t i = 0; i < LUT_SIZE - 1; ++i)
        {
            const battery_porint_t &high = LUT[i];
            const battery_porint_t &low  = LUT[i + 1];

            if (voltage <= high.voltage &&
                voltage >= low.voltage)
            {
                // y = y0 + (x-x0)*(y1-y0)/(x1-x0)
                float t =
                    (voltage - low.voltage) /
                    (high.voltage - low.voltage);

                float percent =
                    low.percent +
                    t * (high.percent - low.percent);

                return static_cast<uint8_t>(percent + 0.5f);
            }
        }

        return 0;
    }
}