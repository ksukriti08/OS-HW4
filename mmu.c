// Starting code version 1.0

#include <stdio.h>
#include <stdlib.h>
#include "mmu.h"
#include "memsim.h"
#include "pagetable.h"
#include "input.h"

/* Private Internals: */

// Swapping functionality
#define DISK_SWAP_FILE_PATH ((const char*) "disk.txt")
FILE* swapFileHandle;

/* Open the file to be used as swap space. */
void MMUOpenSwapFile() {
	swapFileHandle = fopen(DISK_SWAP_FILE_PATH,"w+");
}

void MMUInit() {
	Memsim_Init(); // Set up simulated physical memory system.
	MMUOpenSwapFile(); // Open swap file for use.
	PT_Init(); // Set up page table register value storage per process.
}

int MMUStart() {
	char* line;
	while (TRUE) { // continue to read input until EOF
		if (Input_GetLine(&line) < 1) { // allocate memory for each line
			printf("End of File.\n");
			return 0;
		} else {
			Input_NextInstruction(line);
		}
		free(line);
	}
}

/*
 * Public Interface:
 */

FILE* MMU_GetSwapFileHandle() {
	return swapFileHandle;
}

/*
 * The most helpful function of an MMU. 
 * Translates the VPN to find the correct physical page, then adds the offset value.
 * Our logic is in software as early ones were, but the MMU is now built as hardware accelration.
 * 
 * Traslates the VPN to find the correct physical page, then adds the offset value
 * to find the exact location of the memory reference. If the page is not mapped, return -1.
*/
int MMU_TranslateAddress(int process_id, int VPN, int offset){
	int page;
	if((page = PT_VPNtoPA(process_id, VPN)) != -1){
		return page + offset;
	} else {
		return -1;
	}
}

/* 
 * Main start of simulation of the MMU.
 * Initializes MMU and starts receiving input and executing instructions.
 */
int main () {
	/* Setup free page tracking, page table location register storage (per process), and open swap file. */
	MMUInit(); 
	/* Begin reading instructions and completing requested operations. Loops continuously. Returns when finished. */
	return MMUStart();
}