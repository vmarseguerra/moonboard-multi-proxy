# Moonboard multi connexion proxy

A multi-client UART BLE proxy.

Done to allow old [MoonBoard](https://moonclimbing.com/moonboard) led system that only allows one app connexion to support many.

Use a [NRF52840](https://www.nordicsemi.com/Products/nRF52840) hardware board like the [Adafruit Feather nRF52840 Express](https://www.adafruit.com/product/4062)

## Usage

1. Compile and flash the firmware
2. When close to the Moonboard, power up your device.

    When it is connected to the Moonboard, the built-in led will turn `on`
3. On your phone connect to the `Moonboard multi` board

    Up to 5 apps can now simultaneously use the Moonboard

## Development

Use [PlatformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html) Arduino in [VSCode](https://code.visualstudio.com/)

Example code at: https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/dual-roles-bleuart

## Moonboard BLE protocol

It is a very simple protocol that consists of a single UART BLE service.

Informations found at: https://github.com/e-sr/moonboard/tree/master/ble

To be detected by the app the BLE device name needs to start by "Moonboard" and have the Nordic UART service (NUS, 6E400001). Data flow is only one way and sent to the board.

- A problem starts with `l#` and ends with `#`, holds are separated by a `,`, ordering does not matter
- Each hold is represented by its type (one letter) and its location number, with the format: `X00`
- Hold numbers are ordered in zig zag from bottom left to top right, starts at 0 and ends at 197

Hold type:
- S: Start (green)
- L: Left (violet)
- R: Right (blue)
- M: Match (dark pink)
- F: Foot (light blue)
- E: End (red)

Examples:
- Eat thy cake: `l#S67,R62,R84,R94,R109,R115,R116,E125,S148,R159#`
- Brush fire: `l#S4,S33,R42,R61,R81,R87,R157,E197#`
- Coconut milkshake: `l#S4,S33,R28,R25,E18,R45,R86,E89,R120#`
