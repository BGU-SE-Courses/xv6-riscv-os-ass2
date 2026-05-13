#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define ISRAELI_LOCKS 15
#define QUEUE_SIZE 16

struct israeli_lock
{
    struct spinlock lock;
    uint active; // Is the lock active?
    struct proc *queue[QUEUE_SIZE];
    uint favorC; // Favoritism Coefficient (0-100)
    int queue_size;
    uint pid_holder; // if pid_holder == 0 → free, pid_holder != 0 → held by the process with that PID
};

static struct israeli_lock locks[ISRAELI_LOCKS];

void israeli_lock_init(void)
{
    for (int i = 0; i < ISRAELI_LOCKS; i++)
    {
        initlock(&locks[i].lock, "israeli");
        locks[i].active = 0;
        locks[i].pid_holder = 0;
        locks[i].favorC = 0;
        locks[i].queue_size = 0;
    }
}

int israeli_create(int favoritism)
{
    // Validate favoritism range: spec says 0–100
    if (favoritism < 0 || favoritism > 100)
    {
        return -1;
    }

    // Scan for a free slot
    for (int i = 0; i < ISRAELI_LOCKS; i++)
    {
        acquire(&locks[i].lock);

        if (locks[i].active == 0)
        {
            locks[i].active = 1;
            locks[i].pid_holder = 0;
            locks[i].favorC = favoritism;
            locks[i].queue_size = 0;

            release(&locks[i].lock);
            return i;
        }

        release(&locks[i].lock);
    }

    // No free slot
    return -1;
}

// int israeli_acquire(int lock_id);
// int israeli_release(int lock_id);
// int israeli_destroy(int lock_id);
