all: mm.c HybridScheduler.cpp
#compile C code (memory manager)
	gcc -c -o mm.o mm.c
#compile C++ code
	g++ -c -o HybridScheduler.o HybridScheduler.cpp
#use g++ to link them together
	g++ -o HybridScheduler HybridScheduler.o mm.o
#redirect all "cout" output to OUT.txt
	./HybridScheduler


clean: 
	rm -f *.o *~ 

clean_all:
	rm -f *.o  *~  HybridScheduler
