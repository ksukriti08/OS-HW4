// Starting code version 1.1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "mmu.h"
#include "pagetable.h"
#include "memsim.h"

int pageToEvict = 1; //if using round robin eviction, this can be used to keep track of the next page to evict

// Page table root pointer register values (one stored per process, would be "swapped in" to MMU with process, when scheduled)
typedef struct{
	int ptStartPA;
	int present;
} ptRegister;

typedef struct{
	int valid;
	int protection;
	int present;
	int referenced;
} entryBits;


// Page table root pointer register values 
// One stored for each process, swapped in to MMU when process is scheduled to run)
ptRegister ptRegVals[NUM_PROCESSES]; 

/*
 * Public Interface:
 */

/*
 * Sets the Page Table Entry (PTE) for the given process and VPN.
 * The PTE contains the PFN, valid bit, protection bit, present bit, and referenced bit.
 */

void PT_SetPTE(int pid, int VPN, int PFN, int valid, int protection, int present, int referenced) {
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	// format entry bits
    int curr_index = ptRegVals[pid].ptStartPA;
	// get page number (find where it is in physical memory using ptRegister and add offset (PFN) to get physmem location)
	entryBits entry;
	entry.valid = valid;
	entry.protection = protection;
	entry.present = present;
	entry.referenced = referenced;
	uint8_t bits = 00000000;
	bits|=(entry.valid<<0);
	bits|=(entry.protection<<1);
	bits|=(entry.present<<2);
	bits|=(entry.referenced<<3);
	physmem[curr_index + (VPN*3)] = bits;
	printf("Set entry bits %d for PID %d at physical memory: %d\n", bits, pid, curr_index + (VPN*3));
	physmem[curr_index + (VPN*3) + 1] = VPN;
	printf("Set VPN %d for PID %d at physical memory: %d\n", physmem[curr_index + (VPN*3)+1], pid, curr_index + (VPN*3)+ 1);
	physmem[curr_index + (VPN*3) + 2] = PFN;
}

void PT_UpdatePTE(int pid, int VPN, int newValue) {
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	// format entry bits
    int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
	for(int i = 0; i <16; i++){
		if (loc == 1 && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
			uint8_t bits = physmem[ptStartPA+i-1];
            bits|=(newValue<<1);
			physmem[ptStartPA+i-1] = bits;
			printf("Protection bit updated to: %d\n",bits>>1 & 1);
			break;
		}
		if(loc == 2){
			loc = -1;
		}
		loc++;
	}
}

/* 
 * Initializes the page table for the process, and sets the starting physical address for the page table.
 * After initialization, get the next free page in physical memory.
 * If there are no free pages, evict a page to get a newly freed physical address.
 * Finally, return the physical address of the next free page.
 */
int PT_PageTableInit(int pid, int pa){
	int* physmem = Memsim_GetPhysMem();
	// zero out the page we are about to use for the page table 
    for(int i = 0; i < 16; i++){
		physmem[pa+i] = 0;
	}
	// set the page table's root pointer register value
    ptRegVals[pid].ptStartPA = pa;
	ptRegVals[pid].present = 1;
	// return the PA of the next free page
	pa = Memsim_FirstFreePFN();
	// If there were no free pages,
	// Evict one and use the new space
	if(pa == -1){
		printf("No more free pages\n");
	    return -1;
	}

	return pa;
}

/* 
 * Check the ptRegVars to see if there is a valid starting PA for the given PID's page table.
 * Returns true (non-zero) or false (zero).
 */
 int PT_PageTableExists(int pid){
	if(ptRegVals[pid].ptStartPA == -1){
		return 0;
	}
	else{
		return 1;
	}
 }

/* 
 * Returns the starting physical address of the page table for the given PID.
 * If the page table does not exist, returns -1.
 * If the page table is not in memory, first swaps it in to physical memory.
 * Finally, returns the starting physical address of the page table.
 */
int PT_GetRootPtrRegVal(int pid){
	//todo
	// If the page table does not exist, return -1
    if(PT_PageTableExists(pid)==0){
		return -1;
	}
	// If the page table is not in memory, swap it in
    if(ptRegVals[pid].present == 0){
		printf("Error, page table not in memory\n");
		return -1;
	}	
	// Return the starting physical address of the page table
    return ptRegVals[pid].ptStartPA;
}

/*
 * Evicts the next page. 
 * Updates the corresponding information in the page table, returns the PA of the evicted page.
 * 
 * The supplied input and output used in autotest.sh *RR tests, uses the round-robin algorithm. 
 * You may also implement the simple and powerful Least Recently Used (LRU) policy, 
 * or another fair algorithm.
 */
int PT_Evict() {
	int* physmem = Memsim_GetPhysMem();
	FILE* swapFile = MMU_GetSwapFileHandle();

	//todo

	return 0; //todo
}

/*
 * Searches through the process's page table. If an entry is found containing the specified VPN, 
 * return the address of the start of the corresponding physical page frame in physical memory. 
 *
 * If the physical page is not present, first swaps in the phyical page from the physical disk,
 * and returns the physical address.
 * 
 * Otherwise, returns -1.
 */


int PT_VPNtoPA(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	if(ptStartPA == -1){
		return -1;
	}
	int loc = 0;
	for(int i = 0; i <16; i++){
		if (loc == 1 && physmem[ptStartPA+i] == VPN){ 
			printf("Page frame number of VPN %d: %d\n", VPN, physmem[ptStartPA+i+1]);
			printf("Start of physical address of VPN %d: %d\n", VPN, physmem[ptStartPA+i+1]*PAGE_SIZE);
			return physmem[ptStartPA+i+1]*PAGE_SIZE;
		}
		if(loc == 2){
			loc = -1;
		}
		loc++;
	}
	return -1; 
	}
/*
 * Finds the page table entry corresponding to the VPN, and checks
 * to see if the protection bit is set to 1 (readable and writable).
 * If it is 1, it returns TRUE, and FALSE if it is not found or is 0.
 */
int PT_PIDHasWritePerm(int pid, int VPN){
	int *physmem = Memsim_GetPhysMem();
    int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
	for(int i = 0; i <16; i++){
		if (loc == 1 && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
			uint8_t entrybits = physmem[ptStartPA+i-1];
			// printf("Protection bit: %d\n",entrybits>>1 & 1);
			if((entrybits>>1 & 1)==1){
				loc = 0;
				return TRUE;
			}
		}
		if(loc == 2){
			loc = -1;
		}
		loc++;
	}
	return FALSE;
}


/* 
 * Initialize the register values for each page table location (per process).
 * For example, -1 for the starting physical address of the page table, and FALSE for present.
 */
void PT_Init() {
	for(int i = 0; i <NUM_PAGES; i++){
		{
		ptRegVals[i].ptStartPA = -1;
		ptRegVals[i].present = 0;
		}
	}
	// printf("Initiliazed all pages addresses to -1 and not present\n");
}

