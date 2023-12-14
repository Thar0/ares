auto RDRAMModule::map(RDRAM& rdram) -> void {
  s16 moduleIndex = rdram.moduleMap[id + 0];

  if(moduleIndex == -1) {
    //ID is unoccupied, map it unconditionally

    for(u32 i = 0; i < shadowMem.size / 1_MiB; i++) {
      rdram.moduleMap[id + i] = index;
    }

    if(id < 64) {
      //If the target is in the addressable region

      //Bring in shadow memory
      memcpy(&rdram.ram.data[id << 20], shadowMem.data, shadowMem.size);
    }
  } else if(index < moduleIndex) {
    //Module is higher priority than currently mapped, unmap old
    rdram.modules[moduleIndex].unmap(rdram);
    //Unmap will re-call map for this incoming module
  } else {
    //Currently mapped is higher priority, don't map this module
  }
}

auto RDRAMModule::unmap(RDRAM& rdram) -> void {
  //A module may already be unmapped if there is a higher priority module with the same device ID.
  if(rdram.moduleMap[id + 0] != index) {
    return;
  }

  //The module was mapped, unmap from the index
  for(u32 i = 0; i < shadowMem.size / 1_MiB; i++) {
    rdram.moduleMap[id + i] = -1;
  }

  if(id < 64) {
    //The module was previously mapped inside of the addressable region,
    //save the memory contents to shadow memory
    memcpy(shadowMem.data, &rdram.ram.data[id << 20], shadowMem.size);
  }

  //Try to find a module with the same device ID in priority order, map it in if found
  for(auto& module : rdram.modules) {
    if(module.id == id && module.index != index) { //We have to make sure we aren't about to remap the module we just unmapped
      //Found a match, map this module
      module.map(rdram);
      break;
    }
  }
  //NB this doesn't support mixing modules of different sizes. For example suppose you unmap a 2M modules and there's a 1M modules
  //of lower priority with id offset by 1, it won't be mapped in. Retail N64s exclusively use 2M modules

  if(id < 64 && rdram.moduleMap[id] == -1) {
    //No match was found, leaving a gap in the addressable space
    //Zero the memory region for coherency (with parallel rdp)
    memset(&rdram.ram.data[id << 20], 0, shadowMem.size);
  }
}

auto RDRAMModule::readWord(u32 address, u32 upper) -> u32 {
  n32 data = 0;

  if(upper) {
    //At would-be index 128 the first two registers are clobbered with RDRAM_ROW

    if(address < 2) {
      //RDRAM_ROW
      data.bit(25,31) = sensedRow[0].bit(0,6);
      data.bit(16,17) = sensedRow[0].bit(7,8);
      data.bit( 9,15) = sensedRow[1].bit(0,6);
      data.bit( 0, 1) = sensedRow[1].bit(7,8);
    }
  } else {
    if(address == 0) {
      //RDRAM_DEVICE_TYPE
      data = deviceType;
    }

    if(address == 1) {
      //RDRAM_DEVICE_ID
      data.bit(26,31) = idField.bit(20,25);
      data.bit(23) = idField.bit(26);
      data.bit(8,15) = idField.bit(27,34);
      data.bit(7) = idField.bit(35);
    }
  }

  if(address == 2) {
    //RDRAM_DELAY
    data = delayBits;
    data.bit(27,29) = ackWinDelay;
    data.bit(19,21) = readDelay;
    data.bit(11,12) = ackDelay;
    data.bit(3,5) = writeDelay;
  }

  if(address == 3) {
    //RDRAM_MODE
    data.bit(31) = 0; //ccEnable does not appear to be readable?
    data.bit(30) = ccMult ^ 1;
    data.bit(29) = pwrLng;
    data.bit(26) = autoSkip;
    data.bit(25) = deviceEnable;
    data.bit(19) = ackDisable;

    u32 invert = ccEnable == 0;

    data.bit( 6) = ccValue.bit(0) ^ invert;
    data.bit( 7) = ccValue.bit(3) ^ invert;
    data.bit(14) = ccValue.bit(1) ^ invert;
    data.bit(15) = ccValue.bit(4) ^ invert;
    data.bit(22) = ccValue.bit(2) ^ invert;
    data.bit(23) = ccValue.bit(5) ^ invert;
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
    data = refreshInterval;
  }

  if(address == 5) {
    //RDRAM_REF_ROW
    data.bit(25,31) = refreshRow.bit(0,6);
    data.bit(8,9) = refreshRow.bit(7,8);
    data.bit(19) = refreshBank;
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
    data.bit(24,28) = rowPrecharge;
    data.bit(16,20) = rowSense;
    data.bit( 8,12) = rowImpRestore;
    data.bit( 0, 4) = rowExpRestore;
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
    data = minInterval;
  }

  if(address == 8) {
    //RDRAM_ADDRESS_SELECT
    data.bit(25,31) = swapField.bit(0,6);
    data.bit(16,17) = swapField.bit(7,8);
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUFACTURER
    data = deviceManufacturer;
  }

  return data;
}

