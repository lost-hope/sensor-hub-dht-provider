#include "wled.h"
#include "sensor_bus.h"
#include <DHT.h>

/*
 * DHT11/DHT22 temperature + humidity sensor provider.
 *
 * Reads a DHT11 or DHT22 (AM2302) on a single digital GPIO pin and pushes
 * the two readings into the Sensor Hub (see
 * ../sensor-hub/usermod_sensor_hub.cpp and ../sensor-hub/sensor_bus.h) as
 * "<prefix>_temperature" and "<prefix>_humidity". This usermod never talks
 * to MQTT, the JSON API or the Info tab itself - the hub takes care of all
 * of that once a sensor is registered here.
 *
 * Unlike the I2C sensor providers in this repo, the DHT's data pin is not
 * a shared WLED global - it is configured (and reserved via WLED's
 * PinManager, to avoid clashing with LEDs/relays/other usermods) right
 * here in this usermod's own settings.
 */
class DHTSensorUsermod : public Usermod {
  private:
    DHT* dht = nullptr;
    SensorHub* hub = nullptr;
    uint8_t tempHandle = SENSOR_HANDLE_INVALID;
    uint8_t humidityHandle = SENSOR_HANDLE_INVALID;

    bool enabled = true;
    bool initDone = false;

    unsigned long lastRead = 0;
    uint8_t consecutiveFailures = 0;

    // config
    int8_t pin = -1;              // data pin, unset by default
    uint8_t dhtType = DHT22;      // DHT11 (11) or DHT22/AM2302 (22)
    uint16_t checkIntervalS = 10; // how often to read the sensor (DHT11/22 can't be polled faster than ~1-2s)
    String namePrefix = "dht";    // sensor names become "<prefix>_temperature" / "<prefix>_humidity"
    uint8_t precision = 1;        // decimal places published for both readings

    static const char _name[];
    static const char _enabled[];
    static const char _pin[];
    static const char _dhtType[];
    static const char _checkInterval[];
    static const char _namePrefix[];
    static const char _precision[];

    void registerSensors() {
      if (!hub || tempHandle != SENSOR_HANDLE_INVALID) return; // already registered
      tempHandle     = hub->registerSensor((namePrefix + "_temperature").c_str(), SensorType::Temperature, nullptr, nullptr, precision);
      humidityHandle = hub->registerSensor((namePrefix + "_humidity").c_str(),    SensorType::Humidity,    nullptr, nullptr, precision);
    }

    void setSensorsAvailable(bool available) {
      if (!hub) return;
      if (tempHandle != SENSOR_HANDLE_INVALID)     hub->setSensorAvailable(tempHandle, available);
      if (humidityHandle != SENSOR_HANDLE_INVALID) hub->setSensorAvailable(humidityHandle, available);
    }

  public:
    void setup() override {
      if (pin < 0) { enabled = false; return; }
      if (!PinManager::allocatePin(pin, false, PinOwner::UM_Unspecified)) {
        enabled = false;
        pin = -1;
        return;
      }
      dht = new DHT(pin, dhtType);
      dht->begin();
      initDone = true;
    }

    void loop() override {
      if (!enabled || !initDone || !dht) return;

      if (!hub) hub = getSensorHub(); // Sensor Hub usermod may finish init after us
      if (hub) registerSensors();

      unsigned long now = millis();
      if (now - lastRead < (unsigned long)checkIntervalS * 1000UL) return;
      lastRead = now;

      float h = dht->readHumidity();
      float t = dht->readTemperature();

      if (isnan(h) || isnan(t)) {
        consecutiveFailures++;
        if (consecutiveFailures >= 3) setSensorsAvailable(false);
        return;
      }

      consecutiveFailures = 0;
      setSensorsAvailable(true);
      if (hub) {
        if (tempHandle != SENSOR_HANDLE_INVALID)     hub->updateSensor(tempHandle, t);
        if (humidityHandle != SENSOR_HANDLE_INVALID) hub->updateSensor(humidityHandle, h);
      }
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_pin)] = pin;
      top[FPSTR(_dhtType)] = dhtType;
      top[FPSTR(_checkInterval)] = checkIntervalS;
      top[FPSTR(_namePrefix)] = namePrefix;
      top[FPSTR(_precision)] = precision;
    }

    bool readFromConfig(JsonObject& root) override {
      int8_t oldPin = pin;

      JsonObject top = root[FPSTR(_name)];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      configComplete &= getJsonValue(top[FPSTR(_pin)], pin);
      configComplete &= getJsonValue(top[FPSTR(_dhtType)], dhtType);
      configComplete &= getJsonValue(top[FPSTR(_checkInterval)], checkIntervalS);
      configComplete &= getJsonValue(top[FPSTR(_namePrefix)], namePrefix);
      configComplete &= getJsonValue(top[FPSTR(_precision)], precision);

      if (initDone && pin != oldPin) {
        // pin changed at runtime via the Settings UI - release the old one and re-init on the new one
        if (oldPin >= 0) PinManager::deallocatePin(oldPin, PinOwner::UM_Unspecified);
        delete dht;
        dht = nullptr;
        initDone = false;
        setup();
      }
      return configComplete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('DHTSensor:pin',1,'data pin');"));
      settingsScript.print(F("dd=addDropdown('DHTSensor','dhtType');addOption(dd,'DHT11',11);addOption(dd,'DHT22/AM2302',22);"));
      settingsScript.print(F("addInfo('DHTSensor:checkInterval',1,'seconds between sensor reads (DHT11/22 can\\'t be read faster than ~1-2s)');"));
      settingsScript.print(F("addInfo('DHTSensor:namePrefix',1,'sensor names become &lt;prefix&gt;_temperature/_humidity - must be unique across all sensor providers');"));
      settingsScript.print(F("addInfo('DHTSensor:precision',1,'decimal places published for both readings');"));
    }
};

const char DHTSensorUsermod::_name[]          PROGMEM = "DHTSensor";
const char DHTSensorUsermod::_enabled[]       PROGMEM = "enabled";
const char DHTSensorUsermod::_pin[]           PROGMEM = "pin";
const char DHTSensorUsermod::_dhtType[]       PROGMEM = "dhtType";
const char DHTSensorUsermod::_checkInterval[] PROGMEM = "checkInterval";
const char DHTSensorUsermod::_namePrefix[]    PROGMEM = "namePrefix";
const char DHTSensorUsermod::_precision[]     PROGMEM = "precision";

static DHTSensorUsermod dht_sensor;
REGISTER_USERMOD(dht_sensor);
