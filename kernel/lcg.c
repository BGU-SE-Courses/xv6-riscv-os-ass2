
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

static uint lcg_state = 1;

static struct spinlock lcg_lock;

void lcg_init(void)
{
    initlock(&lcg_lock, "lcg");
}

void lcg_srand(uint seed)
{
    acquire(&lcg_lock);
    lcg_state = seed;
    release(&lcg_lock);
}

uint lcg_rand(void)
{
    uint r;
    uint a = 1664525;
    uint b = 1013904223;
    uint m = 1 << 31;
    acquire(&lcg_lock);
    lcg_state = (a * lcg_state + b) % m;
    r = lcg_state;
    release(&lcg_lock);
    return r;
}