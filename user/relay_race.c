#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_TEAMS 3
#define RUNNERS_PER_TEAM 5
#define TARGET_SCORE 30
#define FAVORITISM 50

int main(void)
{
    // Initialize a new race with a clean slate and a target score
    team_race_reset(TARGET_SCORE);

    int lock_id = israeli_create(FAVORITISM);
    if (lock_id < 0)
    {
        printf("Failed to create lock\n");
        exit(1);
    }

    // Seed PRNG (pseudo-random number generator)
    lcg_srand(getpid());

    // Fork the runners
    for (int i = 0; i < NUM_TEAMS * RUNNERS_PER_TEAM; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            printf("fork failed\n");
            exit(1);
        }

        if (pid == 0)
        {
            // Child: become a runner
            int team = i % NUM_TEAMS;
            setgid(team);

            while (1)
            {
                israeli_acquire(lock_id);

                // Critical section: increment our team's score
                int new_score = team_score_inc(team);

                printf("Runner %d (Team %d) acquired the baton\n",
                       getpid(), team);
                printf("Team %d score = %d\n", team, new_score);

                israeli_release(lock_id);

                // Has any team won
                if (team_race_winner() >= 0)
                {
                    exit(0);
                }

                sleep(1); // brief pause
            }
        }
    }

    // Parent waits for all children (main thread)
    for (int i = 0; i < NUM_TEAMS * RUNNERS_PER_TEAM; i++)
    {
        wait(0);
    }

    // Report results
    int winner = team_race_winner();
    printf("\n--- Race over! Winning team: %d ---\n", winner);

    israeli_destroy(lock_id);
    exit(0);
}