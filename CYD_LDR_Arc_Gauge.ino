/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-lvgl-ebook/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*  Install the "lvgl" library version 9.X by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// RGB LED Pins
#define CYD_LED_RED 4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE 17
#define CYD_BACKLIGHT 21

// Global LED states
bool redLEDstate = true;
bool greenLEDstate = true;
bool blueLEDstate = true;

// LDR pin
#define LDR_PIN 34


// Definie SOC arc min/max
#define LDR_ARC_MIN 0
#define LDR_ARC_MAX 100

// Define arc colors (in hex) for SOC states
#define SOC_UPPER_COLOR 0x0A7005
#define SOC_HIGH_COLOR 0x089C84
#define SOC_MID_COLOR 0x829C49
#define SOC_LOW_COLOR 0x9C733F
#define SOC_CRITICAL_COLOR 0x9C3030

static lv_obj_t *home_screen;
static lv_obj_t *settings_screen;
static lv_obj_t *black_screen;
lv_obj_t* arc;
lv_obj_t * toggle_switch_red;
lv_obj_t * toggle_switch_green;
lv_obj_t * toggle_switch_blue;

void lv_create_home_screen(void);
void lv_create_settings_screen(void);
void lv_create_black_screen(void);

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char* buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void sync_toggle_btn_and_LED_states() {
  if(lv_obj_get_state(toggle_switch_red) == redLEDstate) {
        if(redLEDstate == true) {
          lv_obj_remove_state(toggle_switch_red, LV_STATE_CHECKED);
        } else {
          lv_obj_add_state(toggle_switch_red, LV_STATE_CHECKED);
        }
      }
  if(lv_obj_get_state(toggle_switch_green) == greenLEDstate) {
        if(greenLEDstate == true) {
          lv_obj_remove_state(toggle_switch_green, LV_STATE_CHECKED);
        } else {
          lv_obj_add_state(toggle_switch_green, LV_STATE_CHECKED);
        }
      }
  if(lv_obj_get_state(toggle_switch_blue) == blueLEDstate) {
        if(blueLEDstate == true) {
          lv_obj_remove_state(toggle_switch_blue, LV_STATE_CHECKED);
        } else {
          lv_obj_add_state(toggle_switch_blue, LV_STATE_CHECKED);
        }
      }
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t* indev, lv_indev_data_t* data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();

    // Advanced Touchscreen calibration, LEARN MORE » https://RandomNerdTutorials.com/touchscreen-calibration/
    float alpha_x, beta_x, alpha_y, beta_y, delta_x, delta_y;

    // REPLACE WITH YOUR OWN CALIBRATION VALUES » https://RandomNerdTutorials.com/touchscreen-calibration/
    alpha_x = -0.000;
    beta_x = 0.090;
    delta_x = -33.771;
    alpha_y = 0.066;
    beta_y = 0.000;
    delta_y = -14.632;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    // clamp x between 0 and SCREEN_WIDTH - 1
    x = max(0, x);
    x = min(SCREEN_WIDTH - 1, x);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    // clamp y between 0 and SCREEN_HEIGHT - 1
    y = max(0, y);
    y = min(SCREEN_HEIGHT - 1, y);

    // Basic Touchscreen calibration points with map function to the correct width and height
    //x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    //y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void home_screen_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      lv_obj_del(home_screen);
      lv_create_settings_screen();
      sync_toggle_btn_and_LED_states();
    }
    break;

    default: break;
  }
}

static void settings_screen_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      lv_obj_del(settings_screen);
      lv_create_home_screen();
    }
    break;

    default: break;
  }
}

static void go_to_black_screen_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      lv_obj_del(settings_screen);
      lv_create_black_screen();
      // Inverted logic for backlight
      digitalWrite(CYD_BACKLIGHT, LOW);
    }
    break;

    default: break;
  }
}

static void black_screen_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      lv_obj_del(black_screen);
      lv_create_home_screen();
      digitalWrite(CYD_BACKLIGHT, HIGH);
    }
    break;

    default: break;
  }
}

// Callback that is triggered when toggle_switch_red is clicked/toggled
static void toggle_switch_red_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * toggle_switch_red = (lv_obj_t*) lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    // Cheap Yellow Display built-in RGB LED is controlled with inverted logic
    redLEDstate = !lv_obj_has_state(toggle_switch_red, LV_STATE_CHECKED);
    digitalWrite(CYD_LED_RED, redLEDstate);
    LV_UNUSED(toggle_switch_red);
    LV_LOG_USER("State: %s", lv_obj_has_state(toggle_switch_red, LV_STATE_CHECKED) ? "On" : "Off");
  }
}


