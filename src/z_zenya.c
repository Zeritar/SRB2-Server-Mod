#include "console.h"
#include "z_zenya.h"
#include "doomdef.h"
#include "g_game.h"
#include "p_local.h"
#include "d_netcmd.h"

int maptimelimits[100];
int afktimers[MAXPLAYERS];

// Setting the time limits for each map. Putting 0's in unused map slots to avoid potential null-errors.
void MapTimeLimits()
{
    for (int i = 0; i < 100; i++)
    {
        maptimelimits[i] = 0;
    }
    maptimelimits[1] = 5;      // Greenflower Zone Act 1
    maptimelimits[2] = 7;      // Greenflower Zone Act 2
    maptimelimits[3] = 5;      // Greenflower Zone Act 3
    maptimelimits[4] = 8;      // Techno Hill Zone Act 1
    maptimelimits[5] = 10;     // Techno Hill Zone Act 2
    maptimelimits[6] = 5;      // Techno Hill Zone Act 3
    maptimelimits[7] = 10;     // Deep Sea Zone Act 1
    maptimelimits[8] = 10;     // Deep Sea Zone Act 2
    maptimelimits[9] = 5;      // Deep Sea Zone Act 3
    maptimelimits[10] = 10;    // Castle Eggman Zone Act 1
    maptimelimits[11] = 10;    // Castle Eggman Zone Act 2
    maptimelimits[12] = 5;     // Castle Eggman Zone Act 3
    maptimelimits[13] = 10;    // Arid Canyon Zone Act 1
    maptimelimits[14] = 10;    // Arid Canyon Zone Act 2
    maptimelimits[15] = 5;     // Arid Canyon Zone Act 3
    maptimelimits[16] = 10;    // Red Volcano Zone
    maptimelimits[22] = 10;    // Egg Rock Zone Act 1
    maptimelimits[23] = 10;    // Egg Rock Zone Act 2
    maptimelimits[25] = 5;     // Black Core Zone Act 1
    maptimelimits[26] = 5;     // Black Core Zone Act 2
    maptimelimits[27] = 5;     // Black Core Zone Act 3
    maptimelimits[30] = 7;     // Frozen Hillside Zone
    maptimelimits[31] = 8;     // Pipe Towers Zone
    maptimelimits[32] = 8;     // Forest Fortress Zone
    maptimelimits[33] = 5;     // Techno Legacy Zone
    maptimelimits[40] = 10;    // Haunted Heights Zone
    maptimelimits[41] = 10;    // Aerial Garden Zone
    maptimelimits[42] = 10;    // Azure Temple Zone
    CONS_Printf("Initialized level time limits.\n");
}

// Checking map number to see if we need to warp to a secret level
boolean WarpCheck(INT16 curnum, INT16 nextnum)
{
    if (curnum == 27)
    {
        CONS_Printf("End of campaign. Warping to secret level...\n");
        COM_BufAddText("map 30");
        return true;
    }
    else if (curnum == 30)
    {
        COM_BufAddText("map 31");
        return true;
    }
    else if (curnum == 31)
    {
        COM_BufAddText("map 32");
        return true;
    }
    else if (curnum == 32)
    {
        COM_BufAddText("map 33");
        return true;
    }
    else if (curnum == 33)
    {
        COM_BufAddText("map 40");
        return true;
    }
    else if (curnum == 40)
    {
        COM_BufAddText("map 41");
        return true;
    }
    else if (curnum == 41)
    {
        COM_BufAddText("map 42");
        return true;
    }
    else if (curnum == 42)
    {
        CONS_Printf("End of secret levels. Warping to Greenflower Zone Act 1...\n");
        COM_BufAddText("map 01");
        return true;
    }
    return false;
}

// Checking current level time to see if we've reached the timelimit if there is one
void TimeCheck(INT16 curmap, INT16 curtime)
{
    if (maptimelimits[curmap] != 0 && curtime == (maptimelimits[curmap]*TICRATE*60))
    {
        CONS_Printf("Time limit reached. Ending the level...\n");
        COM_BufAddText("exitlevel");
    }
    // Shouldn't happen but lag could be a thing. Avoids console spam in most cases because exitlevel takes a bit to execute.
    else if (maptimelimits[curmap] != 0 && curtime > (maptimelimits[curmap]*TICRATE*60)+TICRATE)
    {
        CONS_Printf("Time limit exceeded. Ending the level...\n");
        COM_BufAddText("exitlevel");
    }

    if (curtime == (5*TICRATE) && maptimelimits[curmap])
    {
        if (mapheaderinfo[gamemap-1]->actnum)
			COM_BufAddText(va("say %s Zone Act %d time limit is %d minutes.\n", mapheaderinfo[curmap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum, maptimelimits[curmap]));
		else
			COM_BufAddText(va("say %s Zone time limit is %d minutes.\n", mapheaderinfo[curmap-1]->lvlttl, maptimelimits[curmap]));
    }
    else if (curtime == ((maptimelimits[curmap]-1)*TICRATE*60))
    {
        COM_BufAddText("say One minute left before level exit.");
    }
    /*else if (curtime == 1)
    {
        // Attempt at killing all bee hives at 5 seconds. Works, but only after clients reconnect. Looking into a way to push the update and sync clients.
        if (curmap == 41)
        {
            thinker_t *th;
            mobj_t *mo;

            for (th = thlist[THINK_MOBJ].next; th!= &thlist[THINK_MOBJ]; th = th->next)
            {
                if (th->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
                    continue;

                mo = (mobj_t *)th;

                if (mo->type != MT_HIVEELEMENTAL)
                    continue;
                
                P_KillMobj(mo, NULL, NULL, 0x80);
            }
        }
    }*/
}

// Called on map load. Must be initialized somewhere, so might as well reset it between maps.
void ResetAFKTimers()
{
    for (int i = 1; i < MAXPLAYERS; ++i)
    {
        afktimers[i] = 0;
    }
}

// maintains list of players who haven't moved in a while and warns them, then kicks them
void AFKChecker()
{
    for (int i = 1; i < MAXPLAYERS; ++i)
    {
        if (playeringame[i])
        {
            if (IsPlayerAdmin(i))
            {
                // Don't kick admins.
                continue;
            }
            else
            {
                player_t* player;
                player = &players[i];
                if (!(player->cmd.buttons || player->cmd.forwardmove || player->cmd.sidemove))
                {
                    afktimers[i]++;
                }
                else
                {
                    afktimers[i] = 0;
                }
                
            }

            // 90 seconds of AFK before getting kicked.
            int8_t afklimit = 90;
            // Player will be warned 10 seconds ahead of getting kicked.
            int8_t afkwarning = 10;

            if (afktimers[i] == (afklimit-afkwarning)*TICRATE)
            {
                COM_BufAddText(va("sayto %d If you don't move within the next %d seconds, you will be kicked.", i, afkwarning));
            }
            else if (afktimers[i] == afklimit*TICRATE)
            {
                COM_BufAddText(va("kick %d Exceeded AFK time limit.", i));
            }

        }
        else if (afktimers[i] > 0)
        {
            afktimers[i] = 0;
        }
    }
}