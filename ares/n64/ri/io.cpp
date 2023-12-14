auto RI::readWord(u32 address, Thread& thread) -> u32 {
  address = (address & 0x1f) >> 2;
  n32 data = 0;

  if(address == 0) {
    //RI_MODE
    data.bit(0,1) = io.operatingMode;
    data.bit(2) = io.stopT;
    data.bit(3) = io.stopR;
  }

  if(address == 1) {
    //RI_CONFIG
    data.bit(0,5) = io.CCtlI;
    data.bit(6) = io.CCtlEn;
  }

  if(address == 2) {
    //RI_CURRENT_LOAD
    //TOVERIFY: are bits 1 and 2 sourced from somewhere, or are they fixed to 1?
    data.bit(0) = io.ackErr;
    data.bit(1) = 1;
    data.bit(2) = 1;
    data.bit(3) = io.stopR;
    data.bit(4) = io.txSelect.bit(0);
  }

  if(address == 3) {
    //RI_SELECT
    data.bit(0,3) = io.rxSelect;
    data.bit(4,7) = io.txSelect;
  }

  if(address == 4) {
    //RI_REFRESH
    data.bit(0,7) = io.refreshDelayClean;
    data.bit(8,15) = io.refreshDelayDirty;
    data.bit(16) = io.refreshBank;
    data.bit(17) = io.refreshEnable;
    data.bit(18) = io.refreshOptimize;
    data.bit(19,22) = io.refreshMultibank;
  }

  if(address == 5) {
    //RI_LATENCY
    data = io.latency;
  }

  if(address == 6) {
    //RI_ERROR
    data.bit(0) = io.ackErr;
    data.bit(1) = io.nackErr;
    data.bit(2) = io.overRangeErr;
  }

  if(address == 7) {
    //RI_BANK_STATUS
    data.bit(0,7) = io.banksValid;
    data.bit(8,15) = io.banksDirty;
  }

  debugger.io(Read, address, data);
  return data;
}

auto RI::writeWord(u32 address, u32 data_, Thread& thread) -> void {
  address = (address & 0x1f) >> 2;
  n32 data = data_;

  if(address == 0) {
    //RI_MODE
    io.operatingMode = data.bit(0,1);
    io.stopT = data.bit(2);
    io.stopR = data.bit(3);
  }

  if(address == 1) {
    //RI_CONFIG
    io.CCtlI = data.bit(0,5);
    io.CCtlEn = data.bit(6);
  }

  if(address == 2) {
    //RI_CURRENT_LOAD
    //TODO: writes trigger an async load of CCtlI value in the RAC
  }

  if(address == 3) {
    //RI_SELECT
    io.rxSelect = data.bit(0,3);
    io.txSelect = data.bit(4,7);
  }

  if(address == 4) {
    //RI_REFRESH
    io.refreshDelayClean = data.bit(0,7);
    io.refreshDelayDirty = data.bit(8,15);
    io.refreshBank = data.bit(16);
    io.refreshEnable = data.bit(17);
    io.refreshOptimize = data.bit(18);
    io.refreshMultibank = data.bit(19,22);
  }

  if(address == 5) {
    //RI_LATENCY
    io.latency = data;
  }

  if(address == 6) {
    //RI_ERROR
    //Any write clears bits
    io.ackErr = 0;
    io.nackErr = 0;
    io.overRangeErr = 0;
  }

  if(address == 7) {
    //RI_BANK_STATUS
    //Any write resets bits
    io.banksDirty = 0b11111111;
    io.banksValid = 0b00000000;
  }

  debugger.io(Write, address, data);
}