auto RDRAMModule::writeWord(u32 address, u32 upper, u32 data_, RDRAM& rdram) -> void {
  n32 data = data_;

  if(upper) {
    if(address < 2) {
      //RDRAM_ROW
      sensedRow[0].bit(0,6) = data.bit(25,31);
      sensedRow[0].bit(7,8) = data.bit(16,17);
      sensedRow[1].bit(0,6) = data.bit( 9,15);
      sensedRow[1].bit(7,8) = data.bit( 0, 1);
    }
  } else {
    if(address == 0) {
      //RDRAM_DEVICE_TYPE
      //read-only
    }

    if(address == 1) {
      //RDRAM_DEVICE_ID
      idField.bit(20,25) = data.bit(26,31);
      idField.bit(26) = data.bit(23);
      idField.bit(27,34) = data.bit(8,15);
      idField.bit(35) = data.bit(7);

      //Different sized RDRAMs have different idField bit lengths
      idField &= ~(shadowMem.size - 1);

      u16 old_id = id;
      u16 new_id = idField.bit(20,35);

      if(new_id != old_id) {
        //DeviceID changed, remap rdram memory
        debug(unusual, "[RDRAMModule::writeWord] deviceID changed: ", old_id, " -> ", new_id);

        //Unmap from old id
        unmap(rdram);
        //Map into new id
        id = new_id;
        map(rdram);
      }
    }
  }

  if(address == 2) {
    //RDRAM_DELAY
    ackWinDelay = data.bit(27,29);
    readDelay = data.bit(19,21);
    ackDelay = data.bit(11,12);
    writeDelay = data.bit(3,5);
  }

  if(address == 3) {
    //RDRAM_MODE
    ccEnable = data.bit(31);
    ccMult = data.bit(30);
    pwrLng = data.bit(29);
    autoSkip = data.bit(26);
    deviceEnable = data.bit(25);
    ackDisable = data.bit(19);

    ccValue.bit(0) = data.bit( 6) ^ 1;
    ccValue.bit(3) = data.bit( 7) ^ 1;
    ccValue.bit(1) = data.bit(14) ^ 1;
    ccValue.bit(4) = data.bit(15) ^ 1;
    ccValue.bit(2) = data.bit(22) ^ 1;
    ccValue.bit(5) = data.bit(23) ^ 1;

    if(ccEnable == 1) {
      //Auto cc mode, adjust manual cc value to auto value that produces the same I_OL value.
      //TOVERIFY: This is based on LG Semicon datasheet electrical characteristics, does it fit hardware?
      ccValue = (110371 + 6080 * ccValue) / 7581;
      if(ccValue > 63)
        ccValue = 63;
    }

    valid = deviceEnable && ccValue > 16 && ccValue < 40; //Arbitrary cc range, TODO work out real functional ranges
  }

  if(address == 4) {
    //RDRAM_REF_INTERVAL
    //read-only (?)
  }

  if(address == 5) {
    //RDRAM_REF_ROW
    refreshRow.bit(0,6) = data.bit(25,31);
    refreshRow.bit(7,8) = data.bit(8,9);
    refreshBank = data.bit(19);
  }

  if(address == 6) {
    //RDRAM_RAS_INTERVAL
    rowPrecharge = data.bit(24,28);
    rowSense = data.bit(16,20);
    rowImpRestore = data.bit(8,12);
    rowExpRestore = data.bit(0, 4);
  }

  if(address == 7) {
    //RDRAM_MIN_INTERVAL
    //writes don't modify the value of this register
    //TODO/TOVERIFY: RDRAM special functions
    //n5 specFunc = data.bit(0,4);
  }

  if(address == 8) {
    //RDRAM_ADDRESS_SELECT
    swapField.bit(0,6) = data.bit(25,31);
    swapField.bit(7,8) = data.bit(16,17);
    swapFieldInv = ~swapFieldInv;
  }

  if(address == 9) {
    //RDRAM_DEVICE_MANUFACTURER
    //read-only
  }
}

auto RDRAM::readWord(u32 address, Thread& thread) -> u32 {
  if(address & 0x80000) {
    debug(unusual, "[RDRAM::readWord] RDRAM register read in broadcast mode");
    //TODO what actually happens here on hardware
  }

  //Note this can span 0..511 but only 0..63 fit in the addressable range
  u32 moduleID = (address & 0x7ffff) >> 10;

  address = (address & 0x3ff) >> 2;
  u32 upper = address > 127;
  address &= 0xF;

  if(!mi.isRdramRegMode()) {
    //Most registers are not readable without RDRAM Register Select enabled in the MI.
    //A couple of registers are still readable however; namely DEVICE_TYPE, DELAY and RAS_INTERVAL.
    if(!((!upper && address == 0) || address == 2 || address == 6)) {
        debug(unusual, "[RDRAM::readWord] RDRAM register read without RDRAM register mode set in MI");
        return 0;
    }
  }

  //Get first responder
  s16 moduleIndex = moduleMap[moduleID];
  if(moduleIndex == -1) {
    debug(unusual, "[RDRAM::readWord] No responder for Device ID ", moduleID);
    return 0;
  }

  RDRAMModule& module = modules[moduleIndex];
  if(!module.deviceEnable) {
    debug(unusual, "[RDRAM::readWord] RDRAM register read without DeviceEnable set in RDRAM_MODE register");
    return 0;
  }

  u32 data = module.readWord(address, upper);

  debugger.io(Read, module.id, address, data);
  return data;
}

auto RDRAM::writeWord(u32 address, u32 data, Thread& thread) -> void {
  u32 bcast = (address & 0x80000) != 0;

  //Note this can span 0..511 but only 0..63 fit in the addressable range
  u32 moduleID = (address & 0x7ffff) >> 10;

  address = (address & 0x3ff) >> 2;
  u32 upper = address > 127;
  address &= 0xF;

  if(bcast) {
    moduleID = 512; //Dummy id for debug tracer

    //Write to all modules
    for(auto& module : modules)
      module.writeWord(address, upper, data, rdram);
  } else {
    //First responder only
    s16 moduleIndex = moduleMap[moduleID];
    if(moduleIndex == -1) {
      debug(unusual, "[RDRAM::writeWord] No responder for Device ID ", moduleID);
      return;
    }
    modules[moduleIndex].writeWord(address, upper, data, rdram);
  }

  debugger.io(Write, moduleID, address, data);
}
