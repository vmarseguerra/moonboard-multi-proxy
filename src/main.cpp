#include <bluefruit.h>
#include <Adafruit_NeoPixel.h>

#define PIN         10
#define NUMPIXELS   198
#define BRIGHTNESS  50

BLEDfu bledfu;
BLEDis bledis;
BLEUart bleuart; // UART server
BLEClientUart clientUart;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

bool isInFrame = false;
bool isPreviousWasL = false;
String frameBuf = "";
String holdBuf = "";

void appConnectCallback(uint16_t conn_handle) {
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  char peerName[32] = {0};
  conn->getPeerName(peerName, sizeof(peerName));
  Serial.print("[App] Connected: ");
  Serial.println(peerName);
}
void appDisconnectCallback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("[App] Disconnected");
}

// Test LEDs by lighting them up one by one for blue, red, green
void testLedColors(uint16_t perPixelDelayMs, uint16_t colorDelayMs) {
  pixels.begin();
  pixels.clear();
  pixels.show();

  const uint8_t seq[][3] = {
    {0,   0, 255}, // blue
    {255, 0,   0}, // red
    {0, 255,   0}  // green
  };
  const size_t seqLen = sizeof(seq) / sizeof(seq[0]);
  for (size_t c = 0; c < seqLen; ++c) {
    for (uint16_t i = 0; i < NUMPIXELS; ++i) {
      pixels.setPixelColor(i, pixels.Color(seq[c][0], seq[c][1], seq[c][2]));
      pixels.show();
      delay(perPixelDelayMs);
    }
    delay(colorDelayMs);
  }

  pixels.clear();
  pixels.show();
}

// Parse a problem frame like "S12,R159" (no surrounding l# or trailing #)
void parseProblemFrame(const String &buf) {
  unsigned int i = 0;
  while (i < buf.length()) {
    int comma = buf.indexOf(',', i);
    if (comma == -1) {
      holdBuf = buf.substring(i);
      i = buf.length();
    } else {
      holdBuf = buf.substring(i, comma);
      i = comma + 1;
    }
    holdBuf.trim();
    if (holdBuf.length() < 2) continue;

    char type = holdBuf.charAt(0);
    String numStr = holdBuf.substring(1);
    if (numStr.length() == 0) continue;

    // Ensure digits only
    bool ok = true;
    for (unsigned int k = 0; k < numStr.length(); ++k) {
      if (!isDigit(numStr.charAt(k))) { ok = false; break; }
    }
    if (!ok) continue;

    int idx = numStr.toInt();
    if (idx < 0 || idx > 197) continue;

    uint8_t R = 0, G = 0, B = 0;
    switch (type) {
      case 'S': /* Start (green) */     R = 0;   G = 255; B = 0;   break;
      case 'L': /* Left (violet) */     R = 170; G = 0;   B = 130; break;
      case 'R': /* Right (blue) */      R = 0;   G = 0;   B = 255; break;
      case 'M': /* Match (dark pink) */ R = 255; G = 0;   B = 40; break;
      case 'F': /* Foot (cyan) */       R = 100; G = 200; B = 200; break;
      case 'E': /* End (red) */         R = 255; G = 0;   B = 0;   break;
      default: continue;
    }

    pixels.setPixelColor(idx, pixels.Color(R, G, B));
  }
  pixels.show();
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  // It might skip the first serial logs, but it also does not stuck the board when on battery
  if (!Serial) delay(1000);
  // while ( !Serial ) delay(10);

  Serial.println(F("Moonboard start"));

  pixels.setBrightness(BRIGHTNESS);
  frameBuf.reserve(250);
  holdBuf.reserve(10);

  Bluefruit.begin(5, 0);  // 5 app (peripherals)
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Moonboard");

  bledfu.begin();

  Bluefruit.Periph.setConnectCallback(appConnectCallback);
  Bluefruit.Periph.setDisconnectCallback(appDisconnectCallback);

  bledis.setModel("Moonboard NRF52840");
  bledis.setSoftwareRev("2025.12.16");
  bledis.begin();

  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bledis);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);  // In unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);    // In seconds

  Serial.println(F("Setup Done"));
  Serial.println(F("Testing all leds strip"));
  testLedColors(10, 500);
  Serial.println(F("Ready"));
  Serial.println(F("-------------------------------------------"));
}

void loop() {
  if (!Bluefruit.Advertising.isRunning()){
    Bluefruit.Advertising.start(0);
  }

  while (bleuart.available()) {
    char c = bleuart.read();

    // Serial log
    if (c == 'l') {
      Serial.print('\n');
    }
    Serial.print(c);

    if (isInFrame) {
      if (c == '#') {
        // End of frame, parse contents
        parseProblemFrame(frameBuf);
        frameBuf = "";
        isInFrame = false;
      } else {
        frameBuf += c;
      }

      if(frameBuf.length() > 245){
        frameBuf = "";
        isInFrame = false;
        Serial.println(F("Frame too long, skipping ..."));
      }
      continue;
    } else {
      // Detect start sequence "l#"
      if (isPreviousWasL && c == '#') {
        isInFrame = true;
        frameBuf = "";
        isPreviousWasL = false;
        pixels.clear();
        pixels.show();
        continue;
      } else {
        isPreviousWasL = (c == 'l');
      }
    }
  }
}