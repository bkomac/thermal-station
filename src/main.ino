#include <DallasTemperature.h>
#include <Espiot.h>
#include <OneWire.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 13
#define TEMPERATURE_PRECISION 12 // 8 9 10 12

Espiot espiot;
PubSubClient mq;

int devicesFound = 0;
DeviceAddress devices[10];

// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void setup(void) {
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  espiot.init();
  mq = espiot.getMqClient();

  // Start up the library
  sensors.begin();

  getDeviceAddress();
}

void loop(void) {
  espiot.loop();
  delay(1000);
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.println("\nRequesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  delay(200);
  espiot.blink();
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["deviceId"] = espiot.getDeviceId();

  JsonArray &devicesArray = root.createNestedArray("devices");
  for (int i = 0; i < devicesFound; i++) {
    Serial.print("\nDevice " + (String)i + " Address: ");
    String address1 = printAddress(devices[i]);
    Serial.println(" Temp:" + (String)printTemperature(devices[i]));

    float tempC = sensors.getTempC(devices[i]);
    JsonObject &device = devicesArray.createNestedObject();
    device["address"] = address1;
    device["temp"] = tempC;
  }

  root.printTo(Serial);
  char buffer[255];
  root.printTo(buffer, sizeof(buffer));

  espiot.mqPublish(buffer);
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
