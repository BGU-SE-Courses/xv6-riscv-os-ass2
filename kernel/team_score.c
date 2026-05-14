#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_TEAMS 16

static struct spinlock score_lock;
static int team_scores[MAX_TEAMS];
static int race_target;
static int race_winner;

void team_score_init(void)
{
    initlock(&score_lock, "team_score");
    for (int i = 0; i < MAX_TEAMS; i++)
    {
        team_scores[i] = 0;
    }
    race_target = 0;
    race_winner = -1;
}

// Reset scores, set target, clear winner. Called by parent before race starts.
void team_race_reset(int target)
{
    acquire(&score_lock);
    for (int i = 0; i < MAX_TEAMS; i++)
    {
        team_scores[i] = 0;
    }
    race_target = target;
    race_winner = -1;
    release(&score_lock);
}

// Atomically increment team's score and return new value.
// If team reaches target and no winner yet, mark this team as winner.
// Returns -1 if team_id is invalid.
int team_score_inc(int team_id)
{
    if (team_id < 0 || team_id >= MAX_TEAMS)
    {
        return -1;
    }

    acquire(&score_lock);
    team_scores[team_id]++;
    int new_score = team_scores[team_id];
    if (race_winner < 0 && new_score >= race_target)
    {
        race_winner = team_id;
    }
    release(&score_lock);

    return new_score;
}

// Returns winning team's id, or -1 if race not over yet.
int team_race_winner(void)
{
    acquire(&score_lock);
    int w = race_winner;
    release(&score_lock);
    return w;
}