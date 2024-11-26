#include <stdio.h>
#include <FastLED.h>
#include "OneButton.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "data.h"
#include "OTA.hpp"

///////////////////////////////////////////////////
////// USER EDITABLE SECTION //////////////////////

#define BRIGHTNESS 50
// How many leds are in the strip?
#define NUM_LEDS 30 
#define NUM_LEDS2 20
#define NUM_LEDS3 20
#define DATA_PIN 2
#define DATA_PIN_SIGNAL 16
#define DATA_PIN_BACK 5

const int PIN_DRL = 27;
const int PIN_SIGNALL = 13;
const int PIN_SIGNALR = 18;
const int PIN_HAZARD = 14;
const int PIN_BRAKE = 15;
#define SIGNAL_COLOR CRGB::Orange
#define WelcomeLightSpeed 80
#define ByeSpeed 40
#define FPS 60

#define SIGNAL_DELAY 30000 //in us

#define DELAY_STROBE 300
#define FADE_STROBE 250
// BREATHE animation
#define smoothness 150.0
#define range 250
// #define NUM_STRIPS 4
#define BUTTON_HZ 30

#define DEBUG 1


////// END USER EDITABLE SECTION //////////////////
///////////////////////////////////////////////////

const static int NUM_LEDS_2 = (int)(NUM_LEDS/2);
const static int NUM_LEDS2_2 = (int)(NUM_LEDS2/2);  
const static int NUM_LEDS3_2 = (int)(NUM_LEDS3/2);

const static int NUM_LEDS_4 = (int)(NUM_LEDS/4);
const static int NUM_LEDS2_4 = (int)(NUM_LEDS2/4);
const static int NUM_LEDS3_4 = (int)(NUM_LEDS3/4);

CRGB brightness[4];

static CRGBArray<NUM_LEDS> leds;
static CRGBArray<NUM_LEDS2> leds2;
static CRGBArray<NUM_LEDS3> leds3;

typedef enum SignalState_t{
  OFF,
  ON,
  BREATHE,
  STROBE
} SignalState_t;

SignalState_t state_hazard = OFF;

int counterl = NUM_LEDS;
int counterr = NUM_LEDS;

static bool state_signall = false;
static bool state_signalr = false;
static bool state_DRL = false;
static bool state_brake = false;

bool animating = false;
bool running_mode = false;
bool running_signal = false;
long last_signal_time = 0;
long last_mode_time = 0;
int counterleft = 0;
int counterright = 0;
int counterhazard = 0;

// button object
OneButton *button_hazard = new OneButton(PIN_HAZARD, true, true);
OneButton *button_left = new OneButton(PIN_SIGNALL, true, true);
OneButton *button_right = new OneButton(PIN_SIGNALR, true, true);
OneButton *button_DRL = new OneButton(PIN_DRL, true, true);
OneButton *button_brake = new OneButton(PIN_BRAKE, true, true);
// end button object

// helper functions
float myabs(float x){
    if(x < 0){
        return -1*x;
    }
    return x;
}
// end helper functions

// functions declaration
void init_button();
void update_button(void *args);
void update(void *args);
void welcomeLightScanning(void *args);
void byeLight(void *args);
void hazardDouble();
void hazardMulti(void *);
void buttonStart(void *);
void buttonStop(void *);
void updateSignalLeft(int);
void updateSignalRight(int);
void updateHazard(int);
// end functions declaration

void setup() {
  // esp_core_dump_init();
  // panic_info_t panic;
  // esp_core_dump_to_uart(&panic);
  
  brightness[0] = CRGB::White;
  brightness[1] = CRGB(200,200,200);
  brightness[2] = CRGB(100,100,100);
  brightness[3] = CRGB(0,0,0);
  
  pinMode(DATA_PIN, OUTPUT);
  pinMode(DATA_PIN_SIGNAL, OUTPUT);
  pinMode(DATA_PIN_BACK, OUTPUT);
  pinMode(PIN_BRAKE, INPUT_PULLUP);

  init_button();
  // pinMode(DATA_PIN_BACK, OUTPUT);

  // Serial.begin(115200);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  // set_max_power_in_volts_and_milliamps(5, 1000);
  FastLED.addLeds<WS2812, DATA_PIN_SIGNAL, GRB>(leds2, NUM_LEDS2);
  // set_max_power_in_volts_and_milliamps(5, 1000);
  FastLED.addLeds<WS2812, DATA_PIN_BACK, GRB>(leds3, NUM_LEDS3);
  FastLED.setBrightness(20);
  // set_max_power_in_volts_and_milliamps(5, 100);

  xTaskCreatePinnedToCore(update,"update_led",2048,NULL,2,NULL,1); // 60LEDS, 40 LEDS, 30 LEDS 2ms
  xTaskCreatePinnedToCore(update_button,"update_button",2048,NULL,3,NULL,1); // 20us

}

