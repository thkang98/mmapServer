#include "mymspace.h"
#include "malloc.h"
#include "stdio.h"

void *mymspace;
size_t mymspace_sz = 0;

void dlmmap_init(void) 
{
  /* Create mmap space with size of DEFAULT_GRANULARITY */
  mymspace = create_mspace(0, 0);
  mymspace_sz = msmalloc_stats_maxfp();
  printf("mymspace:%p sz:%lu\n", mymspace, mymspace_sz);
}
