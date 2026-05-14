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

int israeli_acquire(int lock_id)
{
    // Validate ID range
    if (lock_id < 0 || lock_id >= ISRAELI_LOCKS)
    {
        return -1;
    }

    struct israeli_lock *il = &locks[lock_id];
    struct proc *p = myproc();

    acquire(&il->lock);

    // Slot must be active
    if (il->active == 0)
    {
        release(&il->lock);
        return -1;
    }

    // Fast path: lock is free
    if (il->pid_holder == 0)
    {
        il->pid_holder = p->pid;
        release(&il->lock);
        return 0;
    }

    // Slow path: lock is held, enqueue and sleep
    if (il->queue_size >= QUEUE_SIZE)
    {
        release(&il->lock);
        return -1;
    }

    // Enqueue at the back
    il->queue[il->queue_size] = p;
    il->queue_size++;

    // Sleep until we are chosen as the holder
    while (il->pid_holder != p->pid)
    {
        sleep(p, &il->lock);
    }

    // We are now the holder
    release(&il->lock);
    return 0;
}

// Caller must hold il->lock.
static int
next_chosen_index(struct israeli_lock *il)
{
    if ((lcg_rand() % 100) >= il->favorC)
    {
        return 0; // no favoritism this round → FIFO
    }

    int gid = myproc()->gid;
    for (int i = 0; i < il->queue_size; i++)
    {
        if (il->queue[i]->gid == gid)
        {
            return i;
        }
    }

    return 0; // no same-gid waiter → FIFO
}
int israeli_release(int lock_id)
{
    if (lock_id < 0 || lock_id >= ISRAELI_LOCKS)
    {
        return -1;
    }
    struct israeli_lock *il = &locks[lock_id];

    acquire(&il->lock);
    if (il->active == 0)
    {
        release(&il->lock);
        return -1;
    }

    // Caller must be the current holder
    if (il->pid_holder != myproc()->pid)
    {
        release(&il->lock);
        return -1;
    }
    if (il->queue_size == 0)
    {
        il->pid_holder = 0;
        release(&il->lock);
        return 0;
    }

    int winner_idx = next_chosen_index(il);
    struct proc *winner = il->queue[winner_idx]; // capture before shift
    il->pid_holder = winner->pid;

    // Remove p from the queue
    for (int i = winner_idx; i < il->queue_size - 1; i++)
    {
        il->queue[i] = il->queue[i + 1];
    }
    il->queue_size--;
    wakeup(winner);
    release(&il->lock);
    return 0;
}

int israeli_destroy(int lock_id)
{
    if (lock_id < 0 || lock_id >= ISRAELI_LOCKS)
    {
        return -1;
    }
    acquire(&locks[lock_id].lock);
    if (locks[lock_id].active == 0 || locks[lock_id].queue_size > 0 || locks[lock_id].pid_holder != 0)
    {
        release(&locks[lock_id].lock);
        return -1;
    }
    locks[lock_id].active = 0;
    locks[lock_id].favorC = 0;
    release(&locks[lock_id].lock);
    return 0;
}
