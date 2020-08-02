// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NUMBUCKET 3
//static const uint NUMBUCKET = 3;

struct bucket{
  struct spinlock lock;
  struct buf buf;
};


struct {
  struct spinlock lock[NUMBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head[NUMBUCKET];
} bcache;


/*
struct {
  struct bucket bkt[NUMBUCKET];
  struct spinlock lock;
}bcache;
*/
uint hash(uint blockno) {
  return blockno % NUMBUCKET;
}

void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");
  for(int i=0; i < NUMBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache.bucket");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }


  int i=0;
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.head[i].next;
    b->prev = &bcache.head[i];
    initsleeplock(&b->lock, "buffer");
    bcache.head[i].next->prev = b;
    bcache.head[i].next = b;
    i = (i+1) % NUMBUCKET;
  } 

  // Create linked list of buffers
  /*
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  */
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  //acquire(&bcache.lock);

  uint bucketid = hash(blockno);
  acquire(&bcache.lock[bucketid]);
  struct buf *bufptr;
  for(bufptr = bcache.head[bucketid].next; bufptr != &bcache.head[bucketid]; bufptr = bufptr->next){
    if(bufptr->dev == dev && bufptr->blockno == blockno) {
      bufptr->refcnt++;
      release(&bcache.lock[bucketid]);
      acquiresleep(&bufptr->lock);
      return bufptr;
    }
  }

  for(bufptr = bcache.head[bucketid].prev; bufptr != &bcache.head[bucketid]; bufptr = bufptr->prev){
    if(bufptr->refcnt == 0){
      bufptr->dev = dev;
      bufptr->blockno = blockno;
      bufptr->valid = 0;
      bufptr->refcnt = 1;
      release(&bcache.lock[bucketid]);
      acquiresleep(&bufptr->lock);
      return bufptr;
    }
  }

  panic("bget: no buffers");


  /*
  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  */


  // Not cached; recycle an unused buffer.
  /*
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  */
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bid = hash(b->blockno);

  acquire(&bcache.lock[bid]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bid].next;
    b->prev = &bcache.head[bid];
    bcache.head[bid].next->prev = b;
    bcache.head[bid].next = b;
  }
  
  release(&bcache.lock[bid]);
}

void
bpin(struct buf *b) {
  uint bid = hash(b->blockno);
  acquire(&bcache.lock[bid]);
  b->refcnt++;
  release(&bcache.lock[bid]);
}

void
bunpin(struct buf *b) {
  uint bid = hash(b->blockno);
  acquire(&bcache.lock[bid]);
  b->refcnt--;
  release(&bcache.lock[bid]);
}


