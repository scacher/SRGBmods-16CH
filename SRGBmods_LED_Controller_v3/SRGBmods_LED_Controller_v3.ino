// SRGBmods LED Controller v3 Firmware - 16通道独立版 (修复编译错误)
// 完美支持 signalRGB，修复 USB 死锁，带开机彩虹灯效
// 适用板子：Waveshare RP2040 Plus / Raspberry Pi Pico 等

#if !defined ARDUINO_RASPBERRY_PI_PICO && !defined ARDUINO_ADAFRUIT_FEATHER_RP2040_SCORPIO && !defined ARDUINO_WAVESHARE_RP2040_PLUS
  #error ONLY RP2040-based boards are supported!
#endif

#if !defined USE_TINYUSB
  #error Please select "Adafruit TinyUSB" as "USB Stack" in "Tools"!
#endif

#include <EEPROM.h>
#include <Adafruit_NeoPXL8.h>       // 原版库 (自动使用 PIO0)
#include <Adafruit_NeoPXL8_PIO1.h>  // 魔改库 (强制使用 PIO1)

const byte firmwareVersion[64] = { 1, 3, 0 }; 

// ==================== USB HID ====================
uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_GENERIC_INOUT(64) };
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 1, true);

// ==================== 16 通道统一配置 ====================
#define NUM_CHANNELS      16
#define CHANNELS_PER_PIO  8

const int8_t channelPins[NUM_CHANNELS] = {
  10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25
};

const int channelLedCount[NUM_CHANNELS] = {
  40, 40, 40, 40, 40, 40, 40, 40,   
  40, 40, 40, 40, 40, 40, 40, 40    
};

const int maxLeds_pio0 = 40;  
const int maxLeds_pio1 = 40;  

int channelStart[NUM_CHANNELS];
int totalLedCount = 0;

void buildChannelLayout() {
  totalLedCount = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    channelStart[i] = totalLedCount;
    totalLedCount += channelLedCount[i];
  }
}

int8_t pins_pio0[CHANNELS_PER_PIO];
int8_t pins_pio1[CHANNELS_PER_PIO];

void buildPinArrays() {
  for (int i = 0; i < CHANNELS_PER_PIO; i++) {
    pins_pio0[i] = channelPins[i];
    pins_pio1[i] = channelPins[i + CHANNELS_PER_PIO];
  }
}

Adafruit_NeoPXL8       leds_pio0(maxLeds_pio0, pins_pio0, NEO_GRB);
Adafruit_NeoPXL8_PIO1  leds_pio1(maxLeds_pio1, pins_pio1, NEO_GRB);

// ==================== 【修复】核心颜色计算函数 (提前定义) ====================
uint32_t setRGBbrightness(byte r, byte g, byte b, byte brightness) {
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  return (((uint32_t)r & 0xFF) << 16) | (((uint32_t)g & 0xFF) << 8) | ((uint32_t)b & 0xFF);
}

// ==================== 通道操作抽象层 ====================
void globalToChannel(int globalIdx, int &ch, int &localLed) {
  for (int i = NUM_CHANNELS - 1; i >= 0; i--) {
    if (globalIdx >= channelStart[i]) {
      ch = i;
      localLed = globalIdx - channelStart[i];
      return;
    }
  }
  ch = 0; localLed = 0;
}

void setChannelPixel(int ch, int localLed, uint32_t color) {
  if (ch < CHANNELS_PER_PIO) {
    leds_pio0.setPixelColor(ch * maxLeds_pio0 + localLed, color);
  } else {
    leds_pio1.setPixelColor((ch - CHANNELS_PER_PIO) * maxLeds_pio1 + localLed, color);
  }
}

void setGlobalPixel(int globalIdx, uint32_t color) {
  int ch, localLed;
  globalToChannel(globalIdx, ch, localLed);
  setChannelPixel(ch, localLed, color);
}

void showAllChannels() {
  if (leds_pio0.canShow()) leds_pio0.show();
  if (leds_pio1.canShow()) leds_pio1.show();
}

bool canShowAllChannels() {
  return leds_pio0.canShow() && leds_pio1.canShow();
}

void fillAllSolid(byte r, byte g, byte b, byte brightness = 255) {
  uint32_t color = setRGBbrightness(r, g, b, brightness); // 现在可以正常调用了
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int i = 0; i < channelLedCount[ch]; i++) {
      setChannelPixel(ch, i, color);
    }
  }
  showAllChannels();
}