TickType_t lasttime;
void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelayUntil(&lasttime,(1000/FPS)/portTICK_PERIOD_MS);
  if(animating){

  }
  long timestamp = micros();
  if((counterhazard !=0 || state_hazard == ON) && micros()-last_signal_time > SIGNAL_DELAY){
    updateHazard(counterhazard);
    counterhazard++;
    if(counterhazard==25) counterhazard=0;
    last_signal_time = micros();
  }
  if((state_signall || counterleft !=0) && micros()-last_signal_time > SIGNAL_DELAY){
    updateSignalLeft(counterleft);
    counterleft++;
    if(counterleft==25) counterleft=0;
    last_signal_time = micros();
  }
  if((state_signalr || counterright !=0) && micros()-last_signal_time > SIGNAL_DELAY){
    updateSignalRight(counterright);
    counterright++;
    if(counterright==25) counterright=0;
    last_signal_time = micros();
  }
  // ESP_LOGD("LOOP","time: %ld", micros()-timestamp);
  
}

void init_button(){
  long time = micros();
  ESP_LOGV("init_button","hazard:%d",button_hazard->pin());
  ESP_LOGV("init_button","left:%d",button_left->pin());
  ESP_LOGV("init_button","right:%d",button_right->pin());
  ESP_LOGV("init_button","DRL:%d",button_DRL->pin());
  ESP_LOGV("init_button","brake:%d",button_brake->pin());

  button_hazard->setClickTicks(600);
  button_hazard->setPressTicks(400);
  button_left->setPressTicks(350);
  button_right->setPressTicks(350);
  button_DRL->setPressTicks(500);
  button_brake->setPressTicks(50);

  button_hazard->attachDoubleClick(hazardDouble);
  button_hazard->attachMultiClick(hazardMulti, button_hazard);
  button_hazard->attachLongPressStart(buttonStart, button_hazard);
  button_hazard->attachLongPressStop(buttonStop, button_hazard);

  button_left->attachLongPressStart(buttonStart, button_left);
  button_left->attachLongPressStop(buttonStop, button_left);
  button_right->attachLongPressStart(buttonStart, button_right);
  button_right->attachLongPressStop(buttonStop, button_right);

  button_DRL->attachLongPressStart(buttonStart, button_DRL);
  button_DRL->attachLongPressStop(buttonStop, button_DRL);
  button_brake->attachDuringLongPress(buttonStart, button_brake);
  button_brake->attachLongPressStop(buttonStop, button_brake);
  ESP_LOGD("init_button","time:%ld", micros()-time);
}

void IRAM_ATTR update(void *args){
  long time;
  for(;;){
    time = micros();
    if(state_brake)
      leds3.fill_solid(CRGB::Red);
    else if(!state_DRL && !state_signall && !state_signalr && state_hazard == OFF){
      leds3.fill_solid(CRGB::Black);
    }
    if(!animating)
      FastLED.show();
    ESP_LOGV("UPDATE_LED","time: %ld", (micros()-time));
    vTaskDelay((1000/FPS)/portTICK_PERIOD_MS);
  }
}

