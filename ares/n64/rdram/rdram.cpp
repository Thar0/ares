#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDRAM rdram;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RDRAM::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RDRAM");

  //Allocates enough memory to fill the entire RDRAM memory address space
  ram.allocate(64_MiB);

  debugger.load(node);
}

auto RDRAM::unload() -> void {
  debugger = {};
  ram.reset();
  node.reset();
}

auto RDRAM::power(bool reset) -> void {
  if(!reset) {
    ram.fill();

    for(unsigned i = 0; i < 512; i++)
      moduleMap[i] = -1;

    u32 i = 0;
    for(auto& module : modules) {
      if (!system.expansionPak && i == 2)
        break;

      //Initialize all as RDRAM36 modules

      module.id = 0;
      module.index = i++;

      module.valid = 0;

      module.deviceType.bit(28,31) = 11; //11 column address bits
      module.deviceType.bit(26)    =  1; //9 bits per byte
      module.deviceType.bit(24)    =  0; //Not "enhanced speed" model
      module.deviceType.bit(20,23) =  1; //1 bank address bit
      module.deviceType.bit(16,19) =  9; //9 row address bits
      module.deviceType.bit( 4, 7) =  1; //Version 1
      module.deviceType.bit( 0, 3) =  0; //Type 0

      module.idField = 0;

      module.delayBits = 0x0303'0203;
      module.ackWinDelay = 0;
      module.readDelay = 0;
      module.ackDelay = 0;
      module.writeDelay = 0;

      module.ccEnable = 0;
      module.ccMult = 0;
      module.pwrLng = 0;
      module.autoSkip = 1;
      module.deviceEnable = 0;
      module.ackDisable = 0;
      module.ccValue = 63;

      module.refreshInterval = 0;

      module.refreshRow = 0;
      module.refreshBank = 0;

      module.rowPrecharge = 0;
      module.rowSense = 0;
      module.rowImpRestore = 0;
      module.rowExpRestore = 0;

      module.minInterval = 0x0040'C0E0;

      module.swapField = 0;
      module.swapFieldInv = ~0;

      module.deviceManufacturer = 0x0000'0500; // NEC

      module.sensedRow[0] = 0;
      module.sensedRow[1] = 0;

      //Reset shadow memory for mapping
      module.shadowMem.reset();
      module.shadowMem.allocate(2_MiB);
      module.shadowMem.fill();

      module.map(*this);
    }
  }
}

}
