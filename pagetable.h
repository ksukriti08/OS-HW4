// Starting code version 1.1

/*
 * Public Interface:
 */

#define NUM_PROCESSES 4

/*
 * Public Interface:
 */
void PT_SetPTE(int process_id, int VPN, int PFN, int valid, int protection, int present, int referenced);
int PT_PageTableInit(int process_id, int pageAddress);
int PT_PageTableExists(int process_id);
int PT_GetRootPtrRegVal(int process_id);
int PT_Evict();
int PT_VPNtoPA(int process_id, int VPN);
int PT_PIDHasWritePerm(int process_id, int VPN);
void PT_Init();
void PT_UpdateProtection(int process_id, int VPN);
void PT_SetNotPresent(int process_id, int VPN);
int PT_UpdatePTE(int frame, int diskOffset);
void PT_UpdatePhysicalAddress(int process_id, int VPN, int pageAddress);
int PT_CheckPTEvicted(int frameNum);
void PT_BringFromDisk(int pid, int address, int frame, int pageTable);
int PT_PageTableInMem(int pid);
void PT_SetPresent(int pid, int VPN);
int PT_CheckPresent(int pid, int VPN);
