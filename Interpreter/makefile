ofile = *.o
all: interpreter.exe
interpreter.exe: $(ofile)
	g++ -m32 -g -o interpreter *.o
$(ofile): *.cpp
	g++ -Os -g -fpermissive -Wbuiltin-declaration-mismatch -Wwrite-strings -m32 -c *.cpp

clean:
	rm -rf *.o
