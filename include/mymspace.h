#ifndef _MYSPACE_H_
#define _MYSPACE_H_

#define MYSPACE_PROCESS_SHARED 1

#define ONLY_MSPACES           1
# define HAVE_MMAP             1
#  undef  HAVE_MORECORE 
#define USE_LOCKS              1

#define DEFAULT_GRANULARITY    ((size_t)16U * (size_t)1024U * (size_t)1024U)

#define MMAP_SHARED_NAME       "/tmp/mmap_dlmalloc"

/* ------------------- Declarations of public routines ------------------- */
#define msmalloc(bytes)        mspace_malloc(mymspace, bytes)
#define mscalloc(elm, zie)     mspace_calloc(mymspace, elm, size)
#define msmalloc_stats()       mspace_malloc_stats(mymspace)
#define msmalloc_stats_maxfp() mspace_malloc_stats_maxfp(mymspace)
#define msfree(mem)            mspace_free(mymspace, mem)
#define msdestroy()            destroy_mspace(mymspace)

#ifndef MMALLOC
#define MMALLOC                msmalloc 
#endif

#ifndef MFREE
#define MFREE                  msfree
#endif

#endif
