sort < ~/.config/mesen/trace | uniq -c | sort -rnk1 > /tmp/addr_counts
awk '{ print $2 }' < /tmp/addr_counts | ~/mos-bin/llvm-symbolizer --obj render.nes.elf > /tmp/symbols
awk '{ print $1, $2; do { getline < "/tmp/symbols"; print } while(NF) }' < /tmp/addr_counts
