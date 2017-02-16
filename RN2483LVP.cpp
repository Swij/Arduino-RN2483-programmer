#include "RN2483LVP.h"

RN2483LVP::RN2483LVP()
{
    this->pgd = 5;
    this->pgc = 6;
    this->mclr = 4;
    this->inLVP = false;
}

//Constructor
void RN2483LVP::initRN2483LVP(int pgd, int pgc, int mclr, Stream& usbserial)
{
    this->pgd = pgd;
    this->pgc = pgc;
    this->mclr = mclr;
    this->usbserial = &usbserial;
    this->inLVP = false;
}

// Enter low-voltage program/verify mode
void RN2483LVP::enterLVP()
{
    if (not(inLVP))
    {
        inLVP = true;
        SerialUSB.write("Entering low-voltage program/verify mode\r\n");

        // Pull all programming pins Low
        digitalWrite(mclr, LOW);
        digitalWrite(pgd, LOW);
        digitalWrite(pgc, LOW);

        // Wait atleast 100ns (P13)
        delayMicroseconds(1);

        // Set mclr High then Low
        digitalWrite(mclr, HIGH);
        delayMicroseconds(1);
        digitalWrite(mclr, LOW);

        // Wait atleast 1ms (P18)
        delayMicroseconds(2);

        // Enter key sequence on pgd
        for (int n = 0; n < 32; n++)
        {
            //Lower pgc and write bit to pgd
            digitalWrite(pgc, LOW);
            digitalWrite(pgd, keyseq[n]);
            delayMicroseconds(1);

            //Raise pgc to send bit
            digitalWrite(pgc, HIGH);
            delayMicroseconds(1);
        }
        digitalWrite(pgc, LOW);

        // Wait atleast 40ns
        delayMicroseconds(1);

        // Set mclr High
        digitalWrite(mclr, HIGH);

        // Wait atleast 400us
        delayMicroseconds(450);
    }
}

// Exit low-voltage program/verify mode
void RN2483LVP::exitLVP()
{
    inLVP = false;
    SerialUSB.write("Exiting low-voltage program/verify mode\r\n");
    delayMicroseconds(1);
    digitalWrite(mclr, LOW);
    delayMicroseconds(10);
    digitalWrite(mclr, HIGH);
}

// Serial program/verify operation
int RN2483LVP::sendOp(int cmd, int payload)
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
            //Lower pgc and write bit to pgd
            digitalWrite(pgc, LOW);
            digitalWrite(pgd, bitRead(cmd, n));
            delayMicroseconds(1);

            //Raise pgc to send bit
            digitalWrite(pgc, HIGH);
            delayMicroseconds(1);
        }
        digitalWrite(pgc, LOW);

        // Wait atleast 40ns (P5)
        delayMicroseconds(1);

        if (isread)
        {
            // Write 8 payload bits to RN2483
            for (int n = 0; n < 8; n++)
            {
                //Lower pgc and write bit to pgd
                digitalWrite(pgc, LOW);
                digitalWrite(pgd, bitRead(payload, n));
                delayMicroseconds(1);

                //Raise pgc to send bit
                digitalWrite(pgc, HIGH);
                delayMicroseconds(1);
            }
            digitalWrite(pgc, LOW);

            // Set pgd to input
            pinMode(pgd, INPUT);

            // Wait atleast 20ns (P6)
            delayMicroseconds(1);

            // Read 8 response bits from RN2483
            int response = 255;
            for (int n = 0; n < 8; n++)
            {
                //Raise pgc to recieve bit
                digitalWrite(pgc, HIGH);
                delayMicroseconds(1);

                //Lower pgc and read bit from pgd
                digitalWrite(pgc, LOW);
                bitWrite(response, n, digitalRead(pgd));
                delayMicroseconds(1);
            }

            // Set pgd to output
            pinMode(pgd, OUTPUT);

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
                //Lower pgc and write bit to pgd
                digitalWrite(pgc, LOW);
                digitalWrite(pgd, bitRead(payload, n));
                delayMicroseconds(1);

                //Raise pgc to send bit
                digitalWrite(pgc, HIGH);
                delayMicroseconds(1);
            }
            digitalWrite(pgc, LOW);

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

// Sets the TBLPTR to the address to be read/written
void RN2483LVP::setTBLPTR(int addru, int addrh, int addrl)
{
    sendOp(B0000, 0x0E00 + addru); // MOVLW <Addr[21:16]>
    sendOp(B0000, 0x6EF8);         // MOVWF TBLPTRU
    sendOp(B0000, 0x0E00 + addrh); // MOVLW <Addr[15:8]>
    sendOp(B0000, 0x6EF7);         // MOVWF TBLPTRH
    sendOp(B0000, 0x0E00 + addrl); // MOVLW <Addr[7:0]>
    sendOp(B0000, 0x6EF6);         // MOVWF TBLPTRL
}

// Read value at address
int RN2483LVP::readAddress(int addru, int addrh, int addrl)
{
    setTBLPTR(addru, addrh, addrl);
    return sendOp(B1001, 0x0000); // TBLRD *+
}

// Print device IDs on serial
void RN2483LVP::readDeviceID()
{
    int DEVID1 = readAddress(0x3F, 0xFF, 0xFE);
    int DEVID2 = readAddress(0x3F, 0xFF, 0xFF);
    SerialUSB.print("DEVID1: " + String(DEVID1, HEX) + "h\r\n");
    SerialUSB.print("DEVID2: " + String(DEVID2, HEX) + "h\r\n");
}

// Write value to address in config block
void RN2483LVP::writeAddressConfig(int addru, int addrh, int addrl, int value)
{
    setTBLPTR(addru, addrh, addrl);
    sendOp(B1100, value);
}

// Bulk erase the entire device
void RN2483LVP::bulkErase()
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

// Writes a sequence of bytes into the code memmory
// bytes.length <= 64, bytes[n] <= 0xFFFF
void RN2483LVP::writeCodeSequence(int addru, int addrh, int addrl, int bytes[])
{
    int LoopCount = 0;

    // Bulk erase chip
    bulkErase();

    // Direct access to code memory
    sendOp(B0000, 0x8EA6); // BSF EECON1, EEpgd
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

    // Send NOP and Hold 4th pgc HIGH for atleast 1ms (P9)
    digitalWrite(pgd, LOW);
    for (int n = 0; n < 4; n++)
    {
        digitalWrite(pgc, LOW);
        delayMicroseconds(1);
        digitalWrite(pgc, HIGH);
        delayMicroseconds(1);
    }
    delay(100);

    // Hold pgc LOW for atleast 200us (P10)
    digitalWrite(pgc, LOW);
    delayMicroseconds(400);

    exitLVP();
    enterLVP();
}

void RN2483LVP::printCodeMemory(int start, int stop)
{
    int response;
    for (int n = start; n < stop; n++)
    {
        response = readAddress(0, 0, n);
        SerialUSB.print(String(response, HEX) + "\r\n");
    }
}
