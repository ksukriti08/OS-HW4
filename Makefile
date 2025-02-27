# Starting code version 1.0 
all: mmu

mmu: mmu.o input.o pagetable.o memsim.o instruction.o
	gcc mmu.o input.o pagetable.o memsim.o instruction.o -o mmu

mmu.o: mmu.c 
	gcc -c mmu.c -o mmu.o

input.o: input.c input.h
	gcc -c input.c -o input.o

pagetable.o: pagetable.c pagetable.h
	gcc -c pagetable.c -o pagetable.o

memsim.o: memsim.c memsim.h
	gcc -c memsim.c -o memsim.o

instruction.o: instruction.c instruction.h
	gcc -c instruction.c -o instruction.o

clean:
	rm mmu *.o
