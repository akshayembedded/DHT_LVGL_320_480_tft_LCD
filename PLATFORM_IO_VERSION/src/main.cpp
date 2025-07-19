#include <Arduino.h>
#include <DHT.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

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
unsigned long lastUpdate = 0;

char temp_buf[32];
char hum_buf[32];

void setup()
{
  Serial.begin(115200);
  dht.begin();

  lv_init();

  lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, TFT_ROTATION);

  lv_obj_t *label_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_title, "Environment Monitor");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_title, lv_color_hex(0x00000), 0);
  lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

  // --- Temperature title ---
  label_temp_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_temp_title, "Temperature");
  lv_obj_align(label_temp_title, LV_ALIGN_CENTER, 0, -100);

  // --- Temperature box ---
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

  // --- Humidity title ---
  label_hum_title = lv_label_create(lv_screen_active());
  lv_label_set_text(label_hum_title, "Humidity");
  lv_obj_align(label_hum_title, LV_ALIGN_CENTER, 0, 10);

  // --- Humidity box ---
  box_hum = lv_obj_create(lv_screen_active());
  lv_obj_set_size(box_hum, 220, 60);
  lv_obj_align(box_hum, LV_ALIGN_CENTER, 0, 60);
  lv_obj_set_style_bg_color(box_hum, lv_color_hex(0x3498DB), 0);
  lv_obj_set_style_radius(box_hum, 10, 0);

  label_hum_value = lv_label_create(box_hum);
  lv_label_set_text(label_hum_value, "-- %");
  lv_obj_center(label_hum_value);
  lv_obj_set_style_text_font(label_hum_value, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_hum_value, lv_color_hex(0xFFFFFF), 0);

  Serial.println("Setup done");
   uint32_t flashSize = ESP.getFlashChipSize();
  Serial.print("Flash Size: ");
  Serial.print(flashSize / (1024 * 1024));
  Serial.println(" MB");

  // PSRAM availability
  if (psramFound()) {
    Serial.println("PSRAM: Found");
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024));
    Serial.println(" MB");
  } else {
    Serial.println("PSRAM: Not found");
  }
}

void loop()
{
  lv_tick_inc(5);

  if (millis() - lastUpdate > 2000)
  {
    lastUpdate = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t))
    {
      snprintf(temp_buf, sizeof(temp_buf), "%.1f °C", t);
      snprintf(hum_buf, sizeof(hum_buf), "%.1f %%", h);

      lv_label_set_text(label_temp_value, temp_buf);
      lv_label_set_text(label_hum_value, hum_buf);

      Serial.printf("Temp: %.1f °C, Hum: %.1f %%\n", t, h);
    }
    else
    {
      Serial.println("Failed to read from DHT sensor (NaN)!");
    }
  }

  lv_timer_handler();
  delay(5);
}