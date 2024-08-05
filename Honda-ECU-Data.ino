#include <SoftwareSerialWithHalfDuplex.h>

SoftwareSerialWithHalfDuplex dlcSerial(11, 11, false, false);

int rpm = 0;
int speed = 0;
int iat = 0;
int ect = 0;
int tps = 0;
int o2 = 0;
int stf = 0;
int ltf = 0;
int ta = 0;
float voltage = 0.0;
float imap = 0.0;

byte dlcdata[20] = { 0 };  // dlc data buffer
byte dlcTimeout = 0, dlcChecksumError = 0;

void dlcInit() {
  dlcSerial.write(0x68);
  dlcSerial.write(0x6a);
  dlcSerial.write(0xf5);
  dlcSerial.write(0xaf);
  dlcSerial.write(0xbf);
  dlcSerial.write(0xb3);
  dlcSerial.write(0xb2);
  dlcSerial.write(0xc1);
  dlcSerial.write(0xdb);
  dlcSerial.write(0xb3);
  dlcSerial.write(0xe9);
  delay(300);
}

int dlcCommand(byte cmd, byte num, byte loc, byte len) {
  byte crc = (0xFF - (cmd + num + loc + len - 0x01));  // checksum FF - (cmd + num + loc + len - 0x01)

  unsigned long timeOut = millis() + 200;  // timeout @ 200 ms

  memset(dlcdata, 0, sizeof(dlcdata));

  dlcSerial.listen();

  dlcSerial.write(cmd);  // header/cmd read memory ??
  dlcSerial.write(num);  // num of bytes to send
  dlcSerial.write(loc);  // address
  dlcSerial.write(len);  // num of bytes to read
  dlcSerial.write(crc);  // checksum

  // reply: 00 len+3 data...
  int i = 0;
  while (i < (len + 3) && millis() < timeOut) {
    if (dlcSerial.available()) {
      dlcdata[i] = dlcSerial.read();
      i++;
    }
  }

  if (i < (len + 3)) {  // timeout
    dlcTimeout++;
    if (dlcTimeout > 255)
      dlcTimeout = 0;
    return 0;  // failed
  }

  // checksum
  crc = 0;
  for (i = 0; i < len + 2; i++) {
    crc = crc + dlcdata[i];
  }
  crc = 0xFF - (crc - 1);
  if (crc != dlcdata[len + 2]) {  // checksum failed
    dlcChecksumError++;
    if (dlcChecksumError > 255)
      dlcChecksumError = 0;
    return 0;  // failed
  }

  return 1;  // success
}

void setup() {
  Serial.begin(115200);
  dlcSerial.begin(9600);
  dlcInit();
  delay(1000);
}

void loop() {
  if (readEcuData()) {}
}

bool readEcuData() {
  bool success = false;

  Serial.print("Data,");

  //Engine RPM
  if (dlcCommand(0x20, 0x05, 0x00, 0x10)) {
    if ((dlcdata[2] == 255) && (dlcdata[3] == 255)) {
      dlcdata[2] = 0;
      dlcdata[3] = 0;
    }
    rpm = ((dlcdata[2] * 256) + dlcdata[3]) / 4;
    rpm = max(rpm, 0);  // Ensure RPM is non-negative
    Serial.print("RPM: ");
    Serial.print(rpm);
    Serial.print(",");
    success = true;
  }

  //Engine Coolant Temperature (ºC)
  if (dlcCommand(0x20, 0x05, 0x10, 0x01)) {
    float f = dlcdata[2];
    f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
    ect = round(f) + 40;
    Serial.print("ECT: ");
    Serial.print(ect);
    Serial.print(",");
    success = true;
  }

  //ECU Voltage (V)
  if (dlcCommand(0x20, 0x05, 0x17, 0x01)) {
    float f = dlcdata[2];
    f = f / 10.45;
    voltage = f;
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print(",");
    success = true;
  }

  //Short Term Fuel (%)
  if (dlcCommand(0x20, 0x05, 0x20, 0x01)) {
    stf = dlcdata[2];
    Serial.print("STF: ");
    Serial.print(stf);
    Serial.print(",");
    success = true;
  }

  //Long Term Fuel (%)
  if (dlcCommand(0x20, 0x05, 0x22, 0x01)) {
    ltf = dlcdata[2];
    Serial.print("LTF: ");
    Serial.print(ltf);
    Serial.print(",");
    success = true;
  }

  //Intake Manifold Absolute Pressure (kPa)
  if (dlcCommand(0x20, 0x05, 0x12, 0x01)) {
    imap = dlcdata[2] * 0.716 - 5;
    Serial.print("imap: ");
    Serial.print(imap);
    Serial.print(",");
    success = true;
  }

  //Vehicle Speed (Km/h)
  if (dlcCommand(0x20, 0x05, 0x02, 0x01)) {
    speed = dlcdata[2];
    Serial.print("Speed: ");
    Serial.print(speed);
    Serial.print(",");
    success = true;
  }

  //Intake Air Temperature (º)
  if (dlcCommand(0x20, 0x05, 0x11, 0x01)) {
    float f = dlcdata[2];
    f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
    iat = round(f) + 40;
    Serial.print("IAT: ");
    Serial.print(iat);
    Serial.print(",");
    success = true;
  }

  //Timing advance (º)
  if (dlcCommand(0x20, 0x05, 0x26, 0x01)) {
    ta = ((dlcdata[2] - 24) / 2) + 128;
    Serial.print("TA: ");
    Serial.print(ta);
    Serial.print(",");
    success = true;
  }

  //Throttle Position Sensor (%)
  if (dlcCommand(0x20, 0x05, 0x14, 0x01)) {
    //0% = 25 & 100% = 233
    tps = round((dlcdata[2] - 25) / 2.08);
    tps = max(tps, 0);  // Ensure TPS is non-negative
    Serial.print("TPS: ");
    Serial.print(tps);
    Serial.print(",");
    success = true;
  }

  //Oxygen Sensor O2 (V)
  if (dlcCommand(0x20, 0x05, 0x15, 0x01)) {
    o2 = dlcdata[2];
    Serial.print("O2: ");
    Serial.print(o2);
    Serial.print(",");
    success = true;
  }

  Serial.println();  // End the line for each data set

  return success;
}
