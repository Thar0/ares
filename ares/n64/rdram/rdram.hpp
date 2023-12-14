//RAMBUS RAM

struct RDRAMModule {
  u16 id;
  u8 index; //Doubles as priority, Lower value = higher priority

  //Indicates a properly configured RDRAM, if this is unset reads/writes will not work
  u32 valid;

  //DEVICE_TYPE register
  n32 deviceType;

  //DEVICE_ID register
  n36 idField; //Bits [35:20+n] compared with Adr[35:20+n] in request pkt (where n=0 for 1M, n=1 for 2M, etc.)

  //DELAY register
  n32 delayBits; // read-only
  n3 ackWinDelay;
  n3 readDelay;
  n2 ackDelay;
  n3 writeDelay;

  //MODE register
  n1 ccEnable;
  n1 ccMult;
  n1 pwrLng;
  n1 autoSkip;
  n1 deviceEnable;
  n1 ackDisable;
  n6 ccValue; //TODO ccValue changes over time in auto mode

  //REF_INTERVAL register
  n32 refreshInterval; //Always 0

  //REF_ROW register
  n9 refreshRow;
  n1 refreshBank;

  //RAS_INTERVAL register
  n5 rowPrecharge;
  n5 rowSense;
  n5 rowImpRestore;
  n5 rowExpRestore;

  //MIN_INTERVAL register
  n32 minInterval;

  //ADDRESS_SELECT register
  n9 swapField;
  n9 swapFieldInv;

  //DEVICE_MANUFACTURER register
  n32 deviceManufacturer;

  //ROW register
  n9 sensedRow[2];

  //Shadowed memory contents for when multiple devices share the same DeviceID and it is not the first responder
  Memory::Writable shadowMem;

  //io.cpp
  auto map(struct RDRAM& rdram) -> void;
  auto unmap(struct RDRAM& rdram) -> void;
  auto readWord(u32 address, u32 upper) -> u32;
  auto writeWord(u32 address, u32 upper, u32 data_, struct RDRAM& rdram) -> void;

  //serialization.cpp
  auto serialize(serializer& s) -> void;
};

struct RDRAM : Memory::RCP<RDRAM> {
  Node::Object node;

  struct Writable : public Memory::Writable {
    RDRAM& self;

    Writable(RDRAM& self) : self(self) {}

    template<u32 Size>
    auto read(u32 address, const char* peripheral) -> u64 {
      if (peripheral && system.homebrewMode) {
        self.debugger.readWord(address, Size, peripheral);
      }

      u32 moduleID = address >> 20; //0..63
      s16 moduleNum = rdram.moduleMap[moduleID];

      ri.io.banksValid |= (moduleID < 8) << moduleID;
      ri.io.overRangeErr |= (moduleID >= 8);
      ri.io.ackErr |= moduleNum == -1;

      if (likely(moduleNum != -1 && rdram.modules[moduleNum].valid)) {
        rdram.modules[moduleNum].sensedRow[moduleID & 1] = (address >> 11) & 0x1FF;
        return Memory::Writable::read<Size>(address);
      }
      return 0;
    }

    template<u32 Size>
    auto write(u32 address, u64 value, const char* peripheral) -> void {
      if (peripheral && system.homebrewMode) {
        self.debugger.writeWord(address, Size, value, peripheral);
      }

      u32 moduleID = address >> 20; //0..63
      s16 moduleNum = rdram.moduleMap[moduleID];

      ri.io.banksValid |= (moduleID < 8) << moduleID;
      ri.io.banksDirty |= (moduleID < 8) << moduleID;
      ri.io.overRangeErr |= (moduleID >= 8);
      ri.io.ackErr |= moduleNum == -1;

      //Kill writes to invalid modules
      if (likely(moduleNum != -1 && rdram.modules[moduleNum].valid)) {
        rdram.modules[moduleNum].sensedRow[moduleID & 1] = (address >> 11) & 0x1FF;
        Memory::Writable::write<Size>(address, value);
      }
    }

    template<u32 Size>
    auto writeBurst(u32 address, u32 *value, const char *peripheral) -> void {
      if (address >= size) return;
      Memory::Writable::write<Word>(address | 0x00, value[0]);
      Memory::Writable::write<Word>(address | 0x04, value[1]);
      Memory::Writable::write<Word>(address | 0x08, value[2]);
      Memory::Writable::write<Word>(address | 0x0c, value[3]);
      if (Size == ICache) {
        Memory::Writable::write<Word>(address | 0x10, value[4]);
        Memory::Writable::write<Word>(address | 0x14, value[5]);
        Memory::Writable::write<Word>(address | 0x18, value[6]);
        Memory::Writable::write<Word>(address | 0x1c, value[7]);
      }
    }

    template<u32 Size>
    auto readBurst(u32 address, u32 *value, const char *peripheral) -> void {
      if (address >= size) {
        value[0] = value[1] = value[2] = value[3] = 0;
        if (Size == ICache)
          value[4] = value[5] = value[6] = value[7] = 0;
        return;
      }
      value[0] = Memory::Writable::read<Word>(address | 0x00);
      value[1] = Memory::Writable::read<Word>(address | 0x04);
      value[2] = Memory::Writable::read<Word>(address | 0x08);
      value[3] = Memory::Writable::read<Word>(address | 0x0c);
      if (Size == ICache) {
        value[4] = Memory::Writable::read<Word>(address | 0x10);
        value[5] = Memory::Writable::read<Word>(address | 0x14);
        value[6] = Memory::Writable::read<Word>(address | 0x18);
        value[7] = Memory::Writable::read<Word>(address | 0x1c);
      }
    }

  } ram{*this};

  struct Debugger {
    u32 lastReadCacheline    = 0xffff'ffff;
    u32 lastWrittenCacheline = 0xffff'ffff;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 moduleID, u32 address, u32 data) -> void;
    auto readWord(u32 address, int size, const char *peripheral) -> void;
    auto writeWord(u32 address, int size, u64 value, const char *peripheral) -> void;
    auto cacheErrorContext(string peripheral) -> string;

    struct Memory {
      Node::Debugger::Memory ram;
      Node::Debugger::Memory dcache;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //rdram.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //Instanced modules, 4x 2MB
  RDRAMModule modules[4];

  //Contains the index of the module that is the first responder for a given device id.
  //May also contain -1 indicating no module mapped for that id.
  s16 moduleMap[512];
};

extern RDRAM rdram;
