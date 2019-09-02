ofile=clib.o getopt.o macro.o stdlib.o varity.o data_struct.o global.o main.o string_lib.o function.o hal.o mempool.o struct.o gdb.o Interpreter.o operator.o toolchain.o
all: interpreter.exe
interpreter.exe: .depend $(ofile)
	g++ -m32 -g -o interpreter $(ofile)

clean:
	rm -rf *.o

.depend:	makefile $(ofile:.o=.cpp)
	g++ -Os -g -fpermissive -Wbuiltin-declaration-mismatch -Wwrite-strings -m32 -c $(ofile:.o=.cpp)
