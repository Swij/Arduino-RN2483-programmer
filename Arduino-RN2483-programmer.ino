// Arduino RN2483 programmer
// Implemented using http://ww1.microchip.com/downloads/en/DeviceDoc/41398B.pdf
#define SIZE 14
#define MCLR 4
#define PGD 5
#define PGC 6

int keyseq[] = {0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0};
int testseq[SIZE] = {0xFE, 0xED, 0xFD, 0xAA, 0xDA, 0x01, 0x01, 0x02, 0x13, 0x37, 0x19, 0x93, 0x4E, 0xFA};
bool serialcomplete = false;
bool inLVP = false;
String input = "";

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

  // Entering low-voltage program/verify mode
  enterLVP();
}

// Enter low-voltage program/verify mode
void enterLVP()
{
  if (not(inLVP))
  {
    inLVP = true;
    SerialUSB.write("Entering low-voltage program/verify mode\r\n");

    // Pull all programming pins Low
    digitalWrite(MCLR, LOW);
    digitalWrite(PGD, LOW);
    digitalWrite(PGC, LOW);

    // Wait atleast 100ns (P13)
    delayMicroseconds(1);

    // Set MCLR High then Low
    digitalWrite(MCLR, HIGH);
    delayMicroseconds(1);
    digitalWrite(MCLR, LOW);

    // Wait atleast 1ms (P18)
    delayMicroseconds(2);

    // Enter key sequence on PGD
    for (int n = 0; n < 32; n++)
    {
      //Lower PGC and write bit to PGD
      digitalWrite(PGC, LOW);
      digitalWrite(PGD, keyseq[n]);
      delayMicroseconds(1);

      //Raise PGC to send bit
      digitalWrite(PGC, HIGH);
      delayMicroseconds(1);
    }
    digitalWrite(PGC, LOW);

    // Wait atleast 40ns
    delayMicroseconds(1);

    // Set MCLR High
    digitalWrite(MCLR, HIGH);

    // Wait atleast 400us
    delayMicroseconds(450);
  }
}

// Exit low-voltage program/verify mode
void exitLVP()
{
  inLVP = false;
  SerialUSB.write("Exiting low-voltage program/verify mode\r\n");
  delayMicroseconds(1);
  digitalWrite(MCLR, LOW);
  delayMicroseconds(10);
  digitalWrite(MCLR, HIGH);
}

// Serial program/verify operation
int sendOp(int cmd, int payload)
{
  bool valid = true;
  bool isread = false;

  // Check if command is valid and if it contains a read
  switch (cmd)
  {
  case B0000:
    break;
  case B0010:
    isread = true;
    break;
  case B1000:
    isread = true;
    break;
  case B1001:
    isread = true;
    break;
  case B1010:
    isread = true;
    break;
  case B1011:
    isread = true;
    break;
  case B1100:
    break;
  case B1101:
    break;
  case B1110:
    break;
  case B1111:
    break;
  default:
    valid = false;
  }

  // Check if data payload is the right size
  if ((isread && (payload > 255)) || (!isread && (payload > 65535)))
  {
    SerialUSB.write("Invalid number of payload bits\r\n");
    return false;
  }

  if (valid)
  {
    // Send command bits to the RN2483
    for (int n = 0; n < 4; n++)
    {
      //Lower PGC and write bit to PGD
      digitalWrite(PGC, LOW);
      digitalWrite(PGD, bitRead(cmd, n));
      delayMicroseconds(1);

      //Raise PGC to send bit
      digitalWrite(PGC, HIGH);
      delayMicroseconds(1);
    }
    digitalWrite(PGC, LOW);

    // Wait atleast 40ns (P5)
    delayMicroseconds(1);

    if (isread)
    {
      // Write 8 payload bits to RN2483
      for (int n = 0; n < 8; n++)
      {
        //Lower PGC and write bit to PGD
        digitalWrite(PGC, LOW);
        digitalWrite(PGD, bitRead(payload, n));
        delayMicroseconds(1);

        //Raise PGC to send bit
        digitalWrite(PGC, HIGH);
        delayMicroseconds(1);
      }
      digitalWrite(PGC, LOW);

      // Set PGD to input
      pinMode(PGD, INPUT);

      // Wait atleast 20ns (P6)
      delayMicroseconds(1);

      // Read 8 response bits from RN2483
      int response = 255;
      for (int n = 0; n < 8; n++)
      {
        //Raise PGC to recieve bit
        digitalWrite(PGC, HIGH);
        delayMicroseconds(1);

        //Lower PGC and read bit from PGD
        digitalWrite(PGC, LOW);
        bitWrite(response, n, digitalRead(PGD));
        delayMicroseconds(1);
      }

      // Set PGD to output
      pinMode(PGD, OUTPUT);

      // Wait atleast 40ns (P5A)
      delayMicroseconds(1);
      //SerialUSB.write("Sent 4 command bits and 8 data payload bits\r\nResponse: ");
      //SerialUSB.print(String(response, HEX) + "\r\n");

      return response;
    }
    else
    {
      // Write all 16 bits from the data payload
      for (int n = 0; n < 16; n++)
      {
        //Lower PGC and write bit to PGD
        digitalWrite(PGC, LOW);
        digitalWrite(PGD, bitRead(payload, n));
        delayMicroseconds(1);

        //Raise PGC to send bit
        digitalWrite(PGC, HIGH);
        delayMicroseconds(1);
      }
      digitalWrite(PGC, LOW);

      // Wait atleast 40ns (P5A)
      delayMicroseconds(1);
      //SerialUSB.write("Sent 4 command bits and 16 data payload bits\r\n");

      return true;
    }
  }
  else
  {
    SerialUSB.write("Command not valid\r\n");
    return false;
  }
}

