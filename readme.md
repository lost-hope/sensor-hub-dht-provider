# DHT11/DHT22 Sensor Provider

A [Sensor Hub](../sensor-hub/readme.md) provider usermod for the DHT11 /
DHT22 (AM2302) - registers `dht_temperature` and `dht_humidity` with the
hub by default, which then handles MQTT, Home Assistant discovery, the
JSON API and the Info tab.

## Hardware

Unlike the I2C providers in this repo, the DHT connects via a single
digital data pin, not the shared I2C bus. Set the **Data pin** and
**Sensor type** (DHT11 or DHT22/AM2302) in this usermod's own Settings
page - the pin is reserved through WLED's PinManager so it won't silently
clash with LEDs, relays or other usermods.

DHT11/DHT22 cannot be polled faster than about once per second (DHT11) or
every two seconds (DHT22) - the default 10s check interval is a safe
starting point.

## Usage

Self-contained out-of-tree usermod (see `library.json` for its
`adafruit/DHT sensor library` dependency). Add it to `custom_usermods` next
to the [Sensor Hub](../sensor-hub/readme.md) itself.

## Usermod Settings

| Setting | Default | Description |
|---|---|---|
| Enabled | on | Master on/off switch (also auto-disabled until a pin is set) |
| Pin | unset | Data pin the DHT is wired to |
| Sensor type | DHT22 | DHT11 or DHT22/AM2302 |
| Check interval | 10s | How often the sensor is read |
| Name prefix | `dht` | Sensor names become `<prefix>_temperature/_humidity` - must be unique across every provider registered with the hub |
| Precision | 1 | Decimal places published for both readings |
