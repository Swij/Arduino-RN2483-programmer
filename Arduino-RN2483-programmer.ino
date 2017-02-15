// Arduino RN2483 programmer
// Implemented using http://ww1.microchip.com/downloads/en/DeviceDoc/41398B.pdf
#define MCLR 4
#define PGD 5
#define PGC 6

int keyseq[]  = {0,1,0,0, 1,1,0,1, 0,1,0,0, 0,0,1,1, 0,1,0,0, 1,0,0,0, 0,1,0,1, 0,0,0,0};

void setup() {
  // put your setup code here, to run once:
  SerialUSB.begin(57600);

  // Wait for serialUSB or start after 30 seconds
  while ((!SerialUSB) && (millis() < 30000));
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
void enterLVP() {
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
  for (int n = 0; n < 32; n++) {    
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

// Exit low-voltage program/verify mode
void exitLVP() {
  SerialUSB.write("Exiting low-voltage program/verify mode\r\n");
  delayMicroseconds(1);
  digitalWrite(MCLR, LOW);
  delayMicroseconds(10);
  digitalWrite(MCLR, HIGH);
}

// Serial program/verify operation
int sendOp(int cmd, int payload) {
  bool valid = true;
  bool isread = false;

  // Check if command is valid and if it contains a read
  switch (cmd) {
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
  if ((isread && (payload > 255)) || (!isread && (payload > 65535))) {
    SerialUSB.write("Invalid number of payload bits\r\n");
    return false;
  } 
  
  if (valid) {
    // Send command bits to the RN2483
    for (int n = 0; n < 4; n++) {
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
    
    if (isread) {
      // Write 8 payload bits to RN2483
      for (int n = 0; n < 8; n++) {
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
      for (int n = 0; n < 8; n++) {
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
      SerialUSB.write("Sent 4 command bits and 8 data payload bits\r\nResponse: ");
      SerialUSB.print(String(response, HEX)+"\r\n");
      
      return response;
    } else {
      // Write all 16 bits from the data payload      
      for (int n = 0; n < 16; n++) {
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
      SerialUSB.write("Sent 4 command bits and 16 data payload bits\r\n");

      return true;
    }
  } else {
    SerialUSB.write("Command not valid\r\n");
    return false;
  }
}

void loop() {
  if(SerialUSB.available()){
    char ch = SerialUSB.read();
    switch(ch){
      case '1':
        SerialUSB.write("Read address 3FFFFE\r\n");
        sendOp(B0000, 0x0E3F);
        sendOp(B0000, 0x6EF8);
        sendOp(B0000, 0x0EFF);
        sendOp(B0000, 0x6EF7);
        sendOp(B0000, 0x0EFE);
        sendOp(B0000, 0x6EF6);
        sendOp(B1001, 0x0000);
        break;
      case '2':
        SerialUSB.write("Read address 3FFFFF\r\n");
        sendOp(B0000, 0x0E3F);
        sendOp(B0000, 0x6EF8);
        sendOp(B0000, 0x0EFF);
        sendOp(B0000, 0x6EF7);
        sendOp(B0000, 0x0EFF);
        sendOp(B0000, 0x6EF6);
        sendOp(B1001, 0x0000);
        break;
      case '3':
        exitLVP();
        enterLVP();
        break;
      case '4':
        exitLVP();
        break;
      case '5':
        break;
      case '\r':
        break;
      case '\n':
        break;
      default:
        SerialUSB.write("Invalid input\r\n");
    }
  }
}
