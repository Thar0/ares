auto RDRAMModule::serialize(serializer& s) -> void {
  s(id);
  s(index);
  s(valid);
  s(deviceType);
  s(idField);
  s(delayBits);
  s(ackWinDelay);
  s(readDelay);
  s(ackDelay);
  s(writeDelay);
  s(ccEnable);
  s(ccMult);
  s(pwrLng);
  s(autoSkip);
  s(deviceEnable);
  s(ackDisable);
  s(ccValue);
  s(refreshInterval);
  s(refreshRow);
  s(refreshBank);
  s(rowPrecharge);
  s(rowSense);
  s(rowImpRestore);
  s(rowExpRestore);
  s(minInterval);
  s(swapField);
  s(swapFieldInv);
  s(deviceManufacturer);
  s(sensedRow);
  s(shadowMem);
}

auto RDRAM::serialize(serializer& s) -> void {
  s(ram);
  s(moduleMap);
  for(auto& module : modules) {
    module.serialize(s);
  }
}
