file = io.open("trace", "w")
io.output(file)


vma = 0
prevCycleCount = 0
function callback(address, value)
  cycleCount = emu.getState().cpu.cycleCount
  prg = emu.getPrgRomOffset(address)
  if vma ~= 0 then
    io.write(string.format("0x%08x %d\n", vma, cycleCount - prevCycleCount))
  end
  if prg == -1 then
    vma = 0
    goto continue
  end
  bank = prg // 0x4000
  vma = address
  if address < 0xc000 then
    vma = vma | bank << 16
  end
  prevCycleCount = cycleCount
  ::continue::
end
emu.addMemoryCallback(callback, emu.memCallbackType.cpuExec, 0)
