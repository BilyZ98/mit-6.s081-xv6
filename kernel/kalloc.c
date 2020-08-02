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
char *itoa(int num, char *str);
void reverse(char *str, int len);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

char memnames[NCPU][15];

void reverse(char *str, int len) {
  int i =0;
  int j = len - 1;
  while (i < j)
  {   
    char tmp = str[i];
    str[i] = str[j];
    str[j] = tmp;
  }
  /*
  for(; i < len / 2; i++, j--){
    char tmp = str[i];
    str[i] = str[j];
    str[j] = tmp;
  }
  */
}

// convert int to string with numerical base 10
char *itoa(int num, char *str) {

  if(num == 0) {
    str[0] = '0';
    str[1] = '\0';
    return str;
  }
  int i=0;
  for(; num != 0; num/=10, i++){
    int least_sig_num = num % 10;
    str[i] = '0' + least_sig_num;
  }
  reverse(str, i);
  str[i] = '\0';
  return str;
}
void
kinit()
{
  int i=0;
  //char memname[15];
  char prefix[5] = "kmem";
  for(; i < NCPU; i++) {
    memmove(memnames[i], prefix, 4);
    itoa(i, memnames[i]+4);
    //memmove(memname+4, itoa(i, memname+4), sizeof(i));
    //printf("%s\n", memname);
    initlock(&(kmem[i].lock), memnames[i]);
    //printf("%s\n",kmem[i].lock.name);
  }
  freerange(end, (void*)PHYSTOP);

}

void
freerange(void *pa_start, void *pa_end)
{
  push_off();
  //int cpu_id = cpuid();
  for(int i=0; i < NCPU; i++) {
    if(i != cpuid()) {
      kmem[i].freelist = 0;
    }
  }
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
 
  pop_off();
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  struct run *r;
  int cpu_id = cpuid();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[cpu_id].lock);
  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  release(&kmem[cpu_id].lock);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  struct run *r;
  int cpu_id = cpuid();


  acquire(&kmem[cpu_id].lock);
  r = kmem[cpu_id].freelist;
  if(r){
    kmem[cpu_id].freelist = r->next;

  }

 release(&kmem[cpu_id].lock); 
  pop_off();
  if(!r) {
    for(int i=0; i < NCPU; i++) {
      if(i != cpu_id){
        acquire(&kmem[i].lock);
        r = kmem[i].freelist;
        if(r) {
          kmem[i].freelist = r->next;
          release(&kmem[i].lock);
          break;
        }
        release(&kmem[i].lock);
      }
    }
  }
  
 
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  return (void*)r;
}
