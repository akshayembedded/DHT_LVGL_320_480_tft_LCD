#include <Arduino.h>
#include <DHT.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "Wire.h"
void lcd_init();
void lcd_handle();
void update_battery_icon(float voltage, int percent, bool charging);
float readBatteryVoltage() ;
int getBatteryPercentage(float voltage);
bool isCharging();

#define BAT_ADC 35
constexpr uint8_t DHT_PIN = 14;
DHT dht(DHT_PIN, DHT11);

#define TFT_HOR_RES   320
#define TFT_VER_RES   480
#define TFT_ROTATION  LV_DISPLAY_ROTATION_90

#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

lv_obj_t *label_temp_title;
lv_obj_t *label_hum_title;
lv_obj_t *label_temp_value;
lv_obj_t *label_hum_value;
lv_obj_t *box_temp;
lv_obj_t *box_hum;
lv_obj_t *battery_icon_label;
lv_obj_t *battery_voltage_label;
lv_obj_t *battery_percent_label;
lv_obj_t *charging_icon_label;

unsigned long lastUpdate = 0;

char temp_buf[32];
char hum_buf[32];

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  dht.begin();
  Wire.begin();
  lv_init();
  lcd_init();
}

void loop() {
  float voltage = readBatteryVoltage();
  int percentage = getBatteryPercentage(voltage);
  bool charging = isCharging();

  update_battery_icon(voltage, percentage, charging);
  lcd_handle();
  lv_timer_handler();
  delay(100);
}

void lcd_init() {
  lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, TFT_ROTATION);

  lv_obj_t *label_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_title, "Environment Monitor");
  lv_obj_set_style_text_font(label_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_title, lv_color_hex(0x00000), 0);
  lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

  label_temp_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_temp_title, "Temperature");
  lv_obj_align(label_temp_title, LV_ALIGN_CENTER, 0, -90);

  box_temp = lv_obj_create(lv_screen_active());
  lv_obj_set_size(box_temp, 220, 60);
  lv_obj_align(box_temp, LV_ALIGN_CENTER, 0, -50);
  lv_obj_set_style_bg_color(box_temp, lv_color_hex(0xFF5733), 0);
  lv_obj_set_style_radius(box_temp, 10, 0);

  label_temp_value = lv_label_create(box_temp);
  lv_label_set_text(label_temp_value, "-- °C");
  lv_obj_center(label_temp_value);
  lv_obj_set_style_text_font(label_temp_value, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_temp_value, lv_color_hex(0xFFFFFF), 0);

  label_hum_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_hum_title, "Humidity");
  lv_obj_align(label_hum_title, LV_ALIGN_CENTER, 0, -8);

  box_hum = lv_obj_create(lv_screen_active());
  lv_obj_set_size(box_hum, 220, 60);
  lv_obj_align(box_hum, LV_ALIGN_CENTER, 0, 30);
  lv_obj_set_style_bg_color(box_hum, lv_color_hex(0x3498DB), 0);
  lv_obj_set_style_radius(box_hum, 10, 0);

  label_hum_value = lv_label_create(box_hum);
  lv_label_set_text(label_hum_value, "-- %");
  lv_obj_center(label_hum_value);
  lv_obj_set_style_text_font(label_hum_value, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_hum_value, lv_color_hex(0xFFFFFF), 0);

  battery_icon_label = lv_label_create(lv_screen_active());
  lv_obj_align(battery_icon_label, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFFFF00), 0); // Set to green
  lv_label_set_text(battery_icon_label, LV_SYMBOL_BATTERY_EMPTY);

  charging_icon_label = lv_label_create(lv_screen_active());
  lv_label_set_text(charging_icon_label, LV_SYMBOL_CHARGE);
  lv_obj_set_style_text_color(charging_icon_label, lv_color_hex(0xFFA500), 0); // Red
  lv_obj_align_to(charging_icon_label, battery_icon_label, LV_ALIGN_OUT_LEFT_MID, -1, 0);
  lv_obj_add_flag(charging_icon_label, LV_OBJ_FLAG_HIDDEN);

  // battery_percent_label = lv_label_create(lv_screen_active());
  // lv_label_set_text(battery_percent_label, "--%");
  // lv_obj_align_to(battery_percent_label, battery_icon_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  // battery_voltage_label = lv_label_create(lv_screen_active());
  // lv_label_set_text(battery_voltage_label, "-- V");
  // lv_obj_align_to(battery_percent_label, battery_percent_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

void lcd_handle() {
  lv_tick_inc(5);

  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      snprintf(temp_buf, sizeof(temp_buf), "%.1f °C", t);
      snprintf(hum_buf, sizeof(hum_buf), "%.1f %%", h);

      lv_label_set_text(label_temp_value, temp_buf);
      lv_label_set_text(label_hum_value, hum_buf);

      Serial.printf("Temp: %.1f °C, Hum: %.1f %%\n", t, h);
    } else {
      Serial.println("Failed to read from DHT sensor (NaN)!");
    }
  }
}

float readBatteryVoltage() {
  int raw = analogRead(BAT_ADC);
  return ((raw * 2.0 * 3.3) / 4095.0);
  
}

int getBatteryPercentage(float voltage) {
  if (voltage >= 3.82) return 100;
  if (voltage >= 3.75) return 75;
  if (voltage >= 3.60) return 50;
  if (voltage >= 3.50) return 25;
  return 10;
}

bool isCharging() {
  Wire.beginTransmission(0x75);
  Wire.write(0x70);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(0x75, 1);
  if (Wire.available()) {
    uint8_t val = Wire.read();
    return val & 0x08;
  }
  return false;
}

void update_battery_icon(float voltage, int percent, bool charging) {
  //  int raw = analogRead(BAT_ADC);

  const char* battery_icon;
  if (percent >= 95)
    {battery_icon = LV_SYMBOL_BATTERY_FULL; lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0x00FF00), 0); }// Green
  else if (percent >= 65)
    {battery_icon = LV_SYMBOL_BATTERY_3;  lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0x66FF66), 0);}
  else if (percent >= 35)
    {battery_icon = LV_SYMBOL_BATTERY_2;  lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFFFF00), 0);} // Yellow
  else if (percent >= 15)
    {battery_icon = LV_SYMBOL_BATTERY_1;  lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFFA500), 0); }// Orange
  else
    {battery_icon = LV_SYMBOL_BATTERY_EMPTY; lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFF0000), 0);} // Red

  lv_label_set_text(battery_icon_label, battery_icon);

  if (charging)
    lv_obj_clear_flag(charging_icon_label, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(charging_icon_label, LV_OBJ_FLAG_HIDDEN);

  // char buf[16];
  // snprintf(buf, sizeof(buf), "%d%%", percent);
  // lv_label_set_text(battery_percent_label, buf);

  // snprintf(buf, sizeof(buf), "%4d V", raw);
  // lv_label_set_text(battery_voltage_label, buf);
}
