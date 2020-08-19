#ifndef MMAP_H
#define MMAP_H




//#include "spinlock.h"

#define NUM_VNA 20


// virtual memory area
struct VMA {
  struct VMA *next;

  uint64 addr;
  uint64 length;
  struct file *f;
  uint64 permission;
  uint64 flags;
  uint64 offset;
  uint64 read_offset;
  uint64 write_offset;
  int unmapped_size;

  int pid;
  //struct spinlock lock;

};

#endif



