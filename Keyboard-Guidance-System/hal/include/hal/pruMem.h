// Module to interact with PRU memory 

#ifndef _PRUMEM_H_
#define _PRUMEM_H_

volatile void* getPruMmapAddr(void);
void freePruMmapAddr(volatile void* pPruBase);


#endif