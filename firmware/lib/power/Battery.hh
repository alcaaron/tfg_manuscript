#pragma once
#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <Arduino.h>

namespace lib::power
{
    typedef struct
    {
        float voltage;
        uint8_t percent;
    } battery_porint_t;

    static constexpr battery_porint_t LUT[] =
    {
        {4.20f, 100},
        {4.15f, 95},
        {4.11f, 90},
        {4.08f, 80},
        {4.02f, 70},
        {3.98f, 60},
        {3.95f, 50},
        {3.91f, 40},
        {3.87f, 30},
        {3.85f, 20},
        {3.80f, 15},
        {3.75f, 10},
        {3.70f, 8},
        {3.60f, 5},
        {3.50f, 3},
        {3.40f, 2},
        {3.30f, 1},
        {3.20f, 1},
        {3.10f, 0},
        {3.00f, 0}
    };

    static constexpr size_t LUT_SIZE =
        sizeof(LUT) / sizeof(LUT[0]);

    class Battery
    {
    public:
        Battery();
        uint8_t get_battery_percentage();
    private:
        uint8_t voltage_to_percent(uint16_t &voltage);
        uint16_t read_battery_voltage();
    };
}