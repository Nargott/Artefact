#include <RFM69.h>
#include <WS2812FX.h>

#define RFM_MOSI 11
#define RFM_MISO 12
#define RFM_SCK 13
#define RFM_NSS 8
#define RFM_INT 2 //INT 0
#define LED_PIN 6
#define IRF_SENSOR 9
#define CHARGER_DIVIDER_PIN A0

#define LED_COUNT 4

#define DETECT_CHARGER_VOLTAGE 50 //when we detect, that charger is attached

//radio params
#define NODEID        1
#define RECEIVER      2
#define NETWORKID     100
#define FREQUENCY     RF69_433MHZ

//modes
#define MODE_FIRST 0
#define MODE_SECOND 1

#define SEND_INTERVAL 100 //how often packets will be sent
#define ACTIVE_TIMEOUT 10 * 1000 //10 seconds to shut down LEDs if nobody moves around

//time managment
unsigned long curTime = 0;
unsigned long lastSendTime = 0;
unsigned long lastModeChangeTime = 0;
unsigned long lastActiveAt = 0;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); //init LEDs
RFM69 radio(RFM_NSS, RFM_INT, true); //init radio

int mode, prevMode;
void setMode(int newMode) {
  prevMode = mode;
  mode = newMode;
  lastModeChangeTime = millis();

  switch(mode) {
    case MODE_FIRST: //common rainbow effect
      ws2812fx.setSpeed(100);
      ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
      ws2812fx.start();
    break;
    case MODE_SECOND: //charge flicker
      ws2812fx.setSpeed(80);
      ws2812fx.setMode(FX_MODE_FIRE_FLICKER);
      ws2812fx.start();
    break;
  }
}

void setup() {
  pinMode(IRF_SENSOR, INPUT); //init Dopler sensor pin as input

  Serial.begin(9600); //init debug Serial

  if (!radio.initialize(FREQUENCY, NODEID, NETWORKID)) { //init radio
    Serial.println("radio is not initialised");
  }
  Serial.println("radio is OK!");

  ws2812fx.init(); //init LEDs
  ws2812fx.setBrightness(50); //set LEDs Brightness

  setMode(MODE_FIRST); //set first (rainbow) mode
}

byte ackCount=0;
uint32_t packetCount = 0;
void loop() {
  curTime = millis();
  ws2812fx.service(); //process LEDs animations

  if ((curTime - lastSendTime) > SEND_INTERVAL) { //time to send something

    if (analogRead(CHARGER_DIVIDER_PIN) > DETECT_CHARGER_VOLTAGE) { //check if not charged to switch modes
      if (mode == MODE_FIRST) setMode(MODE_SECOND);
    } else if (mode != MODE_FIRST) {
      setMode(MODE_FIRST);
    }
    
    lastSendTime = curTime;
    radio.send(RECEIVER, (const void*) ('A'), 1); //send A letter via radio
  }

  if (mode == MODE_FIRST) {
    if ((digitalRead(IRF_SENSOR) == HIGH) && ((lastActiveAt == 0) || ((curTime - lastActiveAt) > ACTIVE_TIMEOUT))) { //activate a LEDs
      Serial.println("Activate");
      
      lastActiveAt = curTime;
      if (!ws2812fx.isRunning()) {
        ws2812fx.start();
      }
    }

    if ((digitalRead(IRF_SENSOR) == LOW) && ((curTime - lastActiveAt) > ACTIVE_TIMEOUT) && ws2812fx.isRunning()) { //disactivate a LEDs
      Serial.print("Disactivate on: "); Serial.println(digitalRead(IRF_SENSOR));
      ws2812fx.stop();
      ws2812fx.strip_off();
    }
  }
}