// Bulk erase the entire device
void bulkErase()
{
  // Write 0F0F to 3C0005h
  writeAddressConfig(0x3C, 0x00, 0x05, 0x0F0F);
  // Write 8F8F to 3C0004h
  writeAddressConfig(0x3C, 0x00, 0x04, 0x8F8F);
  sendOp(B0000, 0x0000);
  sendOp(B0000, 0x0000);
  // Wait atleast 15ms (P11) to let the erase complete
  delay(20);
}

// Print device IDs on serial
void readDeviceID()
{
  int DEVID1 = readAddress(0x3F, 0xFF, 0xFE);
  int DEVID2 = readAddress(0x3F, 0xFF, 0xFF);
  SerialUSB.print("DEVID1: " + String(DEVID1, HEX) + "h\r\n");
  SerialUSB.print("DEVID2: " + String(DEVID2, HEX) + "h\r\n");
}

// Sets the TBLPTR to the address to be read/written
void setTBLPTR(int addru, int addrh, int addrl)
{
  sendOp(B0000, 0x0E00 + addru); // MOVLW <Addr[21:16]>
  sendOp(B0000, 0x6EF8);         // MOVWF TBLPTRU
  sendOp(B0000, 0x0E00 + addrh); // MOVLW <Addr[15:8]>
  sendOp(B0000, 0x6EF7);         // MOVWF TBLPTRH
  sendOp(B0000, 0x0E00 + addrl); // MOVLW <Addr[7:0]>
  sendOp(B0000, 0x6EF6);         // MOVWF TBLPTRL
}

// Read value at address
int readAddress(int addru, int addrh, int addrl)
{
  setTBLPTR(addru, addrh, addrl);
  return sendOp(B1001, 0x0000); // TBLRD *+
}

// Write value to address in config block
void writeAddressConfig(int addru, int addrh, int addrl, int value)
{
  setTBLPTR(addru, addrh, addrl);
  sendOp(B1100, value);
}

// Writes a sequence of bytes into the code memmory
// bytes.length <= 64, bytes[n] <= 0xFFFF
void writeCodeSequence(int addru, int addrh, int addrl, int bytes[])
{
  int LoopCount = 0;

  // Direct access to code memory
  sendOp(B0000, 0x8EA6); // BSF EECON1, EEPGD
  sendOp(B0000, 0x9CA6); // BCF EECON1, CFGS
  sendOp(B0000, 0x84A6); // BSF EECON1, WREN

  // Point to row to write.
  setTBLPTR(addru, addrh, addrl);

  // Load write buffer. Repeat for all but the last two bytes.
  for (int n = 0; n < SIZE - 2; n = n + 2)
  {
    sendOp(B1101, (bytes[n + 1] << 8) + bytes[n]);
  }

  // Load write buffer for last two bytes and start programming
  sendOp(B1111, (bytes[SIZE - 2] << 8) + bytes[SIZE - 1]);

  // Send NOP and Hold 4th PGC HIGH for atleast 1ms (P9)
  digitalWrite(PGD, LOW);
  for (int n = 0; n < 4; n++)
  {
    digitalWrite(PGC, LOW);
    delayMicroseconds(1);
    digitalWrite(PGC, HIGH);
    delayMicroseconds(1);
  }
  delay(100);

  // Hold PGC LOW for atleast 200us (P10)
  digitalWrite(PGC, LOW);
  delayMicroseconds(400);
}

void readTest()
{
  int response;
  for (int n = 0; n < SIZE; n++)
  {
    response = readAddress(0, 0, n);
    SerialUSB.print(String(response, HEX) + "\r\n");
  }
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
    enterLVP();
    switch (input.charAt(0))
    {
    case 'W':
      SerialUSB.print("Bulk erasing\r\n");
      bulkErase();
      SerialUSB.print("Writing\r\n");
      writeCodeSequence(0, 0, 0, testseq);
      SerialUSB.print("Done writing\r\n");
      break;
    case 'R':
      SerialUSB.print("Reading\r\n");
      readTest();
      break;
    case 'D':
      SerialUSB.print("Reading device ID\r\n");
      readDeviceID();
      break;
    case 'E':
      SerialUSB.print("Bulk erasing\r\n");
      bulkErase();
      break;
    default:
      SerialUSB.write("Invalid serial command\r\n");
    }
    serialcomplete = false;
    input = "";
    exitLVP();
  }
}
