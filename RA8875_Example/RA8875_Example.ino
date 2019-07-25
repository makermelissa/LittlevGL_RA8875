
#include "SPI.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"
#include "lvgl.h"


// This is calibration data for the raw touch data to the screen coordinates
// In the future, we should calibrate the display rather than hardcode and store the values in eeprom
/*
*                 TS_MAXX
*           --------------------
*           |                  |
*           |                  |
*           |                  |
*           |                  |
*           |                  |
*  TS_MINY  |                  |  TS_MAXY
*           |                  |
*           |                  |
*           |                  |
*           |                  |
*           |                  |
*           --------------------
*                 TS_MINX
*/

#define TS_MINX 300
#define TS_MINY 150
#define TS_MAXX 3700
#define TS_MAXY 3900

#define MINPRESSURE 40
#define MAXPRESSURE 1000

#define TFT_INT 16
#define TFT_CS 5
#define TFT_RST 17

lv_obj_t * globalLabel;

Adafruit_RA8875 tft = Adafruit_RA8875(TFT_CS, TFT_RST);

void my_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array) {
  uint16_t color;
  int32_t row_count = x2 - x1 + 1;
  uint16_t color_row[LV_HOR_RES]; // Maximum size should be the screen width
  
  for (int y = y1; y <= y2; y++) {
    // Copy and convert the line of X pixels into row array
    for (int x = 0; x < row_count; x++) {
      color_row[x] = color565(color_array->red, color_array->green, color_array->blue);
      color_array++;
    }
    
    // call tft.drawPixels();
    tft.drawPixels(color_row, row_count, x1, y);
  }
  //tft.fillRoundRect(200, 10, 200, 100, 10, RA8875_RED);
  lv_flush_ready();
}

void my_disp_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_p)
{
  uint16_t color;
  int32_t row_count = x2 - x1 + 1;
  uint16_t color_row[LV_HOR_RES]; // Maximum size should be the screen width
  
  for (int y = y1; y <= y2; y++) {
    // Copy and convert the line of X pixels into row array
    for (int x = 0; x < row_count; x++) {
      color_row[x] = color565(color_p->red, color_p->green, color_p->blue);
      color_p++;
    }
    
    tft.drawPixels(color_row, row_count, x1, y);
  }
}

void my_disp_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
  uint16_t color_value = color565(color.red, color.green, color.blue);
  tft.fillRect(x1, y1, x2, y2, color_value);
}

bool my_tp_read(lv_indev_data_t *data) {
  bool tp_is_pressed = !digitalRead(TFT_INT) && tft.touched();
  int16_t last_x = 0;
  int16_t last_y = 0;
  uint16_t tx, ty;
  float xScale = 1024.0F/tft.width();
  float yScale = 1024.0F/tft.height();
  lv_label_set_text(globalLabel, "Not Pressed");
  if (tp_is_pressed) {
    // Touch pad is being pressed now
    tft.touchRead(&tx, &ty);
    last_x = (uint16_t)(tx/xScale);
    last_y = (uint16_t)(ty/yScale);
    
    lv_label_set_text(globalLabel, "Pressed");

    //tft.fillCircle(last_x, last_y, 4, RA8875_RED);
  }
  
  data->point.x = last_x;
  data->point.y = last_y;
  data->state = tp_is_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

  return false;       /*Return false because no moare to be read*/
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

static lv_res_t btn_click_action(lv_obj_t * btn) {
    uint8_t id = lv_obj_get_free_num(btn);
lv_label_set_text(globalLabel, "Released");
    printf("Button %d is released\n", id);

    /* The button is released.
     * Make something here */

    return LV_RES_OK; /*Return OK if the button is not deleted*/
}

void setup() {
  Serial.begin(9600);
  Serial.print("Initializing..."); 
  // put your setup code here, to run once:
    // Initialize LCD
    tft.begin(RA8875_800x480);
    //tft.clearScreen();
    // origin = left,top landscape (USB left upper)

    tft.displayOn(true);
    tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
    tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
    tft.PWM1out(255);
    
    tft.fillScreen(RA8875_BLACK);

    pinMode(TFT_INT, INPUT);
    digitalWrite(TFT_INT, HIGH);
    tft.touchEnable(true);
    //tft.fillRoundRect(200, 10, 200, 100, 10, RA8875_RED);

    /*tft.gPrint(30,30,"This is a test!",RA8875_WHITE,1,&Imagine_FontFixed__15);
    // Initialize Touch Screen
    if (!ts.begin()) {
        tft.gPrint(0,0,"Error: Touchscreen controller not found!",RA8875_WHITE,1,&Imagine_FontFixed__15);
        while(1);
    }

    tft.setForegroundColor(RA8875_YELLOW);
    tft.drawFlashImage(304, 0, 16, 16, * icon_wifi);*/
    lv_init();

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    disp_drv.disp_flush = my_disp_flush;
    //disp_drv.disp_fill = my_disp_fill;/*Fill an area in the frame buffer*/
    //disp_drv.disp_map = my_disp_map;/*Copy a color_map (e.g. image) into the frame buffer*/
    lv_disp_drv_register(&disp_drv);

    /*Initialize the touch pad*/
    lv_indev_drv_t indev_drv;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read = my_tp_read;
    lv_indev_drv_register(&indev_drv);

/********************************************/
/*Create a title label*/
lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
lv_label_set_text(label, "Default buttons");
lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 5);

/*Create a normal button*/
lv_obj_t * btn1 = lv_btn_create(lv_scr_act(), NULL);
lv_cont_set_fit(btn1, true, true); /*Enable resizing horizontally and vertically*/
lv_obj_align(btn1, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
lv_obj_set_free_num(btn1, 1);   /*Set a unique number for the button*/
lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, btn_click_action);

/*Add a label to the button*/
label = lv_label_create(btn1, NULL);
lv_label_set_text(label, "Normal");

/*Copy the button and set toggled state. (The release action is copied too)*/
lv_obj_t * btn2 = lv_btn_create(lv_scr_act(), btn1);
lv_obj_align(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
lv_btn_set_state(btn2, LV_BTN_STATE_TGL_REL);  /*Set toggled state*/
lv_obj_set_free_num(btn2, 2);               /*Set a unique number for the button*/

/*Add a label to the toggled button*/
label = lv_label_create(btn2, NULL);
lv_label_set_text(label, "Toggled");

/*Copy the button and set inactive state.*/
lv_obj_t * btn3 = lv_btn_create(lv_scr_act(), btn1);
lv_obj_align(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
lv_btn_set_state(btn3, LV_BTN_STATE_INA);   /*Set inactive state*/
lv_obj_set_free_num(btn3, 3);               /*Set a unique number for the button*/

/*Add a label to the inactive button*/
label = lv_label_create(btn3, NULL);
lv_label_set_text(label, "Inactive");

globalLabel = lv_label_create(lv_scr_act(), NULL);
lv_label_set_text(globalLabel, "Test");
lv_obj_align(globalLabel, btn3, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void loop() {
  // put your main code here, to run repeatedly:
  lv_task_handler();
  delay(5);
}
