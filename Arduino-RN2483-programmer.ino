// Arduino RN2483 programmer
// Implemented using http://ww1.microchip.com/downloads/en/DeviceDoc/41398B.pdf
#include "RN2483LVP.h"

#define SIZE 14
#define MCLR 4
#define PGD 5
#define PGC 6

int testseq[SIZE] = {0xFE, 0xED, 0xFD, 0xAA, 0xDA, 0x01, 0x01, 0x02, 0x13, 0x37, 0x19, 0x93, 0x4E, 0xFA};
bool serialcomplete = false;
String input = "";
RN2483LVP programmer;

void setup()
{
  // put your setup code here, to run once:
  SerialUSB.begin(57600);

  // Wait for serialUSB or start after 30 seconds
  while ((!SerialUSB) && (millis() < 30000))
    ;
  SerialUSB.write("SerialUSB set up\r\n");

  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize pins
  pinMode(MCLR, OUTPUT);
  pinMode(PGD, OUTPUT);
  pinMode(PGC, OUTPUT);
  digitalWrite(MCLR, LOW);
  digitalWrite(PGD, LOW);
  digitalWrite(PGC, LOW);

  // Wiggle MCLR
  digitalWrite(MCLR, HIGH);
  delay(10);
  digitalWrite(MCLR, LOW);
  delay(10);
  digitalWrite(MCLR, HIGH);
  delay(10);

  // Initialize RN2483 low-voltage programmer object
  programmer.initRN2483LVP(PGD, PGC, MCLR, SerialUSB);

  // Entering low-voltage program/verify mode
  programmer.enterLVP();
}

void loop()
{
  while (SerialUSB.available())
  {
    char c = SerialUSB.read();

    if (c == '\n' || c == '\r' || c == 'X')
    {
      if (input != "")
      {
        serialcomplete = true;
      }
      break;
    }
    input += c;
  }

  if (serialcomplete)
  {
    int response;
    switch (input.charAt(0))
    {
    case 'W':
      SerialUSB.print("Writing\r\n");
      programmer.writeCodeSequence(0, 0, 0, testseq);
      SerialUSB.print("Done writing\r\n");
      break;
    case 'R':
      SerialUSB.print("Reading\r\n");
      programmer.readTest();
      break;
    case 'D':
      SerialUSB.print("Reading device ID\r\n");
      programmer.readDeviceID();
      break;
    case 'E':
      SerialUSB.print("Bulk erasing\r\n");
      programmer.bulkErase();
      break;
    default:
      SerialUSB.write("Invalid serial command\r\n");
    }
    serialcomplete = false;
    input = "";
  }
}
