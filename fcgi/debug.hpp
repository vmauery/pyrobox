#ifndef _DEBUG_HPP_
#define _DEBUG_HPP_

//#define DEBUG 1
#ifdef DEBUG

void debug_mem_init();
void debug_mem_fini();
int check_addr(void *);

#else

#define debug_mem_init()
#define debug_mem_fini()
#define check_addr()
#endif

#endif
