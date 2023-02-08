file = io.open("trace", "w")
io.output(file)

function callback(address, value)
  prg = emu.getPrgRomOffset(address)
  if prg == -1 then goto continue end 
  bank = prg // 0x4000
  vma = address
  if address < 0xc000 then
    vma = vma | bank << 16
  end
  io.write(string.format("%08x\n", vma))
  ::continue::
end
emu.addMemoryCallback(callback, emu.memCallbackType.cpuExec, 0)
