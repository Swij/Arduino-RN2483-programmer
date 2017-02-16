#ifndef RN2483LVP_H
#define RN2483LVP_H

#include "Arduino.h"
#define SIZE 14

// RN2483 low-voltage programmer
class RN2483LVP
{
public:
  RN2483LVP();
  void initRN2483LVP(int pgd, int pgc, int mclr, Stream& usbserial);
  void enterLVP();
  void exitLVP();
  int sendOp(int cmd, int payload);
  void setTBLPTR(int addru, int addrh, int addrl);
  int readAddress(int addru, int addrh, int addrl);
  void readDeviceID();
  void writeAddressConfig(int addru, int addrh, int addrl, int value);
  void bulkErase();
  void writeCodeSequence(int addru, int addrh, int addrl, int bytes[]);
  void printCodeMemory(int start, int stop);

private:
  Stream* usbserial;
  int keyseq[32] = {0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0};
  int pgd;
  int pgc;
  int mclr;
  bool inLVP;
};

#endif
