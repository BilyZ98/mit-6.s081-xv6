//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  printf("sock bad alloc");
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

int sockread(struct sock* sk, uint64 addr, int n){
  struct proc *pr = myproc();
  // char *ch;
  // int i;
  
  acquire(&sk->lock);
  while(mbufq_empty(&sk->rxq) && !pr->killed) {
    sleep(&sk->rxq, &sk->lock);
  }
  if(myproc()->killed){
    release(&sk->lock);
    return -1;
  }
  struct mbuf* buf = mbufq_pophead(&sk->rxq);
  release(&sk->lock);

  int len = buf->len;
  if(len > n) {
    len = n;
  }
  if(copyout(pr->pagetable, addr, buf->head, len) == -1) {
    mbuffree(buf);
    return -1;
  }
  // for(i=0; i < n; i++){
  //   ch = mbufpull(buf, 1);
  //   if(ch == 0) 
  //     break;
  //   if(copyout(pr->pagetable, addr+i, ch, 1) == -1)
  //     break;
  // }
  mbuffree(buf);

  return len;
}
int sockwrite(struct sock *sk, uint64 addr, int n){
  struct proc *pr = myproc();
  // int i;
  //char ch;

  // acquire(&sk->lock);
  struct mbuf *buf = mbufalloc(MBUF_DEFAULT_HEADROOM);
  if(!buf){
    // release(&sk->lock);
    return -1;
  }
  // for(i=0; i<n; i++){
  //   if(myproc()->killed) {
  //     // release(&sk->lock);
  //     return -1;
  //   }
  //   char *c = mbufput(buf, 1);
  //   if(copyin(pr->pagetable, c, addr+i, 1) == -1){
  //     printf("copy failed\n");
  //     mbuftrim(buf, 1);
  //     break;
  //   }
  // }
  if(copyin(pr->pagetable, mbufput(buf, n), addr, n) == -1) {
    mbuffree(buf);
    return -1;
  }
  // printf("buf content is %s\n", buf->head);
  printf("buf len is %d\n", buf->len);
  net_tx_udp(buf, sk->raddr, sk->lport, sk->rport);
  printf("remote addr %d local port %d, remote port %d\n", sk->raddr, sk->lport, sk->rport);
  // release(&sk->lock);
  return n;
}

void sockclose(struct sock *sk){
  // acquire(&sk->lock);
  acquire(&lock);
  // struct sock *pos = sockets;
  // if(pos == sk) {
  //   sockets = pos->next;
  // } else {
  //   while(pos->next != sk){
  //     pos = pos->next;
  //   }
  //   pos->next = sk->next;

  // }
  struct sock **pos;
  pos = &sockets;
  while(*pos) {
    if(*pos == sk) {
      *pos = sk->next;
      break;
    } 
    pos = &(*pos)->next;
  }
  release(&lock);

  while(!mbufq_empty(&sk->rxq)){
    mbuffree(mbufq_pophead(&sk->rxq));
  }
  // release(&sk->lock);
  kfree(sk);
}

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //

  acquire(&lock);
  struct sock *pos = sockets;
  // struct file *f;
  while(pos){
    if(pos->raddr == raddr && pos->lport ==lport && pos->rport==rport){
      break;
    }
    pos = pos->next;
  }
  if(!pos || pos->raddr != raddr || pos->lport != lport || pos->rport != rport) {
    // does this mean that someone want to connect to us?
    // what should we do?
    //int res = ;
    // if(sockalloc(&f, raddr, lport, rport) != 0) {
    //   release(&lock);
    //   return;
    // } else {
    //   pos = sockets;
    // }
    printf("not recognized packet recieved, raddr:%d, lport:%d, rport:%d\n", raddr, lport, rport);
    release(&lock);
    mbuffree(m);
    return;
  }
  // the allocated sock is at the front of the list 
  
  acquire(&pos->lock);
  mbufq_pushtail(&pos->rxq, m);
  wakeup(&pos->rxq);
  release(&pos->lock);
  //release
  // mbuffree(m);
  release(&lock);
}


