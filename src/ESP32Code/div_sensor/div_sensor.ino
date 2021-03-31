#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <PZEM004Tv30.h>
PZEM004Tv30 pzem1(&Serial);

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// WiFi AP SSID
#define WIFI_SSID "WIFI_SSID_DIV"
// WiFi password
#define WIFI_PASSWORD "WIFI_PWD_DIV"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "https://us-central1-1.gcp.cloud2.influxdata.com"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "TOK_SME_DIV"
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "ORG_ID_DIV"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "sme-divya"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor1("Phase1");

void setup() {
  Serial.begin(9600);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
 // If no Wifi signal, try to reconnect it
    if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED)) {
      Serial.println("Wifi connection lost");
    }

    // Store measured value into point
    // Report RSSI of currently connected network
    //sensor.addField("rssi", WiFi.RSSI());

    // Print what are we exactly writing
    //Serial.print("Writing: \n");
    //Serial.println(sensor1.toLineProtocol());
    //Serial.println(sensor2.toLineProtocol());
    //Serial.println(sensor3.toLineProtocol());

    // Clear fields for reusing the point. Tags will remain untouched
    sensor1.clearFields();

    float voltage = pzem1.voltage();
    float current = pzem1.current();
    float power = pzem1.power();
    float energy = pzem1.energy();
    float pf = pzem1.pf();
    float frequency = pzem1.frequency();
//    Test without sensor to see if db works
//    float voltage = 100.0;
//    float current = 200.0;
//    float power = 300.0;
//    float energy = 400.0;
//    float pf = 500.0;
//    float frequency = 600.0;
    sensor1.addField("voltage", voltage);
    sensor1.addField("current", current);
    sensor1.addField("power", power);
    sensor1.addField("energy", energy);
    sensor1.addField("frequency", frequency);
    sensor1.addField("pf", pf);

    Serial.print("Writing: ");
    Serial.println(sensor1.toLineProtocol());

    Serial.print("Slave"); Serial.print(1); Serial.print("= Voltage: "); Serial.print(voltage); Serial.print("V, ");
    Serial.print("Current: "); Serial.print(current); Serial.print("A, ");
    Serial.print("Power: "); Serial.print(power); Serial.print("W, ");
    Serial.print("Energy: "); Serial.print(energy,3); Serial.print("kWh, ");
    Serial.print("Frequency: "); Serial.print(frequency, 1); Serial.print("Hz, ");
    Serial.print("PF: "); Serial.println(pf);
    if (!client.writePoint(sensor1)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    delay(20);
}
