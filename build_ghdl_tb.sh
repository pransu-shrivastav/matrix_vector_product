#

# build ghdl testbench
#
rm -rf obj_vhdl
mkdir obj_vhdl
gcc -c testbench/testbench.c -I ./ -I $AHIR_RELEASE/include  -o obj_vhdl/testbench.o 
gcc -c vhdlCStubs.c -I ./ -I $AHIR_RELEASE/include  -o obj_vhdl/vhdlCStubs.o 
gcc -o bin/testbench_vhdl obj_vhdl/vhdlCStubs.o obj_vhdl/testbench.o -L$AHIR_RELEASE/lib -lBitVectors -lSockPipes -lSocketLibPipeHandler  -lpthread 

