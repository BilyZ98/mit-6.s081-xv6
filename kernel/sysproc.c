#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

typedef void (*func)();

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void) {
  int n;
  uint64 func_pointer;
  //func func_ptr;
  //uint64 func_pointer_64;
  if(argint(0, &n) < 0 || argaddr(1, &func_pointer)) {
    return -1;
  }

  //func_pointer_64 = func_pointer;
  struct proc* p = myproc();
  p->alarm_interval = n;
  //func_ptr = reinterpret_cast<func> func_pointer;
  p->handler = func_pointer;
  p->tick_count = -1;

  return 0;
}

uint64 
sys_sigreturn(void) {
  struct proc *p = myproc();
  p->tf->epc = p->alarmcontext.epc;
  p->tf->ra = p->alarmcontext.ra;
  p->tf->sp = p->alarmcontext.sp;
  p->tf->gp = p->alarmcontext.gp;
  p->tf->tp = p->alarmcontext.tp;
  p->tf->t0 =   p->alarmcontext.t0;
  p->tf->t1 =   p->alarmcontext.t1;
  p->tf->t2 =  p->alarmcontext.t2;
  p->tf->s0 = p->alarmcontext.s0;
  p->tf->s1 =  p->alarmcontext.s1;
  p->tf->a0 =   p->alarmcontext.a0;
  p->tf->a1 =  p->alarmcontext.a1;
  p->tf->a2 =  p->alarmcontext.a2;
  p->tf->a3 = p->alarmcontext.a3;
  p->tf->a4 = p->alarmcontext.a4;
  p->tf->a5 = p->alarmcontext.a5;
  p->tf->a6 = p->alarmcontext.a6;
  p->tf->a7 = p->alarmcontext.a7;
  p->tf->s2 =p->alarmcontext.s2;
  p->tf->s3 = p->alarmcontext.s3;
  p->tf->s4 = p->alarmcontext.s4;
  p->tf->s5 = p->alarmcontext.s5;
  p->tf->s6 = p->alarmcontext.s6;
  p->tf->s7 = p->alarmcontext.s7;
  p->tf->s8 = p->alarmcontext.s8;
  p->tf->s9 = p->alarmcontext.s9;
  p->tf->s10 = p->alarmcontext.s10;
  p->tf->s11 = p->alarmcontext.s11;
  p->tf->t3 = p->alarmcontext.t3;
  p->tf->t4 = p->alarmcontext.t4;
  p->tf->t5 = p->alarmcontext.t5;
  p->tf->t6 = p->alarmcontext.t6;

  p->handler_on = 0;

  return 0;
}