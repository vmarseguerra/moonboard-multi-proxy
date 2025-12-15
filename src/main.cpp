

#include <bluefruit.h>

BLEDis bledis;
BLEUart bleuart;  // UART server
BLEClientUart clientUart;

bool deviceConnected = false;

void appConnectCallback(uint16_t conn_handle) {
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  char peerName[32] = {0};
  conn->getPeerName(peerName, sizeof(peerName));
  Serial.print("[App] Connected: ");
  Serial.println(peerName);
}
void appDisconnectCallback(uint16_t conn_handle, uint8_t reason) { Serial.println("[App] Disconnected"); }

void moonboardConnectCallback(uint16_t conn_handle) {
  Serial.println("[Moonboard] Connected");

  if (clientUart.discover(conn_handle)) {
    clientUart.enableTXD();  // Enable TXD's notify
    deviceConnected = true;
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    // Disconnect since we couldn't find bleuart service
    Bluefruit.disconnect(conn_handle);
  }
}
void moonBoardDisconnectCallback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("[Moonboard] Disconnected");
  deviceConnected = false;
  digitalWrite(LED_BUILTIN, LOW);
  // Bluefruit.Scanner.start(0); // Not needed as we have used: Scanner.restartOnDisconnect(true)
}

void scanCallback(ble_gap_evt_adv_report_t* report) {
  Serial.println("[Moonboard] Found, connecting...");
  Bluefruit.Central.connect(report);
}

// Should not be necessary as the board never send data
void onMoonboardUARTRx(BLEClientUart& uart) {
  while (uart.available()) {
    uint8_t c = uart.read();
    bleuart.write(c);  // Broadcast to all
  }
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  // It might skip the first serial logs, but it also does not stuck the board when on battery
  if (!Serial) delay(1000);
  // while ( !Serial ) delay(10);

  Serial.println(F("Moonboard proxy start"));

  Bluefruit.begin(5, 1);  // 5 app (peripherals) , 1 Moonboard (central)
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Moonboard multi");

  Bluefruit.Periph.setConnectCallback(appConnectCallback);
  Bluefruit.Periph.setDisconnectCallback(appDisconnectCallback);
  Bluefruit.Central.setConnectCallback(moonboardConnectCallback);
  Bluefruit.Central.setDisconnectCallback(moonBoardDisconnectCallback);

  bledis.setModel("MoonboardProxy");
  bledis.setSoftwareRev("2025.12.15");
  bledis.begin();

  bleuart.begin();

  clientUart.begin();
  clientUart.setRxCallback(onMoonboardUARTRx);

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bledis);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);  // In unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);    // In seconds
  Bluefruit.Advertising.start(0);              // Don't stop advertising after n seconds

  Bluefruit.Scanner.setRxCallback(scanCallback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterUuid(bleuart.uuid);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.start(0);

  Serial.println(F("Setup Done"));
  Serial.println(F("-------------------------------------------"));
  Serial.println();
}

void loop() {
  while (bleuart.available()) {
    char c = bleuart.read();
    if (deviceConnected) clientUart.write(c);

    // Serial log
    if (c == 'l') {
      Serial.print('\n');
    }
    Serial.print(c);
  }
}
