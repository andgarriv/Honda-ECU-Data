#include <SoftwareSerialWithHalfDuplex.h>

SoftwareSerialWithHalfDuplex dlcSerial(11, 11, false, false);

int rpm = 0;

byte dlcdata[20] = {0}; // dlc data buffer
byte dlcTimeout = 0, dlcChecksumError = 0;
bool dlcWait = false;

int dtcErrors[14], dtcCount = 0;

void dlcInit()
{
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

int dlcCommand(byte cmd, byte num, byte loc, byte len)
{
  byte crc = (0xFF - (cmd + num + loc + len - 0x01)); // checksum FF - (cmd + num + loc + len - 0x01)

  unsigned long timeOut = millis() + 200; // timeout @ 200 ms

  memset(dlcdata, 0, sizeof(dlcdata));

  dlcSerial.listen();

  dlcSerial.write(cmd); // header/cmd read memory ??
  dlcSerial.write(num); // num of bytes to send
  dlcSerial.write(loc); // address
  dlcSerial.write(len); // num of bytes to read
  dlcSerial.write(crc); // checksum

  // reply: 00 len+3 data...
  int i = 0;
  while (i < (len + 3) && millis() < timeOut)
  {
    if (dlcSerial.available())
    {
      dlcdata[i] = dlcSerial.read();
      // if (dlcdata[i] != 0x00 && dlcdata[i+1] != (len+3)) continue; // ignore ?
      i++;
    }
  }

  if (i < (len + 3))
  { // timeout
    dlcTimeout++;
    if (dlcTimeout > 255)
      dlcTimeout = 0;
    return 0; // failed
  }

  // checksum
  crc = 0;
  for (i = 0; i < len + 2; i++)
  {
    crc = crc + dlcdata[i];
  }
  crc = 0xFF - (crc - 1);
  if (crc != dlcdata[len + 2])
  { // checksum failed
    dlcChecksumError++;
    if (dlcChecksumError > 255)
      dlcChecksumError = 0;
    return 0; // failed
  }

  return 1; // success
}

void setup() {
  Serial.begin(115200);
  dlcSerial.begin(9600);
  Serial.println("Iniciando...");
  dlcInit();
  delay(1000);
}

void loop() {
  if (readEcuData()) {

  }
  delay(200); // Esperar 1 segundo antes de la siguiente lectura
}

bool readEcuData()
{
  if (dlcCommand(0x20, 0x05, 0x00, 0x10))
  { // row 1
    // Verificar si los datos recibidos son 255, 255
    if ((dlcdata[2] == 255) && (dlcdata[3] == 255)) {
      dlcdata[2] = 0;
      dlcdata[3] = 0;
    }

    // Calcular las RPM
    rpm = ((dlcdata[2] * 256) + dlcdata[3]) / 4;
    
    if (rpm < 0)
    {
      rpm = 0;
    }
    Serial.println(rpm);
    return true;
  }
  return false;
}