// Callback that is triggered when the toggle_switch_green switch is clicked/toggled
static void toggle_switch_green_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * toggle_switch_green = (lv_obj_t*) lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    // Cheap Yellow Display built-in RGB LED is controlled with inverted logic
    greenLEDstate = !lv_obj_has_state(toggle_switch_green, LV_STATE_CHECKED);
    digitalWrite(CYD_LED_GREEN, greenLEDstate);
    LV_UNUSED(toggle_switch_green);
    LV_LOG_USER("State: %s", lv_obj_has_state(toggle_switch_green, LV_STATE_CHECKED) ? "On" : "Off");
  }
}

// Callback that is triggered when the toggle_switch_blue switch is clicked/toggled
static void toggle_switch_blue_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * toggle_switch_blue = (lv_obj_t*) lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    // Cheap Yellow Display built-in RGB LED is controlled with inverted logic
    blueLEDstate = !lv_obj_has_state(toggle_switch_blue, LV_STATE_CHECKED);
    digitalWrite(CYD_LED_BLUE, blueLEDstate);
    LV_UNUSED(toggle_switch_blue);
    LV_LOG_USER("State: %s", lv_obj_has_state(toggle_switch_blue, LV_STATE_CHECKED) ? "On" : "Off");
  }
}

// Set the LDR value in the arc and text label
static void set_ldr_value(void* text_label_luminosity_value, int32_t v) {
  // Replace this with the SOC reading (The actual range is 1800.
  // I just changed it to make it more responsive in standard lighting conditions)
  int ldr_value = map(analogRead(LDR_PIN), 1080, 0, 0, 100);

  if (ldr_value <= 20.0) {
    lv_obj_set_style_text_color((lv_obj_t*)text_label_luminosity_value, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SOC_CRITICAL_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(SOC_CRITICAL_COLOR), LV_PART_KNOB);
  } else if (ldr_value > 20.0 && ldr_value <= 40) {
    lv_obj_set_style_text_color((lv_obj_t*)text_label_luminosity_value, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SOC_LOW_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(SOC_LOW_COLOR), LV_PART_KNOB);
  } else if (ldr_value > 40.0 && ldr_value <= 60) {
    lv_obj_set_style_text_color((lv_obj_t*)text_label_luminosity_value, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SOC_MID_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(SOC_MID_COLOR), LV_PART_KNOB);
  } else if (ldr_value > 60.0 && ldr_value <= 80) {
    lv_obj_set_style_text_color((lv_obj_t*)text_label_luminosity_value, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SOC_HIGH_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(SOC_HIGH_COLOR), LV_PART_KNOB);
  } else {
    lv_obj_set_style_text_color((lv_obj_t*)text_label_luminosity_value, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(SOC_UPPER_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(SOC_UPPER_COLOR), LV_PART_KNOB);
  }

  lv_arc_set_value(arc, map(int(ldr_value), LDR_ARC_MIN, LDR_ARC_MAX, 0, 100));

  String ldr_value_text = String(ldr_value) + "%";
  lv_label_set_text((lv_obj_t*)text_label_luminosity_value, ldr_value_text.c_str());
  Serial.print("Luminosity: ");
  Serial.println(ldr_value_text);
}

void lv_create_home_screen(void) {
  home_screen = lv_obj_create(NULL);
  lv_scr_load(home_screen);

  lv_obj_t * go_to_settings_btn = lv_btn_create(home_screen);
  lv_obj_align(go_to_settings_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_add_event_cb(go_to_settings_btn, home_screen_event_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_t * go_to_settings_btn_label = lv_label_create(go_to_settings_btn);
  lv_label_set_text(go_to_settings_btn_label, LV_SYMBOL_SETTINGS);
  lv_obj_align_to(go_to_settings_btn_label, go_to_settings_btn, LV_ALIGN_CENTER, 0, 0);
  // Create an Arc
  arc = lv_arc_create(lv_screen_active());
  lv_obj_set_size(arc, 210, 210);
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);
  lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);

  // Create a text label in font size 32 to display the latest LDR reading
  lv_obj_t* text_label_luminosity_value = lv_label_create(home_screen);
  lv_label_set_text(text_label_luminosity_value, "--.--");
  lv_obj_set_style_text_align(text_label_luminosity_value, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label_luminosity_value, LV_ALIGN_CENTER, 0, 10);
  static lv_style_t style_ldr;
  lv_style_init(&style_ldr);
  lv_style_set_text_font(&style_ldr, &lv_font_montserrat_32);
  lv_obj_add_style(text_label_luminosity_value, &style_ldr, 0);

  // Create an animation to update with the latest LDR value every .1 seconds
  lv_anim_t a_ldr;
  lv_anim_init(&a_ldr);
  lv_anim_set_exec_cb(&a_ldr, set_ldr_value);
  lv_anim_set_duration(&a_ldr, 1000);
  lv_anim_set_playback_duration(&a_ldr, 1000);
  lv_anim_set_var(&a_ldr, text_label_luminosity_value);
  lv_anim_set_values(&a_ldr, 0, 100);
  lv_anim_set_repeat_count(&a_ldr, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a_ldr);
}

void lv_create_settings_screen(void)
{
  settings_screen = lv_obj_create(NULL);
  lv_scr_load(settings_screen);

  lv_obj_t * go_to_home_btn = lv_btn_create(settings_screen);
  lv_obj_align(go_to_home_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_add_event_cb(go_to_home_btn, settings_screen_event_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_t * go_to_home_btn_label = lv_label_create(go_to_home_btn);
  lv_label_set_text(go_to_home_btn_label, LV_SYMBOL_HOME);
  lv_obj_align_to(go_to_home_btn_label, go_to_home_btn, LV_ALIGN_CENTER, 0, 0);

  // Button to go to the blank screen
  lv_obj_t * go_to_black_screen_btn = lv_btn_create(settings_screen);
  lv_obj_align(go_to_black_screen_btn, LV_ALIGN_TOP_RIGHT, -10, 80);
  lv_obj_add_event_cb(go_to_black_screen_btn, go_to_black_screen_event_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_t * go_to_black_screen_btn_label = lv_label_create(go_to_black_screen_btn);
  lv_label_set_text(go_to_black_screen_btn_label, "Black\nscreen");
  lv_obj_align_to(go_to_black_screen_btn_label, go_to_black_screen_btn, LV_ALIGN_CENTER, 0, 0);

  // Create a toggle switch (toggle_switch_red)
  toggle_switch_red = lv_switch_create(settings_screen);
  lv_obj_add_event_cb(toggle_switch_red, toggle_switch_red_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(toggle_switch_red, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_align(toggle_switch_red, LV_ALIGN_TOP_LEFT, 10, 25);

  lv_obj_t * toggle_switch_red_label = lv_label_create(settings_screen);
  lv_label_set_text(toggle_switch_red_label, "Red LED");
  lv_obj_set_style_text_color(toggle_switch_red_label, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_align_to(toggle_switch_red_label, toggle_switch_red, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Create a toggle switch (toggle_switch_green)
  toggle_switch_green = lv_switch_create(settings_screen);
  lv_obj_add_event_cb(toggle_switch_green, toggle_switch_green_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(toggle_switch_green, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_align(toggle_switch_green, LV_ALIGN_TOP_LEFT, 10, 105);

  lv_obj_t * toggle_switch_green_label = lv_label_create(settings_screen);
  lv_label_set_text(toggle_switch_green_label, "Green LED");
  lv_obj_set_style_text_color(toggle_switch_green_label, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align_to(toggle_switch_green_label, toggle_switch_green, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Create a toggle switch (toggle_switch_blue)
  toggle_switch_blue = lv_switch_create(settings_screen);
  lv_obj_add_event_cb(toggle_switch_blue, toggle_switch_blue_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(toggle_switch_blue, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_align(toggle_switch_blue, LV_ALIGN_TOP_LEFT, 10, 65);

  lv_obj_t * toggle_switch_blue_label = lv_label_create(settings_screen);
  lv_label_set_text(toggle_switch_blue_label, "Blue LED");
  lv_obj_set_style_text_color(toggle_switch_blue_label, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_align_to(toggle_switch_blue_label, toggle_switch_blue, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

}

void lv_create_black_screen(void) {
  black_screen = lv_obj_create(NULL);
  lv_scr_load(black_screen);

  lv_obj_t * go_back_to_home_btn = lv_btn_create(black_screen);
  lv_obj_align(go_back_to_home_btn, LV_ALIGN_CENTER, 0, 0);
  // SCREEN_WIDTH and SCREEN_HEIGHT must be inverted here as 
  // the original declaration is assuming portrait rotation
  lv_obj_set_size(go_back_to_home_btn, SCREEN_HEIGHT, SCREEN_WIDTH);
  lv_obj_add_event_cb(go_back_to_home_btn, black_screen_event_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_t * go_back_to_home_btn_label = lv_label_create(go_back_to_home_btn);
  lv_label_set_text(go_back_to_home_btn_label, "  Go back to\nHome screen");
  lv_obj_align_to(go_back_to_home_btn_label, go_back_to_home_btn, LV_ALIGN_CENTER, 0, 0);
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  // Turn all LEDs off at startup (inverted logic for CYD)
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);

  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t* disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI
  lv_create_home_screen();
}

void loop() {
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}