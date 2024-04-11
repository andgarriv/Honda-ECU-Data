#include <SoftwareSerialWithHalfDuplex.h>

SoftwareSerialWithHalfDuplex dlcSerial(12, 12, false, false);

byte obd_select = 1;  // 1 = obd1, 2 = obd2

byte dlcdata[20] = { 0 };  // dlc data buffer

byte ect_alarm = 98;   // celcius
byte vss_alarm = 100;  // kph

unsigned long err_timeout = 0, err_checksum = 0, ect_cnt = 0, vss_cnt = 0;


void setup() {
  dlcSerial.begin(9600);
  dlcInit();
  delay(1000);
}

void loop() {
  procdlcSerial();
}

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

  int i = 0;
  while (i < (len + 3) && millis() < timeOut) {
    if (dlcSerial.available()) {
      dlcdata[i] = dlcSerial.read();
      i++;
    }
  }

  if (i < (len + 3)) {  // timeout
    err_timeout++;
    return 0;  // data error
  }
  // checksum
  crc = 0;
  for (i = 0; i < len + 2; i++) {
    crc = crc + dlcdata[i];
  }
  crc = 0xFF - (crc - 1);
  if (crc != dlcdata[len + 2]) {  // checksum failed
    err_checksum++;
    return 0;  // data error
  }
  return 1;  // success
}

void procdlcSerial() {
  static unsigned long msTick = millis();

  if (millis() - msTick >= 250) {  // run every 250 ms
    msTick = millis();

    //char h_initobd2[12] = {0x68,0x6a,0xf5,0xaf,0xbf,0xb3,0xb2,0xc1,0xdb,0xb3,0xe9}; // 200ms - 300ms delay
    //byte h_cmd1[6] = {0x20,0x05,0x00,0x10,0xcb}; // row 1
    //byte h_cmd2[6] = {0x20,0x05,0x10,0x10,0xbb}; // row 2
    //byte h_cmd3[6] = {0x20,0x05,0x20,0x10,0xab}; // row 3
    //byte h_cmd4[6] = {0x20,0x05,0x76,0x0a,0x5b}; // ecu id
    static int rpm = 0, ect = 0, iat = 0, maps = 0, baro = 0, tps = 0, afr = 0, volt = 0, volt2 = 0, fp = 0, imap = 0, sft = 0, lft = 0, inj = 0, ign = 0, lmt = 0, iac = 0, knoc = 0;

    static unsigned long vsssum = 0, running_time = 0, idle_time = 0, distance = 0;
    static byte vss = 0, vsstop = 0, vssavg = 0;

    if (dlcCommand(0x20, 0x05, 0x00, 0x10)) {                                    // row 1
      if (obd_select == 1) rpm = 1875000 / (dlcdata[2] * 256 + dlcdata[3] + 1);  // OBD1
      if (obd_select == 2) rpm = (dlcdata[2] * 256 + dlcdata[3]) / 4;            // OBD2
      // in odb1 rpm is -1
      if (rpm < 0) { rpm = 0; }

      vss = dlcdata[4];
    }

    delay(1);
    if (dlcCommand(0x20, 0x05, 0x10, 0x10)) {  // row2
      float f;
      f = dlcdata[2];
      f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
      ect = round(f);
      f = dlcdata[3];
      f = 155.04149 - f * 3.0414878 + pow(f, 2) * 0.03952185 - pow(f, 3) * 0.00029383913 + pow(f, 4) * 0.0000010792568 - pow(f, 5) * 0.0000000015618437;
      iat = round(f);
      maps = dlcdata[4] * 0.716 - 5;  // 101 kPa @ off|wot // 10kPa - 30kPa @ idle
      //baro = dlcdata[5] * 0.716 - 5;
      tps = (dlcdata[6] - 24) / 2;

      /*
      f = dlcdata[7];
      f = f / 51.3; // o2 volt in V
      
      // 0v to 1v / stock sensor
      // 0v to 5v / AEM UEGO / linear
      f = (f * 2) + 10; // afr for AEM UEGO
      afr = round(f * 10); // x10 for display w/ 1 decimal
      */

      f = dlcdata[9];
      f = f / 10.45;         // batt volt in V
      volt = round(f * 10);  // x10 for display w/ 1 decimal
      //alt_fr = dlcdata[10] / 2.55
      //eld = 77.06 - dlcdata[11] / 2.5371
    }

    delay(1);
    if (dlcCommand(0x20, 0x05, 0x20, 0x10)) {  // row3
      float f;
      sft = (dlcdata[2] / 128 - 1) * 100;  // -30 to 30
      lft = (dlcdata[3] / 128 - 1) * 100;  // -30 to 30

      inj = (dlcdata[6] * 256 + dlcdata[7]) / 250;  // 0 to 16

      //ign = (dlcdata[8] - 128) / 2;
      f = dlcdata[8];
      f = (f - 24) / 4;
      ign = round(f * 10);  // x10 for display w/ 1 decimal

      //lmt = (dlcdata[9] - 128) / 2;
      f = dlcdata[9];
      f = (f - 24) / 4;
      lmt = round(f * 10);  // x10 for display w/ 1 decimal

      iac = dlcdata[10] / 2.55;
    }

    delay(1);
    if (dlcCommand(0x20, 0x05, 0x30, 0x10)) {  // row4
      // dlcdata[7] to dlcdata[12] unknown
      knoc = dlcdata[14] / 51;  // 0 to 5
    }

    // IMAP = RPM * MAP / IAT / 2
    // MAF = (IMAP/60)*(VE/100)*(Eng Disp)*(MMA)/(R)
    // Where: VE = 80% (Volumetric Efficiency), R = 8.314 J/°K/mole, MMA = 28.97 g/mole (Molecular mass of air)
    float maf = 0.0;
    imap = rpm * maps / (iat + 273) / 2;
    // ve = 75, ed = 1.595, afr = 14.7
    maf = (imap / 60) * (80 / 100) * 1.595 * 28.9644 / 8.314472;
    // (gallons of fuel) = (grams of air) / (air/fuel ratio) / 6.17 / 454
    //gof = maf / afr / 6.17 / 454;
    //gear = vss / (rpm+1) * 150 + 0.3;


    // trip computer essentials
    if (vss > vsstop) {  // top speed
      vsstop = vss;
    }
    if (rpm > 0) {
      if (vss > 0) {  // running time
        running_time++;
        vsssum += vss;
        vssavg = (vsssum / running_time);

        float f;
        //f = vssavg;
        //f = ((f * 1000) / 14400) * running_time; // @ 250ms
        //distance = round(f);

        // formula: distance = speed * fps / 3600
        // where: distance = kilometer(s), speed = km/h, fps in second(s)
        f = vss;
        f = f * 0.25 / 3600;  // @ 250ms / km
        f = f * 1000;         // km to meters
        distance = distance + round(f);

        // time = distance / speed
      } else {  // idle time
        idle_time++;
      }

      Serial.print("RPM: ");
      Serial.println(rpm);
      Serial.print("Speed: ");
      Serial.println(vss);
      Serial.print("Voltage: ");
      Serial.println(volt / 10.0);
    }
  }
}
