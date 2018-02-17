#include <DallasTemperature.h>
#include <Espiot.h>
#include <OneWire.h>
#include <UltraDistSensor.h>

#define ONE_WIRE_BUS 13
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

#define SR04_TRIGGER_PIN 4
#define SR04_ECHO_PIN 5

// Vcc measurement
ADC_MODE(ADC_VCC);

Espiot espiot;
String appV = "1.0.5";
int devicesFound = 0;
DeviceAddress devices[10];

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ultrasonic distance sensor
UltraDistSensor distsensor;

int lastTime = millis();

void setup(void) {

  Serial.begin(115200);
  Serial.println("Thermal Station " + appV);

  espiot.apPass = "thermal12345678";
  espiot.enableVccMeasure();
  espiot.SENSOR = "DS18B20,HCSR-04";
  espiot.init(appV);

  distsensor.attach(SR04_TRIGGER_PIN, SR04_ECHO_PIN,
                    20000ul); // Trigger pin , Echo pin

  sensors.begin();
  sensors.requestTemperatures();
  getDeviceAddress();
}

void loop(void) {
  delay(100);
  espiot.loop();
yield();
  if (millis() > lastTime + espiot.timeOut && devicesFound > 0) {

    int reading = distsensor.distanceInCm();
    Serial.print("\nSensor Reading :");
    Serial.print(reading);
    Serial.println(" CM");

    Serial.println("\nRequesting temperatures...");
    getDeviceAddress();
    yield();
    sensors.requestTemperatures();
    Serial.println("DONE");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = espiot.getDeviceId();
    root["sensorType"] = espiot.SENSOR;
    root["vcc"] = ESP.getVcc() / 1000.00;
    root["distanceCm"] = reading;

    JsonArray &devicesArray = root.createNestedArray("sensors");
    for (int i = 0; i < devicesFound; i++) {
      yield();
      Serial.print("\nDevice " + (String)i + " Address: ");
      String address1 = printAddress(devices[i]);
      Serial.println(" Temp:" + (String)printTemperature(devices[i]));

      float tempC = sensors.getTempC(devices[i]);
      if (tempC > -127.00) {
        JsonObject &device = devicesArray.createNestedObject();
        device["address"] = address1;
        device["temp"] = tempC;
      }
    }

    String payload;
    root.printTo(payload);
    yield();
    espiot.mqPublish(payload);
    lastTime = millis();
    espiot.blink(1, 300);
    yield();
  }
}

void getDeviceAddress(void) {

  Serial.println("\nGetting the address...");

  devicesFound = sensors.getDeviceCount();
  Serial.print("Num devices: ");
  Serial.println(devicesFound);

  for (int i = 0; i < devicesFound; i++)
    if (!sensors.getAddress(devices[i], i))
      Serial.println("Unable to find address for Device" + i);

  for (int i = 0; i < devicesFound; i++) {
    Serial.print("\nDevice " + (String)i + " Address: ");
    printAddress(devices[i]);
    Serial.println(printTemperature(devices[i]));
  }

  for (int i = 0; i < devicesFound; i++)
    sensors.setResolution(devices[i], TEMPERATURE_PRECISION);

  return;
}

String printAddress(DeviceAddress deviceAddress) {
  String out = "";
  for (uint8_t i = 0; i < 8; i++) {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    out += (String)deviceAddress[i];
  }
  return out;
}

String printTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC < 10)
    return "0" + (String)tempC;
  else
    return (String)tempC;
}

void setupEndpoints(){
    espiot.server.on("/state", HTTP_GET, []() {
      espiot.blink();

      int reading = distsensor.distanceInCm();

      DynamicJsonBuffer jsonBuffer;
      JsonObject &root = jsonBuffer.createObject();
      root["deviceId"] = espiot.getDeviceId();
      root["sensorType"] = espiot.SENSOR;
      root["vcc"] = ESP.getVcc() / 1000.00;
      root["distanceCm"] = reading;

      JsonArray &devicesArray = root.createNestedArray("sensors");
      for (int i = 0; i < devicesFound; i++) {
        yield();
        Serial.print("\nDevice " + (String)i + " Address: ");
        String address1 = printAddress(devices[i]);
        Serial.println(" Temp:" + (String)printTemperature(devices[i]));

        float tempC = sensors.getTempC(devices[i]);
        if (tempC > -127.00) {
          JsonObject &device = devicesArray.createNestedObject();
          device["address"] = address1;
          device["temp"] = tempC;
        }
      }
      yield();

      String content;
      root.printTo(content);
      espiot.server.send(200, "application/json", content);
    });

}