void fillAllRainbow(uint16_t first_hue, int8_t reps, uint8_t sat, uint8_t val, bool gammify = true) {
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    for (int i = 0; i < channelLedCount[ch]; i++) {
      int gIdx = channelStart[ch] + i;
      uint16_t hue = first_hue + (uint32_t)gIdx * reps * 65536 / totalLedCount;
      uint32_t color;
      if (ch < CHANNELS_PER_PIO) {
        color = leds_pio0.ColorHSV(hue, sat, val);
        if (gammify) color = leds_pio0.gamma32(color);
      } else {
        color = leds_pio1.ColorHSV(hue, sat, val);
        if (gammify) color = leds_pio1.gamma32(color);
      }
      setChannelPixel(ch, i, color);
    }
  }
  showAllChannels();
}

// ==================== USB 收发 ====================
byte usbPacket[64];
bool newUSBpacketArrived = false;

const uint8_t BrightFull = 255;
const int ledsPerPacket = 20;

const byte rainbowColors[7][3] = {
  {255,0,0},{255,37,0},{255,255,0},{0,128,0},{128,128,0},{0,0,200},{75,0,130}
};

unsigned long lastPacketRcvd;
bool DataLedOn = false;
bool hardwareLighting = false;
unsigned long lastHWLUpdate;

int packetCount = 0;
int currentGlobalLed = 0;   
bool updateChannel = false;

// ==================== EEPROM 配置 ====================
const int eeprom_HWL_enable = 0;
const int eeprom_HWL_return = 1;
const int eeprom_HWL_returnafter = 2;
const int eeprom_HWL_effectMode = 3;
const int eeprom_HWL_effectSpeed = 4;
const int eeprom_HWL_brightness = 5;
const int eeprom_HWL_color_r = 6;
const int eeprom_HWL_color_g = 7;
const int eeprom_HWL_color_b = 8;
const int eeprom_StatusLED_enable = 9;
const int eeprom_ColorCompression_enable = 10;

byte HWL_enable, HWL_return, HWL_returnafter;
byte HWL_effectMode, HWL_effectSpeed, HWL_brightness;
byte HWL_singleColor[3];
byte StatusLED_enable, ColorCompression_enable;

bool Core0ready = false;
bool picoSuspended = false;

