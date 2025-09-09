//Name: Om Varma Dendukuri 
//ID no: 2025A4PS0677H

#include <SoftwareSerial.h>

// Pins for GPS module
static const int RXPin = 8; //arduino receives GPS TX
static const int TXPin = 7; //arduino sends to GPS RX
static const int GPSBaud = 9600; //communication rate between the arduino and the GPS

//object gpsSerial acts like a serial(like a communication channel) between the gps module and the arduino
SoftwareSerial gpsSerial(RXPin, TXPin);

//maximum number of previous points to store
const int MAX_POINTS = 10;

//lists for required data
float altitudes[MAX_POINTS];
String timestamps[MAX_POINTS];  // UTC time

//its the circulating value for all the data
int MainIndex = 0;

//PeakHeight and AvgAlt
float PeakHeight = 0;
float AvgAlt = 0;

//boolean values for ascending, descending, payload deployment and if landed or not
bool ascending = false;
bool descending = false;
bool payload_deployed = false;
bool landed = false;

//error for all the calculations
float error = 0.5;

String nmeaLine = "";

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPSBaud);
  Serial.println("START");
}

void loop() {
  int idx = (MainIndex - 1 + MAX_POINTS) % MAX_POINTS; // latest index

  //reads gps data
  while (gpsSerial.available()) {
    //reads each char in the NMEA data
    char c = gpsSerial.read();
    if (c == '\n') {
      //breaks all the data into its constiuent lists
      Process_NMEA_Line(nmeaLine);
      //resets nmeaLine
      nmeaLine = "";
    } else if (c != '\r') {
      nmeaLine += c;
    }
  }

  //skips below code if there is no data yet
  if (MainIndex == 0) return;

  //print latest data in terminal
  Serial.print("Time: "); Serial.print(timestamps[idx]);
  Serial.print("Altitude: "); Serial.println(altitudes[idx]);

  //peak heigh
  if (altitudes[idx] > PeakHeight) {
    PeakHeight = altitudes[idx];
  }

  //average altitude to smoothen data
  AvgAlt = 0;
  for (int i = 0; i < MAX_POINTS; i++) {
    AvgAlt += altitudes[i];
  }
  AvgAlt = AvgAlt/MAX_POINTS;

  //to determine whether ascending descending and apogee
  float diff = altitudes[idx] - AvgAlt;

  if (!ascending && diff > error) {
    ascending = true;
    descending = false;
    Serial.println("Ascending...");
  } else if (ascending && diff < -error) {
    descending = true;
    Serial.println("Descending...");
  } else if (ascending && abs(diff) < error && descending == false) {
    Serial.println("Apogee reached!");
  }

  //payload detection (75% peak height)
  if (descending && !payload_deployed && abs(altitudes[idx] - (0.75 * PeakHeight)) < error) {
    Serial.println("PAYLOAD DEPLOYED at 75% peak height!!");
    payload_deployed = true;
  }

  //landing detection
  if (descending && !landed && altitudes[idx] <= error) {
    Serial.println("LANDED!");
    landed = true;
  }

  //delay for 1 second
  delay(1000);
}

// Process each NMEA data
void Process_NMEA_Line(String line) {
  if (line.startsWith("$GNRMC")) {
    Break_RMC_Data(line);
  } else if (line.startsWith("$GNGGA")) {
    Break_GGA_Data(line);
  }
}

// Parse $GNRMC for time, latitude, longitude
void Break_RMC_Data(String data) {

  //defining a variable(list) to store all the contents of the nmea data
  String parts[20];
  int index = 0;

  //for loop through the whole sentance
  for (int i = 0; i < data.length(); i++) {

    //if a comma is present then we move to a new index(next value)
    if (data[i] == ',') index++;
    //if no comma then we take each char from the data and keep adding it to the current index to get the total value of the index
    else parts[index] += data[i];
  }

  //get the time from the data
  timestamps[MainIndex] = parts[1]; // UTC Time
}

// Parse $GNGGA for altitude
void Break_GGA_Data(String data) {

  //defining a variable(list) to store all the contents of the nmea data
  String parts[20];
  int index = 0;

  //for loop through the whole sentance
  for (int i = 0; i < data.length(); i++) {

    //if a comma is present then we move to a new index(next value)
    if (data[i] == ',') index++;
    //if no comma then we take each char from the data and keep adding it to the current index to get the total value of the index
    else parts[index] += data[i];

  }
  //adding to the list of altitudes
  altitudes[MainIndex] = parts[9].toFloat();

  // to cyclicly replace the values in altitudes
  MainIndex = (MainIndex + 1) % MAX_POINTS;
}
