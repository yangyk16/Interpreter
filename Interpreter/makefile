ofile = *.o
all: interpreter.exe
interpreter.exe: $(ofile)
	g++ -m32 -o interpreter *.o
$(ofile): *.cpp
	g++ -Os -fpermissive -m32 -c *.cpp

clean:
	rm -rf *.o