// ==================== 函数前置声明 ====================
void Setup_SRGBmodsLCv1();
void receiveUSBpacket(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
void HWL_readEEPROM();
void toggleOnboardLED(bool state);
void updateLighting();
void handleHWLighting();
void resetLighting();
void processUSBpacket();
void colorLeds(uint8_t const* usbPacket);

// ==================== Core0：USB ====================
void setup() {
  Setup_SRGBmodsLCv1();
  usb_hid.setReportCallback(NULL, receiveUSBpacket);
  usb_hid.begin();

  unsigned long startTime = millis();
  while (!TinyUSBDevice.mounted() && (millis() - startTime < 3000)) delay(1);

  Core0ready = true;
}

void loop() { delay(1); }

void Setup_SRGBmodsLCv1() {
  usb_hid.setStringDescriptor("LED Controller v1");
  USBDevice.setID(0x16D0, 0x1205);
  USBDevice.setManufacturerDescriptor("SRGBmods.net");
  USBDevice.setProductDescriptor("LED Controller v1");
}

void receiveUSBpacket(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  if (!newUSBpacketArrived) {
    memcpy(usbPacket, buffer, 64);
    newUSBpacketArrived = true;
  }
}

// ==================== Core1：灯效 ====================
void setup1() {
  while (!Core0ready) delay(1);

  buildChannelLayout();
  buildPinArrays();

  pinMode(LED_BUILTIN, OUTPUT);
  EEPROM.begin(256);
  HWL_readEEPROM();

  leds_pio0.begin();
  leds_pio1.begin();

  for (int i = 0; i < 3; i++) {
    fillAllSolid(255, 0, 0, 80); delay(200);
    fillAllSolid(0,   0, 0, 0);  delay(200);
  }

  hardwareLighting = true;
  lastHWLUpdate = millis();
  lastPacketRcvd = millis();
}

void loop1() {
  if (TinyUSBDevice.suspended()) {
    if (!picoSuspended) {
      picoSuspended = true;
      toggleOnboardLED(false);
      resetLighting();
    }
    return;
  } else {
    if (picoSuspended) picoSuspended = false;
  }

  if (newUSBpacketArrived) processUSBpacket();
  if (updateChannel)       updateLighting();

  if (millis() - lastPacketRcvd >= 500 && DataLedOn) {
    DataLedOn = false;
    toggleOnboardLED(false);
  }

  handleHWLighting();
  delay(1);
}

// ==================== 其他核心功能 ====================
void HWL_readEEPROM() {
  HWL_enable      = EEPROM.read(eeprom_HWL_enable)      == 0xFF ? 0x01 : EEPROM.read(eeprom_HWL_enable);
  HWL_return      = EEPROM.read(eeprom_HWL_return)      == 0xFF ? 0x01 : EEPROM.read(eeprom_HWL_return);
  HWL_returnafter = EEPROM.read(eeprom_HWL_returnafter) == 0xFF ? 0x0A : EEPROM.read(eeprom_HWL_returnafter);
  HWL_effectMode  = EEPROM.read(eeprom_HWL_effectMode)  == 0xFF ? 0x01 : EEPROM.read(eeprom_HWL_effectMode);
  HWL_effectSpeed = EEPROM.read(eeprom_HWL_effectSpeed) == 0xFF ? 0x06 : EEPROM.read(eeprom_HWL_effectSpeed);
  HWL_brightness  = EEPROM.read(eeprom_HWL_brightness)  == 0xFF ? 0x7F : EEPROM.read(eeprom_HWL_brightness);
  HWL_singleColor[0] = EEPROM.read(eeprom_HWL_color_r);
  HWL_singleColor[1] = EEPROM.read(eeprom_HWL_color_g);
  HWL_singleColor[2] = EEPROM.read(eeprom_HWL_color_b);
  StatusLED_enable        = EEPROM.read(eeprom_StatusLED_enable)        == 0xFF ? 0x00 : EEPROM.read(eeprom_StatusLED_enable);
  ColorCompression_enable = EEPROM.read(eeprom_ColorCompression_enable) == 0xFF ? 0x00 : EEPROM.read(eeprom_ColorCompression_enable);
  hardwareLighting = HWL_enable;
}

void toggleOnboardLED(bool state) {
  if (StatusLED_enable || state == false) digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
}

void updateLighting() {
  if (!DataLedOn) { DataLedOn = true; toggleOnboardLED(true); }
  packetCount = 0;
  currentGlobalLed = 0;
  updateChannel = false;
  showAllChannels();
}

void resetLighting() {
  newUSBpacketArrived = false;
  packetCount = 0;
  currentGlobalLed = 0;
  updateChannel = false;
  fillAllSolid(0, 0, 0, 0);
}

void handleHWLighting() {
  unsigned long currentMillis = millis();
  if (HWL_enable == 1 && hardwareLighting == true) {
    int hwleffectspeed = ceil(300.0 / HWL_effectSpeed);
    if (currentMillis - lastHWLUpdate >= (hwleffectspeed < 10 ? 10 : hwleffectspeed)) {
      lastHWLUpdate = currentMillis;
      switch (HWL_effectMode) {
        case 1: { 
          static uint16_t firsthue = 0;
          fillAllRainbow(firsthue -= 256, 10, 255, HWL_brightness > 75 ? HWL_brightness : 75);
          return;
        }
        case 2: { 
          static int currentColor = 0;
          static uint8_t breath_bright = HWL_brightness;
          static bool isDimming = true;
          if (isDimming && (breath_bright - 1 <= 1)) {
            currentColor = (currentColor + 1) > 6 ? 0 : currentColor + 1;
            isDimming = false;
          } else if (!isDimming && (breath_bright + 1 >= HWL_brightness)) {
            isDimming = true;
          }
          breath_bright = isDimming ? breath_bright - 1 : breath_bright + 1;
          fillAllSolid(rainbowColors[currentColor][0], rainbowColors[currentColor][1], rainbowColors[currentColor][2], breath_bright);
          return;
        }
        case 3: { 
          if (!HWL_singleColor[0] && !HWL_singleColor[1] && !HWL_singleColor[2]) {
            fillAllSolid(0, 0, 0, 0); return;
          }
          fillAllSolid(HWL_singleColor[0], HWL_singleColor[1], HWL_singleColor[2], HWL_brightness);
          return;
        }
        case 4: { 
          static uint8_t breath_bright = HWL_brightness;
          static bool isDimming = true;
          isDimming = (isDimming && (breath_bright - 1 <= 1)) ? false : (!isDimming && (breath_bright + 1 >= HWL_brightness)) ? true : isDimming;
          breath_bright = isDimming ? breath_bright - 1 : breath_bright + 1;
          fillAllSolid(HWL_singleColor[0], HWL_singleColor[1], HWL_singleColor[2], breath_bright);
          return;
        }
      }
    }
  }
  if (HWL_enable == 1 && hardwareLighting == false && HWL_return == 1) {
    if (currentMillis - lastPacketRcvd >= (HWL_returnafter * 1000)) {
      resetLighting();
      hardwareLighting = true;
    }
  }
}

// ==================== signalRGB 协议解析 ====================
void processUSBpacket() {
  newUSBpacketArrived = false;

  if (usbPacket[0] && usbPacket[1] && !usbPacket[2] && usbPacket[3] == 0xAA) {
    lastPacketRcvd = millis();
    if (HWL_enable == 1 && hardwareLighting == true) hardwareLighting = false;
    colorLeds(usbPacket);
    return;
  }
  else if (!usbPacket[0] && !usbPacket[1] && !usbPacket[2] && usbPacket[3] == 0xBB) {
    HWL_enable = usbPacket[4]; HWL_return = usbPacket[5]; HWL_returnafter = usbPacket[6];
    HWL_effectMode = usbPacket[7]; HWL_effectSpeed = usbPacket[8]; HWL_brightness = usbPacket[9];
    HWL_singleColor[0] = usbPacket[10]; HWL_singleColor[1] = usbPacket[11]; HWL_singleColor[2] = usbPacket[12];
    StatusLED_enable = usbPacket[13]; ColorCompression_enable = usbPacket[14];
    hardwareLighting = HWL_enable;
    DataLedOn = false; toggleOnboardLED(false);

    EEPROM.write(eeprom_HWL_enable, HWL_enable); EEPROM.write(eeprom_HWL_return, HWL_return);
    EEPROM.write(eeprom_HWL_returnafter, HWL_returnafter); EEPROM.write(eeprom_HWL_effectMode, HWL_effectMode);
    EEPROM.write(eeprom_HWL_effectSpeed, HWL_effectSpeed); EEPROM.write(eeprom_HWL_brightness, HWL_brightness);
    EEPROM.write(eeprom_HWL_color_r, HWL_singleColor[0]); EEPROM.write(eeprom_HWL_color_g, HWL_singleColor[1]);
    EEPROM.write(eeprom_StatusLED_enable, StatusLED_enable); EEPROM.write(eeprom_ColorCompression_enable, ColorCompression_enable);
    EEPROM.commit();
    delay(50);
    resetLighting();
    return;
  }
  else if (!usbPacket[0] && !usbPacket[1] && !usbPacket[2] && usbPacket[3] == 0xCC) {
    usb_hid.sendReport(0, firmwareVersion, sizeof(firmwareVersion));
    return;
  }
}

void colorLeds(uint8_t const* usbPacket) {
  int multiplier = ColorCompression_enable ? 2 : 1;
  uint8_t uncompressedRGB[ledsPerPacket * 6];

  if (usbPacket[0] == packetCount + 1) {
    int ledsSent = packetCount * ledsPerPacket * multiplier;

    if (ledsSent < totalLedCount) {
      if (ColorCompression_enable) {
        for (int r = 0; r < ledsPerPacket; r++) {
          uncompressedRGB[r*6+0] = (usbPacket[4 + r*3]   & 0x0F) << 4;
          uncompressedRGB[r*6+1] = (usbPacket[4 + r*3]   & 0xF0);
          uncompressedRGB[r*6+2] = (usbPacket[4 + r*3+1] & 0x0F) << 4;
          uncompressedRGB[r*6+3] = (usbPacket[4 + r*3+1] & 0xF0);
          uncompressedRGB[r*6+4] = (usbPacket[4 + r*3+2] & 0x0F) << 4;
          uncompressedRGB[r*6+5] = (usbPacket[4 + r*3+2] & 0xF0);
        }
      }

      for (int ledIdx = 0; ledIdx < ledsPerPacket * multiplier; ledIdx++) {
        if (ledsSent >= totalLedCount) break;

        uint32_t color;
        if (ColorCompression_enable) {
          color = ((uint32_t)uncompressedRGB[ledIdx*3]   << 16)
                | ((uint32_t)uncompressedRGB[ledIdx*3+1] << 8)
                | ((uint32_t)uncompressedRGB[ledIdx*3+2]);
        } else {
          color = ((uint32_t)usbPacket[4 + ledIdx*3]   << 16)
                | ((uint32_t)usbPacket[4 + ledIdx*3+1] << 8)
                | ((uint32_t)usbPacket[4 + ledIdx*3+2]);
        }

        setGlobalPixel(ledsSent, color);   
        ledsSent++;
      }
    }
    packetCount++;
  } else {
    packetCount = 0;
    currentGlobalLed = 0;
  }

  if (packetCount == usbPacket[1]) updateChannel = true;
}
