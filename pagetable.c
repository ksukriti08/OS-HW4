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

int VPN_OFFSET = 1;
int PF_OFFSET = 2;

int frametoEvict = 1; //if using round robin eviction, this can be used to keep track of the next page to evict
int swapSlot = 0;
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

int lastPTEvicted = -1;
int bringPTback = 0;

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
	// printf("Set entry bits %d for PID %d at physical memory: %d\n", bits, pid, curr_index + (VPN*3));
	// printf("Bits: %d\n", physmem[curr_index + (VPN*3)]);
	physmem[curr_index + (VPN*3) + 1] = VPN;
	// printf("Set VPN %d for PID %d at physical memory: %d\n", physmem[curr_index + (VPN*3)+1], pid, curr_index + (VPN*3)+ 1);
	physmem[curr_index + (VPN*3) + 2] = PFN;
	// printf("PFN: %d\n", PFN);
}

/* Updates protection bit to 1 of virtual page */

void PT_UpdateProtection(int pid, int VPN) {
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	// format entry bits
    int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i <16; i++){
			if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ 
				uint8_t bits = physmem[ptStartPA+i-1];
				bits|=(1<<1);
				physmem[ptStartPA+i-1] = bits;
				// printf("Protection bit updated to: %d\n",bits>>1 & 1);
				return;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
}

/* Updates present bit to 0 of virtual page */

void PT_SetNotPresent(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
				uint8_t bits = physmem[ptStartPA+i-1];
				bits&=~(1<<2);
				physmem[ptStartPA+i-1] = bits;
				printf("Present bit for virtual page %d for pid %d updated to: %d\n",VPN, pid, bits>>2 & 1);
				return;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
}

/* Updates present bit to 0 of virtual page not in memory*/

void PT_SetNotPresentF(int pid, int Frame){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	// printf("Starting address of PT: %d\n", ptStartPA);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == 2 && physmem[ptStartPA+i] == Frame){ // Also check if given PID has a page table 
				uint8_t bits = physmem[ptStartPA+i-PF_OFFSET];
				bits&=~(1<<2);
				physmem[ptStartPA+i-2] = bits;
				printf("Present bit for virtual page %d for pid %d updated to: %d\n",physmem[ptStartPA+i-1], pid, bits>>2 & 1);
				return;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
}


/* Updates present bit to 1 of virtual page */

void PT_SetPresent(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
				uint8_t bits = physmem[ptStartPA+i-1];
				bits|=(1<<2);
				physmem[ptStartPA+i-1] = bits;
				// printf("Present bit for virtual page %d for pid %d updated to: %d\n",VPN, pid, bits>>2 & 1);
				return;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
}


/* reads present bit of VPN entry, returns true if in memory and false if on disk */

int PT_CheckPresent(int pid, int VPN){
	int *physmem = Memsim_GetPhysMem();
    int ptStartPA = PT_GetRootPtrRegVal(pid);
	// printf("ptStartPA: %d\n", ptStartPA);
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int loc = 0;
	for(int i = 0; i<PAGE_SIZE; i++){
		// printf("physmem[ptStartPA+i]: %d \n", physmem[ptStartPA+i]);
		if (loc == 1 && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
			// printf("VPN here!\n");
			uint8_t entrybits = physmem[ptStartPA+i-1];
			// printf("Protection bit: %d\n",entrybits>>1 & 1);
			if(((entrybits>>2) & 1)==1){
				loc = 0;
				printf("VPN %d for pid %d IS present in physical memory\n", VPN, pid);
				return TRUE;
			}
		}
		if(loc == 2){
			loc = -1;
		}
		loc++;
	}
	printf("VPN %d for pid %d IS NOT present in physical memory\n", VPN, pid);
	return FALSE;
}


/* Updates physical address of an evicted page to location on disk using its VPN*/

void PT_UpdatePhysicalAddress(int pid, int VPN, int PA){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
				physmem[ptStartPA+i+1] = PA;
				// printf("Physical address for virtual page %d for pid %d updated to %d \n",VPN, pid, PA);
				return;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
}

/* Updates physical address of an evicted page to location on disk using its previous Frame Number*/

