#pragma once

struct Reading {
  float temp_c = 0.0f;
  float humidity_pct = 0.0f;
  float pressure_hpa = 0.0f;
  float lux = 0.0f;
  float uv_index = 0.0f;
  float battery_v = 0.0f;
  bool has_bme280 = false;
  bool has_lux = false;
  bool has_bh1750 = false;
  bool has_uv = false;
  bool ok = false;
};
