#include "data.h"
#include "rtl_433.h"
#include "util.h"
#include "math.h"

#define CRC_POLY 0x31
#define CRC_INIT 0xff

static char* get_battery(const uint8_t* br) {
        if ((br[7] >> 4) != 1) {
                return "OK";
        } else {
                return "LOW";
        }
}

static float get_temperature(const uint8_t* br) {
    const int temp_raw = (br[2] << 8) + br[3];
    return ((temp_raw & 0x0fff) - 0x190) / 10.0;
}

static int get_humidity(const uint8_t* br) {
    return br[4];
}

static float get_wind_avg_kmh(const uint8_t* br) {
    return ((br[5] * 34.0f) / 100) * 3.6f; // Km/h
}

static float get_wind_gust_kmh(const uint8_t* br) {
    return ((br[6] * 34.0f) / 100) * 3.6f; // Km/h
}

static float get_rainfall(const uint8_t* br) {
        return ((((unsigned short)br[7] & 0x0f) << 8) | br[8]) * 0.3f;
}

static int maplin_n25fr_callback(bitbuffer_t *bitbuffer) {
    data_t *data;
    const uint8_t *br;

    if (bitbuffer->num_rows != 1) {
        return 0;
    }
    if (bitbuffer->bits_per_row[0] != 80) {
        return 0;
    }

    br = bitbuffer->bb[0];

    if (br[0] != 0xff) {
        // preamble missing
        return 0;
    }

    if (br[9] != crc8(br, 9, CRC_POLY, CRC_INIT)) {
        // crc mismatch
        return 0;
    }

    const float temperature = get_temperature(br);
    const int humidity = get_humidity(br);
    const float speed = get_wind_avg_kmh(br);
    const float gust = get_wind_gust_kmh(br);
    const char* battery = get_battery(br);
    const float rain = get_rainfall(br);

    data = data_make("temperature_C", "Temperature",    DATA_FORMAT, "%.01f",DATA_DOUBLE, temperature,
                     "humidity",      "Humidity",       DATA_FORMAT, "%u %%",DATA_INT,    humidity,
                     "speed",         "Wind avg speed", DATA_FORMAT, "%.02f",DATA_DOUBLE, speed,
                     "gust",          "Wind gust",      DATA_FORMAT, "%.02f",DATA_DOUBLE, gust,
                     "rain",          "Total rainfall", DATA_FORMAT, "%.01f",DATA_DOUBLE, rain,
                     "battery",       "Battery",        DATA_STRING, battery,
                     NULL);
    data_acquired_handler(data);
    return 1;
}

static char *output_fields[] = {
        "temperature_C",
        "humidity",
        "speed",
        "gust",
        "rain",
        "battery",
        NULL
};

r_device maplin_n25fr = {
    .name           = "Maplin N25FR",
    .modulation     = OOK_PULSE_PWM_RAW,
    .short_limit    = 800,
    .long_limit     = 2800,
    .reset_limit    = 2800,
    .json_callback  = &maplin_n25fr_callback,
    .disabled       = 0,
    .demod_arg      = 0,
    .fields         = output_fields
};

