#include <DallasTemperature.h>
#include <Espiot.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 13
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

Espiot espiot;
String appV = "1.0.1";
int devicesFound = 0;
DeviceAddress devices[10];

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int lastTime = millis();

void setup(void) {

  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  espiot.init(appV);

  sensors.begin();
  sensors.requestTemperatures();
  getDeviceAddress();
}

void loop(void) {
  espiot.loop();

  if (millis() > lastTime + espiot.timeOut && devicesFound > 0) {
    Serial.println("\nRequesting temperatures...");
    getDeviceAddress();
    sensors.requestTemperatures();
    Serial.println("DONE");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = espiot.getDeviceId();

    JsonArray &devicesArray = root.createNestedArray("sesnsors");
    for (int i = 0; i < devicesFound; i++) {
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

    espiot.mqPublish(payload);
    lastTime = millis();
    espiot.blink(1, 300);
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
