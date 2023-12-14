//RDRAM Interface

struct RI : Memory::RCP<RI> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //ri.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    //RI_MODE
    n2 operatingMode;
    n1 stopT;
    n1 stopR;

    //RI_CONFIG
    n6 CCtlI;
    n1 CCtlEn;

    //RI_SELECT
    n4 rxSelect;
    n4 txSelect;

    //RI_REFRESH
    n8 refreshDelayClean;
    n8 refreshDelayDirty;
    n1 refreshBank;
    n1 refreshEnable;
    n1 refreshOptimize;
    n4 refreshMultibank;

    //RI_LATENCY
    n4 latency;

    //RI_ERROR
    n1 ackErr;
    n1 nackErr;
    n1 overRangeErr;

    //RI_BANK_STATUS
    n8 banksValid;
    n8 banksDirty;
  } io;
};

extern RI ri;