void PT_UpdatePhysicalAddressF(int pid, int PFN, int PA){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == 2 && physmem[ptStartPA+i] == PFN){ // Also check if given PID has a page table 
				physmem[ptStartPA+i] = PA/PAGE_SIZE;
				printf("Physical address for virtual page %d for pid %d updated to %d \n",physmem[ptStartPA+i-1], pid, PA);
				return;
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
    for(int i = 0; i < PAGE_SIZE; i++){
		physmem[pa+i] = -1; // change
	}
	// set the page table's root pointer register value
    ptRegVals[pid].ptStartPA = pa;
	ptRegVals[pid].present = 1;
	// return the PA of the next free page
	pa = Memsim_FirstFreePFN();
	if(pa!=-1){
		// printf("Free frame found: %d at address %d \n", pa/PAGE_SIZE, pa);
	}
	// If there were no free pages,
	// Evict one and use the new space
	if(pa == -1){
		// printf("No more free pages\n");
		// PT_Evict();
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
 * Check the ptRegVars to see if Page table is in memory.
 */
int PT_PageTableInMem(int pid){
	if(ptRegVals[pid].present == 0){
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
		// printf("Error, page table not in memory\n");
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
int PT_Evict(int pid) {
	
	int* physmem = Memsim_GetPhysMem();

	frametoEvict = frametoEvict % NUM_FRAMES;
	int val = 0;

	// need to get physical address of virtual page to eject 

	int starting_address = PAGE_SIZE * frametoEvict; // get current starting pa for virtual page
	// ptRegVals[pageToEvict].ptStartPA = swapSlot; // set start pa to pa on disk
	// ptRegVals[pageToEvict].present = 0; // set present to false since it is on disk
	// write all content to disk
	FILE* swapFileHandle = fopen("disk.txt","a");
	for(int i = 0; i < PAGE_SIZE; i++){
		fprintf(swapFileHandle, "%d\n", physmem[starting_address+i]);
		// printf("%d\n", physmem[starting_address+i]);
	}

	for(int i = 0; i < PAGE_SIZE; i++){ // frees corresponding frame of physical memory
		physmem[starting_address+i] = 0;
	}

	printf("Frame %d written to swap slot %d on disk \n", frametoEvict, swapSlot);
	fclose(swapFileHandle);
	UpdateFreePages(frametoEvict);
	int PT_num = PT_CheckPTEvicted(frametoEvict); // checks if a page table was evicted
	int PTforVP = PT_UpdatePTE(frametoEvict, swapSlot); // checks if virtual page has page table in memory
	if(PT_num<4){
		printf("Set starting address of page table %d to %d on disk\n", PT_num, swapSlot);
		ptRegVals[PT_num].ptStartPA = swapSlot; // sets starting address of PT to swap slot on disk
		lastPTEvicted = PT_num;
	}
	if(!PTforVP && PT_num>3&&ptRegVals[pid].ptStartPA!=-1){
		printf("We need to bring evicted page table back\n");
		bringPTback = 1;
	}
	// FILE* file = fopen("disk.txt","r");
	// while (fscanf(file, "%d", &val) == 1) {
    //     // printf("Value: %d\n", val);
    // }
	swapSlot = swapSlot+PAGE_SIZE;
	frametoEvict++; 
	return frametoEvict-1;
}

/* Checks if a page table was evicted. If yes, updates its status*/

int PT_CheckPTEvicted(int frameNum){
	int pa = frameNum * PAGE_SIZE;
	for(int i = 0; i <NUM_PAGES; i++){
		if(pa == ptRegVals[i].ptStartPA && ptRegVals[i].present)
		{
			// printf("Page table %d was evicted from frame %d\n", i, frameNum);
			// printf("Set page table %d present to %d\n", i, 0);
			ptRegVals[i].present = 0;
			return i;
		}
	}
	return 4;
}

/* Updates PTE when a page is written to disk*/

int PT_UpdatePTE(int frame, int diskOffset){
	int* physmem = Memsim_GetPhysMem();
	for(int i = 0; i < NUM_PROCESSES; i++){
		if(ptRegVals[i].present){
			int starting_address = ptRegVals[i].ptStartPA;
			int loc = 0;
			for(int j = 0; j < PAGE_SIZE; j++){
				if (loc == 2 && physmem[starting_address+j] == frame){ 
					// printf("Frame %d found in page table for pid %d \n", frame, i);
					PT_SetNotPresent(i, physmem[starting_address+j-1]); // sets virtual page of corresponding pid to not present in memmory 
					PT_UpdatePhysicalAddress(i, physmem[starting_address+j-1], swapSlot); // sets physical address to corresponding offset in disk
					return 1;
				}
				if(loc == 2){
					loc = -1;
				}
				loc++;
			}
		}
	}
	// printf("Frame %d not found in any of the page tables in memory currently \n", frame);
	return 0;

}

// need to check if present = 0 before this 

/* Searches disk for appropriate page and loads it into physical memory */

void PT_BringFromDisk(int pid, int VPN, int frameNum, int pageTable){
	int* physmem = Memsim_GetPhysMem();
	int val = 0;
	int disk_pa; 
	if(!pageTable){
		disk_pa = PT_DiskVPNtoPAf(pid, VPN);
	}
	if(pageTable){
		disk_pa = ptRegVals[pid].ptStartPA;
		// printf("Disk address for PT %d: %d\n", pid, disk_pa);
	}
	FILE* file = fopen("disk.txt","r");
	int current_pos = 0;
	int end = disk_pa + PAGE_SIZE;
	int offset = 0;
	while (fscanf(file, "%d", &val) == 1) {
		// printf("Value: %d\n", val);
		if(current_pos >= disk_pa && current_pos < end){
			// printf("Value: %d at disk pa %d\n", val, disk_pa+offset);
			physmem[frameNum*PAGE_SIZE+offset] = val; // loads disk content into physical memory
			// printf("Put %d at physical memory loc: %d \n", val, frameNum*PAGE_SIZE+offset);
			if(!pageTable){
				PT_SetPresent(pid, VPN); // sets present bit of VPN now in memory to 1
				// printf("%d\n", frameNum*PAGE_SIZE );
				PT_UpdatePhysicalAddress(pid, VPN, frameNum*PAGE_SIZE);
			}
			offset++;
			// printf("Physical address of page %d for pid %d updated to %d \n", VPN, pid, frameNum*PAGE_SIZE);
		}
		current_pos++;
    }
	// int i = 0;
	// while (fscanf(file, "%d", &val) == 1 && disk_pa >= start && disk_pa < end) {
	// 	if(i>15){
	// 		printf("Value: %d\n", val); 
	// 	}
	// 	i++;
	// }
	if(pageTable == 0){
    	printf("VPN %d for pid %d brought from disk at disk offset %d \n", VPN, pid, disk_pa);
		UpdateStoredPages(frameNum);
	}
    if(pageTable){
		printf("Page Table for pid %d brought from disk at disk offset %d \n", pid, disk_pa);
		ptRegVals[pid].ptStartPA = frameNum * PAGE_SIZE;
		ptRegVals[pid].present = 1;
		// printf("Page Table for pid %d put into frame: %d \n", pid, frameNum);
		UpdateStoredPages(frameNum);
	}
}

/* Checks if page table for currently evicted proccess is in memory */
int checkVPPTinMEM(){
	if(bringPTback == 1){
		// printf("We are bringing PT back \n");
		return 0;
	}
	return 1;
}

// checks all the page tables on the disk to see if they have the mapping for previosuly evicted virtual page in the corresponding frame

int checkDiskforPT(int evictedFrame){
	int val = 0;
	for(int i = 0; i <NUM_PAGES; i++){
		// if(ptRegVals[i].present = 1){
		// 	printf("All page tables in memory \n");
		// }
		FILE* file = fopen("disk.txt","r");
		// printf("Page 1 page table present: %d\n", ptRegVals[1].present);
		if(ptRegVals[i].present == 0 && ptRegVals[i].ptStartPA!=-1){
			int disk_pa = ptRegVals[i].ptStartPA;
			// if(disk_pa == -1){
			// 	printf("i:%d\n", i);
			// 	printf("disk_pa: %d\n", disk_pa);
			// 	break;
			// }
				printf("checking disk pa %d:\n", disk_pa);
				int current_pos = 0;
				int end = disk_pa + PAGE_SIZE;
				while (fscanf(file, "%d", &val) == 1) {
					int loc = 0;
					if(current_pos >= disk_pa && current_pos < end){
						printf("Value: %d\n", val);
						// printf("Evicted frame: %d \n", evictedFrame);
						if (val == evictedFrame){ 
							// printf("Loc: %d\n", loc);
							printf("Frame %d found in page table for pid %d \n", evictedFrame, i);
							bringPTback = 0; // reset bringPT back to 0
							return i;
						// if(loc == 3){
						// 	loc = -1;
						// }
						// loc++;
						}
					}
					current_pos++;
					// if(current_pos == end){
					// 	printf("breaking\n");
					// 	break;
					// }
				}
				fclose(file);
		}	
	}
	return -1;
}

/*
 * Searches through the process's page table. If an entry is found containing the specified VPN, 
 * return the address of the start of the corresponding physical page frame in physical memory. 
 *
 * If the physical page is not present, first swaps in the phyical page from the physical disk,
 * and returns the physical address. WHY SWAP IN PHYSICAL PAGE?
 * 
 * Otherwise, returns -1.
 */

 int PT_DiskVPNtoPAf(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	printf("Starting physical address for page table %d: %d\n", pid, ptStartPA);
	if(ptStartPA == -1){
		return -1;
	}
	int loc = 0;
	for(int i = 0; i <PAGE_SIZE; i++){

		if (loc == 1 && physmem[ptStartPA+i] == VPN ){ 
			// printf("ptStartPA: %d\n", ptStartPA);
			// printf("i: %d\n", i);
			printf("Page frame number of VPN %d for pid %d: %d\n", VPN, pid, physmem[ptStartPA+i+1]);
			printf("Start of physical address of VPN %d: %d\n", VPN, physmem[ptStartPA+i+1]);
			return physmem[ptStartPA+i+1];
		}
		if(loc == 2){
			loc = -1;
		}
		loc++;
	}
	return -1; 
	}

int PT_VPNtoPA(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	// printf("Starting physical address for page table %d: %d\n", pid, ptStartPA);
	if(ptStartPA == -1){
		return -1;
	}
	int loc = 0;
	for(int i = 0; i <PAGE_SIZE; i++){

		if (loc == 1 && physmem[ptStartPA+i] == VPN ){ 
			printf("ptStartPA: %d\n", ptStartPA);
			printf("i: %d\n", i);
			printf("Page frame number of VPN %d for pid %d: %d\n", VPN, pid, physmem[ptStartPA+i+1]);
			printf("Start of physical address of VPN %d: %d\n", VPN, physmem[ptStartPA+i+1]);
			return physmem[ptStartPA+i+1];
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
	for(int i = 0; i<PAGE_SIZE; i++){
		// printf("We are in this loop yay \n");
		if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ 
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

int checkVPNcreated(int pid, int VPN){
	int* physmem = Memsim_GetPhysMem();
	assert(PT_PageTableExists(pid)); //  table should exist if this is being called
	int ptStartPA = PT_GetRootPtrRegVal(pid);
	int loc = 0;
		for(int i = 0; i<PAGE_SIZE; i++){
			if (loc == VPN_OFFSET && physmem[ptStartPA+i] == VPN){ // Also check if given PID has a page table 
				return 1;
			}
			if(loc == 2){
				loc = -1;
			}
			loc++;
		}
	return 0;
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

int last_SwapSlot(){
	return swapSlot - PAGE_SIZE;
}

void skipFrame(){
	frametoEvict++;
}

int getFrameinLine(){
	return frametoEvict % 4;
}

// if PT not in memory and page is evicted how to handle?
// its either a page table 

 // if bringing PT evicts VPN needed how to handle? 
 // if the physical address of page table currently is same as physical address as VPN, then you know you just replaced VPN and we need to bring it back