void update_button(void *args){
  long time;
  for(;;){
    time = micros();
    button_hazard->tick();
    button_left->tick();
    button_right->tick();
    button_DRL->tick();
    button_brake->tick();
    ESP_LOGV("UPDATE_BUTTON", "time : %ld", (micros()-time));
    vTaskDelay((1000 / BUTTON_HZ) / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void hazardDouble(){
  if(state_hazard != BREATHE){
    state_hazard = BREATHE;  
    ESP_LOGD("BREATHE", "hazard BreathE ON");
  }
  else{
    state_hazard = OFF;
    ESP_LOGD("BREATHE", "hazard BreathE OFF");
    // leds.fill_solid(CRGB::Black);
    // leds2.fill_solid(CRGB::Black);
    // leds3.fill_solid(CRGB::Black);
  }
}

void startOTA(void *args){
    startServerOTA();
    delay(10);
    xTaskCreatePinnedToCore(otaTask,"OTA_TASK",4096,NULL,2,NULL,0);
    vTaskDelete(NULL);
}

void hazardMulti(void *oneButton){
  OneButton *button = (OneButton *)oneButton;
  ESP_LOGI("Multi","I clicked %d times", button->getNumberClicks());
  if(button->getNumberClicks() == 8){
    xTaskCreatePinnedToCore(startOTA,"OTA",4096,NULL,2,NULL,0);
  }
  return;
  if(state_hazard != STROBE){
    state_hazard = STROBE;  
    ESP_LOGD("STROBE", "hazard STROBE ON");
  }
  else{
    state_hazard = OFF;
    ESP_LOGD("STROBE", "hazard STROBE OFF");
    // leds.fill_solid(CRGB::Black);
    // leds2.fill_solid(CRGB::Black);
    // leds3.fill_solid(CRGB::Black);
  }
}

void buttonStart(void *oneButton){
  OneButton *button = (OneButton *)oneButton;
  ESP_LOGD("BUTTON","pin: %d",(int)((*button).pin()));
  switch(button->pin()){
    case PIN_BRAKE:
      state_brake = true;
      break;
    case PIN_HAZARD:
      if(state_hazard == OFF) state_hazard = ON;
      break;
    case PIN_SIGNALL:
      state_signall = true;
      break;
    case PIN_SIGNALR:
      state_signalr = true;
      break;
    case PIN_DRL:
      while(animating){
        ESP_LOGD("DRL","waiting for animate finish");
        vTaskDelay(5);
      }
      animating = true;
      xTaskCreatePinnedToCore(welcomeLightScanning,"welcomelight",2048,NULL,2,NULL,0);
      state_DRL = true;
      break;
    default:
      ESP_LOGW("Button","NOT A VALID PIN %d", button->pin());
      break;
  }
}

void buttonStop(void *oneButton){
  OneButton *button = (OneButton *)oneButton;
  ESP_LOGD("BUTTON","pin: %d",(int)button->pin());
  switch(button->pin()){
    case PIN_BRAKE:
      state_brake = false;
      break;
    case PIN_HAZARD:
      state_hazard = OFF;
      break;
    case PIN_SIGNALL:
      state_signall = false;
      break;
    case PIN_SIGNALR:
      state_signalr = false;
      break;
    case PIN_DRL:
      while(animating){
        ESP_LOGD("DRL","waiting for animate finish");
        vTaskDelay(5);
      }
      animating = true;
      xTaskCreatePinnedToCore(byeLight,"byeLight",4096,NULL,2,NULL,0);
      state_DRL = false;
      break;
    default:
      ESP_LOGW("Button","NOT A VALID PIN %d", button->pin());
      break;
  }
}

void welcomeLightScanning(void *args){
  ESP_LOGD("WELCOME","welcomeLightScanning\n");
  // scanning one pixel both directions to center of the strip then back to outer edges
  for (int i = 0; i < NUM_LEDS_2; i++)
  {
    ESP_LOGD("WELCOME","i: %d\n", i);
    leds[i] = CRGB::Gray;
    leds[NUM_LEDS - i - 1] = CRGB::Gray;
    leds[i-1] = CRGB::Black;
    leds[NUM_LEDS - i] = CRGB::Black;
    if (i < NUM_LEDS2_2)
    {
      leds2[i] = CRGB::Gray;
      leds2[NUM_LEDS2 - i - 1] = CRGB::Gray;
      leds2[i-1] = CRGB::Black;
      leds2[NUM_LEDS2 - i] = CRGB::Black;
    }
    if (i < NUM_LEDS3_2 && !state_brake)
    {
      leds3[i] = CRGB::Gray;
      leds3[NUM_LEDS3 - i - 1] = CRGB::Gray;
      leds3[i-1] = CRGB::Black;
      leds3[NUM_LEDS3 - i] = CRGB::Black;
    }
    FastLED.show();
    vTaskDelay(WelcomeLightSpeed / portTICK_PERIOD_MS);
  }
  // reverse direction
  ESP_LOGD("WELCOME","reverse direction\n");
  for(int i = NUM_LEDS_2; i > 0; i--){
    leds[i] = CRGB::Gray;
    leds[NUM_LEDS - i + 1 ] = CRGB::Gray;
    leds[NUM_LEDS - i] = CRGB::Black;
    leds[i+1] = CRGB::Black; 
    if(i < NUM_LEDS2_2){
      leds2[i] = CRGB::Gray;
      leds2[NUM_LEDS2 - i - 1] = CRGB::Gray;
      leds2[NUM_LEDS2 - i ] = CRGB::Black;
      leds2[i+1] = CRGB::Black;
    }
    if(i < NUM_LEDS3_2 && !state_brake){
      leds3[i] = CRGB::Gray;
      leds3[NUM_LEDS3 - i - 1] = CRGB::Gray;
      leds3[NUM_LEDS3 - i ] = CRGB::Black;
      leds3[i+1] = CRGB::Black;
    }
    FastLED.show();
    vTaskDelay(WelcomeLightSpeed / portTICK_PERIOD_MS);
  }

  ESP_LOGD("WELCOME","1st fill\n");
  for (int i = 0; i < NUM_LEDS_2; i++)
  {
    leds[i] = CRGB::Gray;
    leds[NUM_LEDS - i - 1] = CRGB::Gray;
    if (i < NUM_LEDS2_2)
    {
      leds2[i] = CRGB::Gray;
      leds2[NUM_LEDS2 - i - 1] = CRGB::Gray;
    }
    if (i < NUM_LEDS3_2 && !state_brake)
    {
      leds3[i] = CRGB::Gray;
      leds3[NUM_LEDS3 - i - 1] = CRGB::Gray;
    }
    FastLED.show();
    vTaskDelay(WelcomeLightSpeed / portTICK_PERIOD_MS);
  }
  ESP_LOGD("WELCOME","2nd fill\n");
  for(int i = NUM_LEDS_2; i > 0; i--){
    leds[i] = CRGB::White;
    leds[NUM_LEDS - i + 1 ] = CRGB::White;
    if(i < NUM_LEDS2_2){
      leds2[i] = CRGB::White;
      leds2[NUM_LEDS2 - i - 1] = CRGB::White;
    }
    if(i < NUM_LEDS3_2 && !state_brake){
      leds3[i] = CRGB::White;
      leds3[NUM_LEDS3 - i - 1] = CRGB::White;
    }
    FastLED.show();
    vTaskDelay(WelcomeLightSpeed / portTICK_PERIOD_MS);
  }
  animating = false;
  ESP_LOGD("WELCOME","Done\n");
  vTaskDelete(NULL);
}

void byeLight(void *args){
  for(int i = 1; i < 4; i++){
    for(int j = 0; j < NUM_LEDS_2; j++){
      TickType_t now = xTaskGetTickCount();
      leds[NUM_LEDS_2 - j-1] = brightness[i];
      leds[NUM_LEDS_2 + j] = brightness[i];
      if(j < NUM_LEDS2_2){
        leds2[NUM_LEDS2_2 - j-1] = brightness[i];
        leds2[NUM_LEDS2_2 + j] = brightness[i];
        // leds3[NUM_LEDS2_2 - j-1] = brightness[i];
        // leds3[NUM_LEDS2_2 + j] = brightness[i];
      }
      if(j < NUM_LEDS3_2){
        leds3[NUM_LEDS3_2 - j-1] = brightness[i];
        leds3[NUM_LEDS3_2 + j] = brightness[i];
      }
      // FastLED.show();
      vTaskDelay(ByeSpeed / portTICK_PERIOD_MS);
      ESP_LOGD("BYE","Time: %d\n", xTaskGetTickCount() - now);
    }
  }
  animating = false;
  vTaskDelete(NULL);
}

void updateSignalLeft(int counter){
  for(int i = 0; i < NUM_LEDS_2; i ++){
    leds[i].r = data_signal[counter][i][0];
    // ESP_LOGI("DATA","counter %d i %d r : %d",counter, i, data_signal[counter][i][0]);
    leds[i].g = data_signal[counter][i][1];
    leds[i].b = data_signal[counter][i][2];
    if(i < NUM_LEDS2_2){
      leds2[i].r = data_signal[counter][i][0];
      leds2[i].g = data_signal[counter][i][1];
      leds2[i].b = data_signal[counter][i][2];
    }
    if(!state_brake){
      if(i < NUM_LEDS3_2){
        leds3[i].r = data_signal[counter][i][0];
        leds3[i].g = data_signal[counter][i][1];
        leds3[i].b = data_signal[counter][i][2];
      }
    }
  }
}

void updateSignalRight(int counter){
  for(int i = 0; i < NUM_LEDS_2; i ++){
    leds[i+NUM_LEDS_2].r = data_signal[counter][NUM_LEDS_2-i-1][0];
    // ESP_LOGI("DATA","counter %d i %d r : %d",counter, i, data_signal[counter][i+NUM_LEDS_2][0]);
    leds[i+NUM_LEDS_2].g = data_signal[counter][NUM_LEDS_2-i-1][1];
    leds[i+NUM_LEDS_2].b = data_signal[counter][NUM_LEDS_2-i-1][2];
    if(i < NUM_LEDS2_2){
      leds2[i+NUM_LEDS2_2].r = data_signal[counter][NUM_LEDS_2-i-1][0];
      leds2[i+NUM_LEDS2_2].g = data_signal[counter][NUM_LEDS_2-i-1][1];
      leds2[i+NUM_LEDS2_2].b = data_signal[counter][NUM_LEDS_2-i-1][2];
    }
    if(!state_brake){
      if(i < NUM_LEDS3_2){
        leds3[i+NUM_LEDS3_2].r = data_signal[counter][NUM_LEDS_2-i-1][0];
        leds3[i+NUM_LEDS3_2].g = data_signal[counter][NUM_LEDS_2-i-1][1];
        leds3[i+NUM_LEDS3_2].b = data_signal[counter][NUM_LEDS_2-i-1][2];
      }   
    }

  }
}

void updateHazard(int counter){
  updateSignalLeft(counter);
  updateSignalRight(counter);
}