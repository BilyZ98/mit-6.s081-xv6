// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct  {
  char *alloc_base;
  char *mark_base;
  struct spinlock lock;
} refcnt;

void refinc(char *addr);

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  //initlock(&refcnt.lock, "refcnt");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  //acquire(&refcnt.lock);
  refcnt.mark_base =  p;
  refcnt.alloc_base = p + 8 * PGSIZE;
  memset(refcnt.mark_base, 1, 8*PGSIZE);
  //release(&refcnt.lock);
  p = refcnt.alloc_base;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&refcnt.lock);
  char ref = --refcnt.mark_base[((char*)pa - refcnt.alloc_base) / PGSIZE];
  if(ref > 0){
    release(&refcnt.lock);
    return;
  }
  release(&refcnt.lock);
 

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;


  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    refinc((char*)r);
  } else {
    printf("kalloc: invalid addredd\n");
  }
  return (void*)r;
}

void refinc(char *addr) {
  acquire(&refcnt.lock);
  if(addr < refcnt.alloc_base) 
    panic("refinc: address invalid");
  if(refcnt.mark_base[ (addr - refcnt.alloc_base)/PGSIZE] < 0){
    panic("refinc: ref less than 0");
  }
  refcnt.mark_base[(addr - refcnt.alloc_base)/PGSIZE]++;
  release(&refcnt.lock);
}