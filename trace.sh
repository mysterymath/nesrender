awk '{ cycles[$1] += int($2) } END { for (addr in cycles) print addr, cycles[addr] }' < ~/.config/mesen/trace | sort -rnk2 > /tmp/addr_cycles
#sort < ~/.config/mesen/trace | uniq -c | sort -rnk1,2 > /tmp/addr_counts
#awk '{ print $2 }' < /tmp/addr_counts | ~/mos-bin/llvm-symbolizer --obj render.nes.elf > /tmp/symbols
