// Starting code version 1.0

#include <stdio.h>
#include <stdint.h>

#include "memsim.h"
#include "pagetable.h"
#include "mmu.h"

/*
 * Searches the memory for a free page, and assigns it to the process's virtual address. If value is
 * 0, the page has read only permissions, and if it is 1, it has read/write permissions.
 * If the process does not already have a page table, one is assigned to it. If there are no more empty
 * pages, the mapping fails.
 */
int Instruction_Map(int pid, int va, int value_in){
	int frame;
	int* physmem = Memsim_GetPhysMem();

	if (value_in != 0 && value_in != 1) { //check for a valid value (instructions validate the value_in)
		printf("Invalid value for map instruction. Value must be 0 or 1.\n");
		return 1;
	}
	if((frame = PT_VPNtoPA(pid, VPN(va))) != -1 && PT_PIDHasWritePerm(pid, VPN(va))) {
		printf("Error: Virtual page already mapped into physical frame %d.\n", frame);
		return 1;
	} 
	if((frame = PT_VPNtoPA(pid, VPN(va))) != -1 && !PT_PIDHasWritePerm(pid, VPN(va))) {
	    PT_UpdatePTE(pid, VPN(va), value_in);
		printf("Tried to updated PTE\n");
		return 1;
	} 
	//todo
	// Find first free Physical Page (frame) and claim it for the new requested Virtual Page (containing virtual address)
    int free_page = Memsim_FirstFreePFN();
	// If no empty page was found, we must evict a page to make room
    if(free_page == -1){
		printf("Error, no free pages\n");
		return 0;
	}
	// If there isn't already a page table, create one using the page found, and find a new page

	if(!PT_PageTableExists(pid)){
		// Init the Page table  
		int next_free_page = PT_PageTableInit(pid, free_page);
		printf("Put page table for PID %d into physical frame %d.\n", pid, PFN(free_page));
		PT_SetPTE(pid, VPN(va), PFN(next_free_page), 1, value_in, 1, 1);
		printf("Mapped virtual address %d (page %d) into physical frame %d.\n", va, VPN(va), PFN(next_free_page));
	
	}
    else{
		PT_SetPTE(pid, VPN(va), PFN(free_page), 1, value_in, 1, 1);
		printf("Mapped virtual address %d (page %d) into physical frame %d.\n", va, VPN(va), PFN(free_page));

	}
	// Set the PTE with vals
	return 0;
}

/**
* If the virtual address is valid and has write permissions for the process, store
* value in the virtual address specified.
*/
int Instruction_Store(int pid, int va, int value_in){
	int pa;
	int* physmem = Memsim_GetPhysMem();

	if (value_in < 0 || value_in > MAX_VA) { //check for a valid value (instructions validate the value_in)
		printf("Invalid value for store instruction. Value must be 0-255.\n"); 
		return 1;
	}

	if (!PT_PIDHasWritePerm(pid, VPN(va))) { //check if memory is writable
		printf("Error: virtual address %d does not have write permissions.\n", va);
		return 1;
	}

	// Translate the virtual address into its physical address for the process
	pa = PT_VPNtoPA(pid, VPN(va)) + va;
	physmem[pa] = value_in;
	printf("Stored value %u at virtual address %d (physical address %d)\n", value_in, va, pa);
	// Finally stores the value in the physical memory address, mapped from the virtual address
	// Hint, modify a byte in physmem using a pa and value_in

	return 0;
}

/*
 * Translate the virtual address into its physical address for
 * the process. If the virutal memory is mapped to valid physical memory, 
 * return the value at the physical address. Permission checking is not needed,
 * since we assume all processes have (at least) read permissions on pages.
 */
int Instruction_Load(int pid, int va){
	int pa;
	int* physmem = Memsim_GetPhysMem(); 

	if (va < 0 || va > VIRTUAL_SIZE) { //check for a valid value (instructions validate the value_in)
		printf("Error: The virtual address %d is not valid.\n", va);
		return 1;
	}
	pa = PT_VPNtoPA(pid, VPN(va)) + va;
    printf("The value at virtual address %d is %d\n", va, physmem[pa]);
	return 0;
}