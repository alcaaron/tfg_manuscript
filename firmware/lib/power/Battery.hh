#pragma once
#include <config.h>
#include <stdint.h>
#include <stddef.h>
#include <Arduino.h>

namespace lib::power
{
    /*!
     * \brief Voltage-to-percentage mapping entry for the battery LUT.
     */
    typedef struct
    {
        float voltage; /*!< Cell voltage in volts. */
        uint8_t percent; /*!< Corresponding state-of-charge percentage (0–100). */
    } battery_point_t;

    /*!
     * \brief Look-up table mapping cell voltage to state-of-charge percentage.
     *
     * Entries must be ordered from highest to lowest voltage.  The table covers
     * the usable discharge range of a single-cell Li-Ion/LiPo cell from 4.20 V
     * (fully charged) down to 3.00 V (fully depleted).
     */
    static constexpr battery_point_t LUT[] =
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

    /*!
     * \brief Number of entries in the battery voltage look-up table.
     */
    static constexpr size_t LUT_SIZE =
        sizeof(LUT) / sizeof(LUT[0]);

    /*!
     * \brief Battery state-of-charge estimator for a single-cell Li-Ion/LiPo pack.
     *
     * Reads the raw ADC value from the voltage divider connected to the cell,
     * converts it to a cell voltage, and interpolates the result against \ref LUT
     * to produce a percentage suitable for reporting over BLE.
     */
    class Battery
    {
    public:
        Battery();

        /*!
         * \brief Return the current battery charge level.
         * \return State-of-charge in the range [0, 100] percent.
         */
        uint8_t get_battery_percentage();

    private:
        /*!
         * \brief Interpolate a state-of-charge percentage from a cell voltage.
         * \param voltage_v Measured cell voltage in volts.
         * \return State-of-charge in the range [0, 100] percent.
         */
        uint8_t  voltage_to_percent(float voltage_v);

        /*!
         * \brief Sample the ADC pin connected to the battery voltage divider.
         * \return Raw 12-bit ADC reading (0–4095).
         */
        uint16_t read_raw_adc();
    };
} // namespace lib::power