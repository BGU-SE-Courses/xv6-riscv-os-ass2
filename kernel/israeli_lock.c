
#include "types.h"
#define ISRAELI_LOCKS 15
#define QUEUE_SIZE 16

struct israeli_lock
{

    uint locked; // Is the lock held?

    uint active;            // Is the lock active?
    uint queue[QUEUE_SIZE]; // Queue of waiting processes' PIDs
    uint fc;

    uint pid_holder; // PID of the process holding the lock
};
