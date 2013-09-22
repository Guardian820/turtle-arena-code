/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"
#include "../ui/ui_public.h"

#ifdef MISSIONPACK_HUD
#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;
#endif

#ifndef TURTLEARENA // NO_CGFORCEMODLE
int forceModelModificationCount = -1;
#endif
#ifdef MISSIONPACK
int redTeamNameModificationCount = -1;
int blueTeamNameModificationCount = -1;
#endif

void CG_Init( qboolean inGameLoad, int maxSplitView );
void CG_Ingame_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int clientNum0, int clientNum1, int clientNum2, int clientNum3 );
void CG_Shutdown( void );
void CG_Refresh( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, connstate_t state, int realTime );
static char *CG_VoIPString( int localClientNum );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case CG_GETAPIVERSION:
		return ( CG_API_MAJOR_VERSION << 16) | ( CG_API_MINOR_VERSION & 0xFFFF );
	case CG_INIT:
		CG_Init( arg0, arg1 );
		return 0;
	case CG_INGAME_INIT:
		CG_Ingame_Init( arg0, arg1, arg2, arg3, arg4, arg5, arg6 );
		return 0;
	case CG_SHUTDOWN:
		UI_Shutdown();
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand(arg0);
	case CG_REFRESH:
		CG_Refresh( arg0, arg1, arg2, arg3, arg4 );
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer(arg0);
	case CG_LAST_ATTACKER:
		return CG_LastAttacker(arg0);
	case CG_VOIP_STRING:
		return (intptr_t)CG_VoIPString(arg0);
	case CG_KEY_EVENT:
		{
			int key = arg0;
			qboolean down = arg1;

			if ( key == K_ESCAPE && down && !( trap_Key_GetCatcher( ) & KEYCATCH_UI ) ) {
				uiClientState_t cls;

				trap_GetClientState( &cls );

				if ( cls.connState == CA_ACTIVE && trap_GetDemoState() != DS_PLAYBACK ) {
					UI_SetActiveMenu( UIMENU_INGAME );
				}
				else if ( cls.connState != CA_DISCONNECTED ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
				}
				return 0;
			}
		}

		if ( cg.connected && ( trap_Key_GetCatcher( ) & KEYCATCH_CGAME ) ) {
			CG_KeyEvent(arg0, arg1);
		} else {
			UI_KeyEvent(arg0, arg1);
		}
		return 0;
	case CG_MOUSE_EVENT:
		if ( cg.connected && ( trap_Key_GetCatcher( ) & KEYCATCH_CGAME ) ) {
			CG_MouseEvent(arg0, arg1, arg2);
		} else {
			UI_MouseEvent(arg0, arg1, arg2);
		}
		return 0;
	case CG_MOUSE_POSITION:
		return UI_MousePosition( arg0 );
	case CG_SET_MOUSE_POSITION:
		UI_SetMousePosition( arg0, arg1, arg2 );
		return 0;
	case CG_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		return 0;
	case CG_JOYSTICK_EVENT:
		CG_JoystickEvent(arg0, arg1, arg2);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	case CG_CONSOLE_TEXT:
		CG_AddNotifyText();
		return 0;
	case CG_WANTSBINDKEYS:
		return UI_WantsBindKeys();
	case CG_CREATE_USER_CMD:
		return (intptr_t)CG_CreateUserCmd(arg0, arg1, arg2, IntAsFloat(arg3), IntAsFloat(arg4));
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
#ifdef TA_WEAPSYS
projectileInfo_t	cg_projectiles[MAX_BG_PROJ];
weaponInfo_t		cg_weapons[MAX_BG_WEAPONS];
weaponGroupInfo_t	cg_weapongroups[MAX_BG_WEAPON_GROUPS];
#else
weaponInfo_t		cg_weapons[MAX_WEAPONS];
#endif
itemInfo_t			cg_items[MAX_ITEMS];
#ifdef TA_NPCSYS
npcInfo_t			cg_npcs[MAX_NPCS];
#endif

vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
#ifndef NOTRATEDM // No gibs.
vmCvar_t	cg_gibs;
#endif
#ifdef IOQ3ZTM // DRAW_SPEED
vmCvar_t	cg_drawSpeed;
#endif
vmCvar_t	cg_drawTimer;
#ifdef TA_MISC // COMIC_ANNOUNCER
vmCvar_t	cg_announcerText;
vmCvar_t	cg_announcerVoice;
#endif
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
#ifndef TURTLEARENA // NO_AMMO_WARNINGS
vmCvar_t	cg_drawAmmoWarning;
#endif
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
#ifndef IOQ3ZTM // LERP_FRAME_CLIENT_LESS
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
#endif
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
#ifndef TURTLEARENA // NOZOOM
vmCvar_t	cg_zoomFov;
#endif
vmCvar_t	cg_splitviewVertical;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_stats;
vmCvar_t 	cg_buildScript;
#ifndef TURTLEARENA // NO_CGFORCEMODLE
vmCvar_t 	cg_forceModel;
#endif
vmCvar_t	cg_paused;
#ifndef NOBLOOD
vmCvar_t	cg_blood;
#endif
vmCvar_t	cg_predictItems;
vmCvar_t	cg_deferPlayers;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
#ifdef MISSIONPACK
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
#endif
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_smoothClients;
vmCvar_t	pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
#ifndef IOQ3ZTM // NEW_CAM
vmCvar_t	cg_cameraOrbitDelay;
#endif
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
vmCvar_t	cg_oldPlasma;
vmCvar_t	cg_trueLightning;
vmCvar_t	cg_atmosphericEffects;
vmCvar_t	cg_teamDmLeadAnnouncements;
vmCvar_t	cg_voipShowMeter;
vmCvar_t	cg_voipShowCrosshairMeter;
vmCvar_t	cg_consoleLatency;
vmCvar_t	cg_drawShaderInfo;
vmCvar_t	cg_coronafardist;
vmCvar_t	cg_coronas;
vmCvar_t	cg_fovAspectAdjust;
vmCvar_t	cg_fadeExplosions;
vmCvar_t	ui_stretch;

#if !defined MISSIONPACK && defined IOQ3ZTM // Support MissionPack players.
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
#endif
#ifdef MISSIONPACK
vmCvar_t 	cg_redTeamName;
vmCvar_t 	cg_blueTeamName;
#ifndef IOQ3ZTM
vmCvar_t	cg_singlePlayer;
#endif
vmCvar_t	cg_enableDust;
vmCvar_t	cg_enableBreath;
vmCvar_t	cg_singlePlayerActive;
vmCvar_t	cg_recordSPDemo;
vmCvar_t	cg_recordSPDemoName;
vmCvar_t	cg_obeliskRespawnDelay;
#endif
#ifdef TA_WEAPSYS // MELEE_TRAIL
vmCvar_t	cg_drawMeleeWeaponTrails;
#endif
#ifdef TA_MISC // MATERIALS 
vmCvar_t	cg_impactDebris;
#endif
#ifdef IOQ3ZTM // LASERTAG
vmCvar_t	cg_laserTag;
#endif
#ifdef TA_PATHSYS // 2DMODE
vmCvar_t	cg_2dmode;
vmCvar_t	cg_2dmodeOverride;
#endif

vmCvar_t	cg_color1[MAX_SPLITVIEW];
vmCvar_t	cg_color2[MAX_SPLITVIEW];
vmCvar_t	cg_handicap[MAX_SPLITVIEW];
vmCvar_t	cg_teamtask[MAX_SPLITVIEW];
vmCvar_t	cg_teampref[MAX_SPLITVIEW];
#ifndef TA_WEAPSYS_EX
vmCvar_t	cg_autoswitch[MAX_SPLITVIEW];
#endif
vmCvar_t	cg_drawGun[MAX_SPLITVIEW];
vmCvar_t	cg_thirdPerson[MAX_SPLITVIEW];
vmCvar_t	cg_thirdPersonRange[MAX_SPLITVIEW];
vmCvar_t	cg_thirdPersonAngle[MAX_SPLITVIEW];
#ifdef IOQ3ZTM // ANALOG
vmCvar_t	cg_thirdPersonAnalog[MAX_SPLITVIEW];
#endif

#ifdef MISSIONPACK
vmCvar_t	cg_currentSelectedPlayer[MAX_SPLITVIEW];
vmCvar_t	cg_currentSelectedPlayerName[MAX_SPLITVIEW];
#endif

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	float		rangeMin;
	float		rangeMax;
	qboolean	rangeIntegral;
} cvarTable_t;

typedef struct {
	vmCvar_t	*vmCvars; // [MAX_SPLITVIEW]
	char		*baseName;
	char		*defaultString;
	int			baseCvarFlags;
	float		rangeMin;
	float		rangeMax;
	qboolean	rangeIntegral;
} userCvarTable_t;

#define RANGE_ALL 0, 0, qfalse
#define RANGE_BOOL 0, 1, qtrue
#define RANGE_INT(min,max) min, max, qtrue
#define RANGE_FLOAT(min,max) min, max, qfalse

static cvarTable_t cgameCvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0, RANGE_ALL },	// used for debugging
#ifndef TURTLEARENA // NOZOOM
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE, RANGE_FLOAT(1, 160) },
#endif
#ifdef TURTLEARENA // FOV
	{ &cg_fov, "cg_fov", "70", CVAR_ARCHIVE, RANGE_FLOAT(1, 160) },
#else
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE, RANGE_FLOAT(1, 160) },
#endif
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE, RANGE_INT( 30, 100 ) },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE, RANGE_ALL  },
#ifndef NOTRATEDM // No gibs.
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef IOQ3ZTM // DRAW_SPEED
	{ &cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE, RANGE_BOOL },
#endif
#ifdef TA_MISC // Show the time on the HUD.
	{ &cg_drawTimer, "cg_drawTimer", "1", CVAR_ARCHIVE, RANGE_BOOL },
#else
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE, RANGE_BOOL },
#endif
#ifdef TA_MISC // COMIC_ANNOUNCER
	{ &cg_announcerText, "cg_announcerText", "0", 0  },
	{ &cg_announcerVoice, "cg_announcerVoice", "1", 0  },
#endif
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
#ifndef TURTLEARENA // NO_AMMO_WARNINGS
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef TA_DATA
	{ &cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE, RANGE_ALL },
#else
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE, RANGE_ALL },
#endif
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE, RANGE_ALL },
#ifdef TA_DATA // Data actually support it now, so default to off.
	{ &cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE, RANGE_BOOL },
#else
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT, RANGE_ALL },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT, RANGE_ALL },
	{ &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE, RANGE_ALL  },
	{ &cg_bobup , "cg_bobup", "0.005", CVAR_CHEAT, RANGE_ALL },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT, RANGE_ALL },
#ifndef IOQ3ZTM // LERP_FRAME_CLIENT_LESS
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT, RANGE_BOOL },
#endif
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_errorDecay, "cg_errordecay", "100", 0, RANGE_ALL },
	{ &cg_nopredict, "cg_nopredict", "0", 0, RANGE_BOOL },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_showmiss, "cg_showmiss", "0", 0, RANGE_BOOL },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT, RANGE_ALL },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT, RANGE_ALL },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT, RANGE_ALL },
	{ &cg_splitviewVertical, "cg_splitviewVertical", "0", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef IOQ3ZTM // TEAM_CHAT_CON // con_notifytime
	{ &cg_teamChatTime, "cg_teamChatTime", "5", CVAR_ARCHIVE, RANGE_ALL },
#else
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE, RANGE_ALL },
#endif
#ifdef TA_MISC // TEAM_CHAT_CON
	{ &cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE, RANGE_INT( 0, TEAMCHAT_HEIGHT ) },
#else
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE, RANGE_INT( 0, TEAMCHAT_HEIGHT ) },
#endif
#ifndef TURTLEARENA // NO_CGFORCEMODLE
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE, RANGE_BOOL },
#if defined MISSIONPACK || defined IOQ3ZTM
	{ &cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE, RANGE_BOOL },
#else
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
#ifdef TURTLEARENA // ZTM: Draw team overlay info at the top right of the screen.
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "1", CVAR_ARCHIVE, RANGE_INT(0, 3) },
#else
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE, RANGE_INT(0, 3) },
#endif
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO_ALL, RANGE_ALL },
	{ &cg_stats, "cg_stats", "0", 0, RANGE_ALL },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef MISSIONPACK
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0, RANGE_ALL },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM, RANGE_ALL },
#ifndef NOBLOOD
#ifdef NOTRATEDM // ZTM: Default to no blood.
	{ &cg_blood, "com_blood", "0", CVAR_ARCHIVE, RANGE_ALL },
#else
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE, RANGE_ALL },
#endif
#endif
	{ NULL,  "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, RANGE_INT(0, GT_MAX_GAME_TYPE-1) },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
#if !defined MISSIONPACK && defined IOQ3ZTM // Support MissionPack players.
	{ &cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, RANGE_ALL },
	{ &cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SYSTEMINFO, RANGE_ALL },
#endif
#ifdef MISSIONPACK
#ifndef IOQ3ZTM
	{ &cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_SYSTEMINFO | CVAR_ROM, RANGE_ALL },
#endif
	{ &cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO, RANGE_BOOL },
	{ &cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO, RANGE_BOOL },
	{ &cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_SYSTEMINFO | CVAR_ROM, RANGE_ALL },
	{ &cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO, RANGE_ALL },
#ifdef MISSIONPACK_HUD
	{ &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE, RANGE_ALL },
#endif
#endif
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT, RANGE_ALL },
#ifndef IOQ3ZTM // NEW_CAM
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE, RANGE_ALL },
#endif
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0, RANGE_ALL },
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0, RANGE_ALL },
	{ &cg_timescale, "timescale", "1", 0, RANGE_ALL },
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT, RANGE_ALL },

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, RANGE_ALL },
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef TURTLEARENA // WEAPONS
	{ &cg_oldPlasma, "cg_oldPlasma", "0", CVAR_ARCHIVE, RANGE_BOOL },
#else
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_atmosphericEffects, "cg_atmosphericEffects", "1", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_teamDmLeadAnnouncements, "cg_teamDmLeadAnnouncements", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_voipShowMeter, "cg_voipShowMeter", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_voipShowCrosshairMeter, "cg_voipShowCrosshairMeter", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_consoleLatency, "cg_consoleLatency", "3000", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_drawShaderInfo, "cg_drawShaderInfo", "0", 0, RANGE_BOOL },
	{ &cg_coronafardist, "cg_coronafardist", "1536", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_coronas, "cg_coronas", "1", CVAR_ARCHIVE, RANGE_INT( 0, 3 ) },
	{ &cg_fovAspectAdjust, "cg_fovAspectAdjust", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_fadeExplosions, "cg_fadeExplosions", "0", CVAR_ARCHIVE, RANGE_BOOL },
#ifdef TA_WEAPSYS // MELEE_TRAIL
	{ &cg_drawMeleeWeaponTrails, "cg_drawMeleeWeaponTrails", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
#ifdef TA_MISC // MATERIALS
	{ &cg_impactDebris, "cg_impactDebris", "1", CVAR_ARCHIVE, RANGE_FLOAT( 0, 10 ) },
#endif
#ifdef IOQ3ZTM // LASERTAG
	{ &cg_laserTag, "g_laserTag", "0", CVAR_SERVERINFO, RANGE_BOOL },
#endif
#ifdef TA_PATHSYS // 2DMODE
	{ &cg_2dmode, "g_2dmode", "0", CVAR_SERVERINFO, RANGE_ALL },
	{ &cg_2dmodeOverride, "cg_2dmodeOverride", "0", 0, RANGE_ALL },
#endif
//	{ &cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL }

	{ &ui_stretch, "ui_stretch", "0", CVAR_ARCHIVE, RANGE_BOOL },
};

static userCvarTable_t userCvarTable[] = {
	{ cg_color1, "color1", XSTRING( DEFAULT_CLIENT_COLOR1 ), CVAR_USERINFO | CVAR_ARCHIVE, RANGE_ALL },
	{ cg_color2, "color2", XSTRING( DEFAULT_CLIENT_COLOR2 ), CVAR_USERINFO | CVAR_ARCHIVE, RANGE_ALL },
	{ cg_handicap, "handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_ALL },
	{ cg_teamtask, "teamtask", "0", CVAR_USERINFO, RANGE_ALL },
	{ cg_teampref, "teampref", "", CVAR_USERINFO, RANGE_ALL },

#ifndef TA_WEAPSYS_EX
	{ cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE, RANGE_BOOL },
#endif
#ifdef TURTLEARENA
	{ cg_drawGun, "cg_drawViewWeapons", "1", CVAR_ARCHIVE, RANGE_BOOL },
#else
	{ cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE, RANGE_INT(0, 3) },
#endif
#ifdef TURTLEARENA // FOV // THIRD_PERSON
	{ cg_thirdPerson, "cg_thirdPerson", "1", 0, RANGE_BOOL },
	{ cg_thirdPersonRange, "cg_thirdPersonRange", "120", 0, RANGE_ALL },
	{ cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", 0, RANGE_ALL },
#else
	{ cg_thirdPerson, "cg_thirdPerson", "0", 0, RANGE_BOOL },
	{ cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT, RANGE_ALL },
	{ cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT, RANGE_ALL },
#endif
#ifdef IOQ3ZTM // ANALOG
	{ cg_thirdPersonAnalog, "cg_thirdPersonAnalog", "0", 0, RANGE_BOOL },
#endif

#ifdef MISSIONPACK
	{ cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE, RANGE_ALL },
	{ cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE, RANGE_ALL }
#endif
};

static int  cgameCvarTableSize = ARRAY_LEN( cgameCvarTable );
static int  userCvarTableSize = ARRAY_LEN( userCvarTable );

/*
=================
CG_RegisterCvar
=================
*/
void CG_RegisterCvar( vmCvar_t *vmCvar, char *cvarName, char *defaultString, int cvarFlags, float rangeMin, float rangeMax, qboolean rangeIntegral ) {
	trap_Cvar_Register( vmCvar, cvarName, defaultString, cvarFlags );

	if ( rangeMin != 0 || rangeMax != 0 ) {
		trap_Cvar_CheckRange( cvarName, rangeMin, rangeMax, rangeIntegral );
	}
}

/*
=================
CG_RegisterCgameCvars
=================
*/
void CG_RegisterCgameCvars( void ) {
	cvarTable_t	*cv;
	int			i;

	for ( i = 0, cv = cgameCvarTable ; i < cgameCvarTableSize ; i++, cv++ ) {
		CG_RegisterCvar( cv->vmCvar, cv->cvarName, cv->defaultString,
						cv->cvarFlags, cv->rangeMin, cv->rangeMax, cv->rangeIntegral );
	}
}

/*
=================
CG_RegisterUserCvars
=================
*/
void CG_RegisterUserCvars( void ) {
	int				userInfo[MAX_SPLITVIEW] = { CVAR_USERINFO, CVAR_USERINFO2, CVAR_USERINFO3, CVAR_USERINFO4 };
	char			*modelNames[MAX_SPLITVIEW] = { DEFAULT_MODEL, DEFAULT_MODEL2, DEFAULT_MODEL3, DEFAULT_MODEL4 };
#ifdef IOQ3ZTM // BLANK_HEADMODEL
	char			*headModelNames[MAX_SPLITVIEW] = { "", "", "", "" };
#else
	char			*headModelNames[MAX_SPLITVIEW] = { DEFAULT_HEAD, DEFAULT_HEAD2, DEFAULT_HEAD3, DEFAULT_HEAD4 };
#endif
#ifndef IOQ3ZTM_NO_TEAM_MODEL
	char			*teamModelNames[MAX_SPLITVIEW] = { DEFAULT_TEAM_MODEL, DEFAULT_TEAM_MODEL2, DEFAULT_TEAM_MODEL3, DEFAULT_TEAM_MODEL4 };
	char			*teamHeadModelNames[MAX_SPLITVIEW] = { DEFAULT_TEAM_HEAD, DEFAULT_TEAM_HEAD2, DEFAULT_TEAM_HEAD3, DEFAULT_TEAM_HEAD4 };
#endif
	char			*name;
	userCvarTable_t	*uservar;
	vmCvar_t		*vmcvar;
	int				i, j;
	int				cvarFlags;

	for ( i = 0, uservar = userCvarTable ; i < userCvarTableSize ; i++, uservar++ ) {
		for ( j = 0; j < CG_MaxSplitView(); j++ ) {
			if ( uservar->vmCvars ) {
				vmcvar = &uservar->vmCvars[j];
			} else {
				vmcvar = NULL;
			}

			cvarFlags = uservar->baseCvarFlags;

			// set correct userinfo flag
			if ( cvarFlags & CVAR_USERINFO ) {
				cvarFlags &= ~CVAR_USERINFO;
				cvarFlags |= userInfo[j];
			}

			CG_RegisterCvar( vmcvar, Com_LocalClientCvarName( j, uservar->baseName ),
							uservar->defaultString, cvarFlags,
							uservar->rangeMin, uservar->rangeMax, uservar->rangeIntegral );
		}
	}

	// cvars with per-player defaults
	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( i == 0 ) {
			name = DEFAULT_CLIENT_NAME;
		} else {
			name = va("%s%d", DEFAULT_CLIENT_NAME, i + 1);
		}

		trap_Cvar_Register( NULL, Com_LocalClientCvarName(i, "name"), name, userInfo[i] | CVAR_ARCHIVE );

		trap_Cvar_Register( NULL, Com_LocalClientCvarName(i, "model"), modelNames[i], userInfo[i] | CVAR_ARCHIVE );
		trap_Cvar_Register( NULL, Com_LocalClientCvarName(i, "headmodel"), headModelNames[i], userInfo[i] | CVAR_ARCHIVE );

#ifndef IOQ3ZTM_NO_TEAM_MODEL
		trap_Cvar_Register( NULL, Com_LocalClientCvarName(i, "team_model"), teamModelNames[i], userInfo[i] | CVAR_ARCHIVE );
		trap_Cvar_Register( NULL, Com_LocalClientCvarName(i, "team_headmodel"), teamHeadModelNames[i], userInfo[i] | CVAR_ARCHIVE );
#endif

		// ZTM: TODO: Move this somewhere else so one can set teampref on CLI startup args?
		// clear team preference if was previously set (only want it used for one game)
		trap_Cvar_Set( Com_LocalClientCvarName(i, "teampref"), "" );
	}
}

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	char		var[MAX_TOKEN_CHARS];

	CG_RegisterCgameCvars();
	CG_RegisterUserCvars();
	CG_RegisterInputCvars();

	// ZTM: TODO: Move cgs.localServer init somewhere else?
	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

#ifndef TURTLEARENA // NO_CGFORCEMODLE
	forceModelModificationCount = cg_forceModel.modificationCount;
#endif
#ifdef MISSIONPACK
	redTeamNameModificationCount = cg_redTeamName.modificationCount;
	blueTeamNameModificationCount = cg_blueTeamName.modificationCount;
#endif
}

#if defined MISSIONPACK || !defined TURTLEARENA // NO_CGFORCEMODLE
/*
===================
CG_ForceModelChange
===================
*/
static void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
}
#endif

/*
=================
CG_UpdateCgameCvars
=================
*/
void CG_UpdateCgameCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cgameCvarTable ; i < cgameCvarTableSize ; i++, cv++ ) {
		if ( !cv->vmCvar ) {
			continue;
		}

		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
=================
CG_UpdateUserCvars
=================
*/
void CG_UpdateUserCvars( void ) {
	int				i, j;
	userCvarTable_t	*uservar;

	for ( i = 0, uservar = userCvarTable ; i < userCvarTableSize ; i++, uservar++ ) {
		if ( !uservar->vmCvars ) {
			continue;
		}

		for ( j = 0; j < CG_MaxSplitView(); j++ ) {
			trap_Cvar_Update( &uservar->vmCvars[j] );
		}
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	CG_UpdateCgameCvars();
	CG_UpdateUserCvars();
	CG_UpdateInputCvars();

	if ( !cg.connected ) {
		return;
	}

	// check for modications here

	// If team overlay is on, ask for updates from the server.  If it's off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_SetValue( "teamoverlay", 1 );
		} else {
			trap_Cvar_SetValue( "teamoverlay", 0 );
		}
	}


#ifdef MISSIONPACK
	// if force model or a team name changed
	if (
#ifndef TURTLEARENA // NO_CGFORCEMODLE
	forceModelModificationCount != cg_forceModel.modificationCount ||
#endif
		redTeamNameModificationCount != cg_redTeamName.modificationCount
		|| blueTeamNameModificationCount != cg_blueTeamName.modificationCount )
	{
#ifndef TURTLEARENA // NO_CGFORCEMODLE
		forceModelModificationCount = cg_forceModel.modificationCount;
#endif
		redTeamNameModificationCount = cg_redTeamName.modificationCount;
		blueTeamNameModificationCount = cg_blueTeamName.modificationCount;
		CG_ForceModelChange();
	}
#else
#ifndef TURTLEARENA // NO_CGFORCEMODLE
	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount )
	{
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
#endif
#endif
}

int CG_CrosshairPlayer( int localClientNum ) {
	if (!cg.snap || localClientNum < 0 || localClientNum >= CG_MaxSplitView()) {
		return -1;
	}

	if ( cg.time > ( cg.localClients[localClientNum].crosshairClientTime + 1000 ) ) {
		return -1;
	}

	return cg.localClients[localClientNum].crosshairClientNum;
}

int CG_LastAttacker( int localClientNum ) {
	if (!cg.snap || localClientNum < 0 || localClientNum >= CG_MaxSplitView()) {
		return -1;
	}

	if ( !cg.localClients[localClientNum].attackerTime ) {
		return -1;
	}

	return cg.snap->pss[cg.snap->lcIndex[localClientNum]].persistant[PERS_ATTACKER];
}

/*
=================
CG_RemoveNotifyLine
=================
*/
void CG_RemoveNotifyLine( cglc_t *localClient )
{
	int i, offset, totalLength;

	if( !localClient || localClient->numConsoleLines == 0 )
		return;

	offset = localClient->consoleLines[ 0 ].length;
	totalLength = strlen( localClient->consoleText ) - offset;

	// slide up consoleText
	for ( i = 0; i <= totalLength; i++ )
		localClient->consoleText[ i ] = localClient->consoleText[ i + offset ];

	// pop up the first consoleLine
	for ( i = 1; i < localClient->numConsoleLines; i++ )
		localClient->consoleLines[ i - 1 ] = localClient->consoleLines[ i ];

	// clear last slot
	localClient->consoleLines[ localClient->numConsoleLines - 1 ].length = 0;
	localClient->consoleLines[ localClient->numConsoleLines - 1 ].time = 0;

	localClient->numConsoleLines--;
}

/*
=================
CG_AddNotifyText
=================
*/
void CG_AddNotifyText( void ) {
	char text[ MAX_CONSOLE_TEXT - 1 ];
	char *buffer;
	int bufferLen;
	int lc;
	cglc_t *localClient;
	int localClientBits;

	trap_LiteralArgs( text, sizeof ( text ) );

	if( !text[ 0 ] ) {
		for ( lc = 0; lc < CG_MaxSplitView(); lc++ ) {
			cg.localClients[lc].consoleText[ 0 ] = '\0';
			cg.localClients[lc].numConsoleLines = 0;
		}
		return;
	}

	buffer = text;
	bufferLen = strlen( buffer );

	// [player #] perfix for text that only shows up in notify area for one local client
	if ( bufferLen > 4 && !Q_strncmp( buffer, "[player ", 8 ) && isdigit(buffer[8]) && buffer[9] == ']' ) {
		localClientBits = 1 << ( atoi( &buffer[8] ) - 1 );

		buffer += 10;
		bufferLen = strlen( buffer );
	} else {
		localClientBits = ~0;
	}

	for ( lc = 0; lc < CG_MaxSplitView(); lc++ ) {
		if ( !( localClientBits & ( 1 << lc ) ) ) {
			continue;
		}

		localClient = &cg.localClients[lc];

		if( localClient->numConsoleLines == MAX_CONSOLE_LINES )
			CG_RemoveNotifyLine( localClient );

		// free lines until there is enough space to fit buffer
		while ( strlen( localClient->consoleText ) + bufferLen > MAX_CONSOLE_TEXT ) {
			CG_RemoveNotifyLine( localClient );
		}

		Q_strcat( localClient->consoleText, MAX_CONSOLE_TEXT, buffer );
		localClient->consoleLines[ localClient->numConsoleLines ].time = cg.time;
		localClient->consoleLines[ localClient->numConsoleLines ].length = bufferLen;
		localClient->numConsoleLines++;
	}
}

/*
=================
CG_NotifyPrintf

Only printed in notify area for localClientNum (and client console)
=================
*/
void QDECL CG_NotifyPrintf( int localClientNum, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	int			prefixLen;

	Com_sprintf( text, sizeof(text), "[player %d]", localClientNum + 1 );
	prefixLen = strlen(text);

	va_start (argptr, msg);
	Q_vsnprintf (text+prefixLen, sizeof(text)-prefixLen, msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

/*
=================
CG_NotifyBitsPrintf

Only printed in notify area for players specified in localClientBits (and client console)
=================
*/
void QDECL CG_NotifyBitsPrintf( int localClientBits, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	int i;

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( localClientBits & ( 1 << i ) ) {
			CG_NotifyPrintf( i, "%s", text );
		}
	}
}

void QDECL CG_DPrintf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	char		var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL Com_DPrintf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_DPrintf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_Cvar_VariableString
================
*/
char *CG_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_MaxSplitView
================
*/
int CG_MaxSplitView(void) {
	return cgs.maxSplitView;
}

//========================================================================

/*
=================
CG_SetupDlightstyles
=================
*/
void CG_SetupDlightstyles( void ) {
	int i, j;
	char        *str;
	char        *token;
	int entnum;
	centity_t   *cent;

	cg.lightstylesInited = qtrue;

	for ( i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; i++ ) {
		str = (char *) CG_ConfigString( CS_DLIGHTS + i );
		if ( !strlen( str ) ) {
			break;
		}

		token = COM_Parse( &str );   // ent num
		entnum = atoi( token );

		if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
			continue;
		}

		cent = &cg_entities[entnum];

		token = COM_Parse( &str );   // stylestring
		Q_strncpyz( cent->dl_stylestring, token, sizeof( cent->dl_stylestring ) );

		token = COM_Parse( &str );   // offset
		cent->dl_frame      = atoi( token );
		cent->dl_oldframe   = cent->dl_frame - 1;
		if ( cent->dl_oldframe < 0 ) {
			cent->dl_oldframe = strlen( cent->dl_stylestring );
		}

		token = COM_Parse( &str );   // sound id
		cent->dl_sound = atoi( token );

		token = COM_Parse( &str );   // attenuation
		cent->dl_atten = atoi( token );

		for ( j = 0; j < strlen( cent->dl_stylestring ); j++ ) {

			cent->dl_stylestring[j] += cent->dl_atten;  // adjust character for attenuation/amplification

			// clamp result
			if ( cent->dl_stylestring[j] < 'a' ) {
				cent->dl_stylestring[j] = 'a';
			}
			if ( cent->dl_stylestring[j] > 'z' ) {
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp   = 0.0;
		cent->dl_time       = cg.time;
	}
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	bg_iteminfo_t	*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = BG_ItemForItemNum(itemNum);

	if( item->pickup_sound[0] ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue );
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue );
#endif

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {

#ifndef TA_DATA
		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
#endif
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/feedback/redleads.wav", qtrue );
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/feedback/blueleads.wav", qtrue );
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue );
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue );

#ifdef TA_DATA
		cgs.media.captureFlagSound = cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture.wav", qtrue );
		cgs.media.returnFlagSound = trap_S_RegisterSound( "sound/teamplay/flagreturn.wav", qtrue );
#else
		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );
#endif

		if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue );
#ifdef TA_DATA
			cgs.media.redTeamTookBlueFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_red_flag.wav", qtrue );
			cgs.media.blueTeamTookRedFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_flag.wav", qtrue );
#else
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );
#endif
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
#ifdef TA_DATA
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_neutral_returned.wav", qtrue );
			cgs.media.redTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_red_1flag.wav", qtrue );
			cgs.media.blueTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_1flag.wav", qtrue );
#else
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );
#endif
		}

		if ( cgs.gametype == GT_1FCTF || cgs.gametype == GT_CTF || cg_buildScript.integer ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
#ifdef TA_DATA
			for ( i = 0; i < CG_MaxSplitView(); i++ ) {
				cgs.media.playerHasFlagSound[i] = trap_S_RegisterSound( va( "sound/teamplay/voc_player%d_flag.wav", i+1 ), qtrue );
			}
#endif
#ifndef NOTRATEDM // Disable strong lang.
			cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
#endif
		}

		if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
#ifdef TA_DATA
			cgs.media.redBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_red_base_attack.wav", qtrue );
			cgs.media.blueBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_base_attack.wav", qtrue );
#else
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( "sound/teamplay/voc_base_attack.wav", qtrue );
#endif
		}
#else
		cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
#ifdef TA_DATA
		for ( i = 0; i < CG_MaxSplitView(); i++ ) {
			cgs.media.playerHasFlagSound[i] = trap_S_RegisterSound( va( "sound/teamplay/voc_player%d_flag.wav", i+1 ), qtrue );
		}
#endif
#ifndef NOTRATEDM // Disable strong lang.
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
#endif
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
#ifndef NOTRATEDM // No gibs.
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );
#endif

#ifdef MISSIONPACK
#ifndef TURTLEARENA // POWERS
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );
#endif
	cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "sound/items/obelisk_hit_01.wav", qfalse );
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "sound/items/obelisk_hit_02.wav", qfalse );
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "sound/items/obelisk_hit_03.wav", qfalse );
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "sound/items/obelisk_respawn.wav", qfalse );

#ifndef TURTLEARENA // POWERS
	cgs.media.ammoregenSound = trap_S_RegisterSound("sound/items/cl_ammoregen.wav", qfalse);
	cgs.media.doublerSound = trap_S_RegisterSound("sound/items/cl_doubler.wav", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("sound/items/cl_guard.wav", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("sound/items/cl_scout.wav", qfalse);
#endif
#endif

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
#ifndef IOQ3ZTM // MORE_PLAYER_SOUNDS
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);
#endif

#ifdef IOQ3ZTM // LETTERBOX
	cgs.media.letterBoxOnSound = trap_S_RegisterSound( "sound/misc/letterboxon.wav", qfalse );
	cgs.media.letterBoxOffSound = trap_S_RegisterSound( "sound/misc/letterboxoff.wav", qfalse );
#endif

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse );
#if defined MISSIONPACK && !defined TURTLEARENA // NOARMOR
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

#ifndef TURTLEARENA // AWARDS
	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
#endif
	cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue );
#ifdef MISSIONPACK
#ifndef TURTLEARENA // AWARDS
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );
#endif
#endif

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue);

#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue);
#endif

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
	}

#ifdef TA_MISC // MATERIALS
	// Load sounds for materials
	Com_sprintf (name, sizeof(name), "sound/weapons/rocket/rocklf1a.wav");
	cgs.media.matExplode[MT_NONE] = trap_S_RegisterSound(name, qfalse);

	for (i = 1; i < NUM_MATERIAL_TYPES; i++)
	{
		Com_sprintf (name, sizeof(name), "sound/materials/%s_explode.wav", materialInfo[i].name);
		cgs.media.matExplode[i] = trap_S_RegisterSound(name, qfalse);
		if (!cgs.media.matExplode[i]) {
			cgs.media.matExplode[i] = cgs.media.matExplode[MT_NONE];
		}
	}
#endif

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < BG_NumItems() ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
#ifdef TURTLEARENA // HOLDABLE
	cgs.media.shurikenSound = trap_S_RegisterSound ("sound/items/use_shuriken.wav", qfalse);
#endif
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
#ifndef TA_WEAPSYS
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	//cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);
#endif
#ifdef MISSIONPACK
#ifndef TURTLEARENA // WEAPONS
	cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
#endif
	cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
#endif
#ifndef TA_SP
	cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );
#endif

#ifndef TA_WEAPSYS
	cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);
#endif
#endif

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
#ifndef TURTLEARENA // POWERS
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
#endif
#ifndef TA_WEAPSYS
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);
#endif

#if defined MISSIONPACK && !defined TA_DATA // Don't percache sounds we don't use.
	trap_S_RegisterSound("sound/player/james/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/james/taunt.wav", qfalse );

	trap_S_RegisterSound("sound/player/janet/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/janet/taunt.wav", qfalse );
#endif

}


//===================================================================================


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
#ifdef TA_MISC // MATERIALS
	int			j;
#endif
	char		items[MAX_ITEMS+1];
#ifdef TA_NPCSYS
	char		npcs[MAX_NPCS+1];
#endif
#ifdef TA_MISC // MATERIALS
	char		name[MAX_QPATH];
#endif
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	CG_LoadingString( "entities" );

	CG_ParseEntitiesFromString();

	// precache status bar pics
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

#ifdef TURTLEARENA
	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.png" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.png" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.png" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.png" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.png" );
#else
	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );
#endif

#ifndef NOBLOOD
	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );
#endif

#ifdef TURTLEARENA
	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer" );

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time" );
#else
	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );
#endif

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
#ifndef TA_WEAPSYS
#ifdef MISSIONPACK
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail" );
	cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
#endif
	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
#endif
#ifndef NOTRATEDM // No gibs.
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
#endif
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
#ifdef TURTLEARENA // POWERS
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/red" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/yellow" );
	cgs.media.playerTeleportShader = trap_R_RegisterShader("powerups/blue" );
#else
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
#endif
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
#ifndef TURTLEARENA // POWERS
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
#endif
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

#ifdef MISSIONPACK_HARVESTER
	if ( cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
		cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	}

 	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
#elif defined MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
#else
	if ( cgs.gametype == GT_CTF || cg_buildScript.integer ) {
#endif
#ifndef TA_DATA // FLAG_MODEL
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
#endif
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );
#ifdef TA_DATA
		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
#endif
#ifdef MISSIONPACK
#ifndef TA_DATA // FLAG_MODEL
		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );
#endif

#ifndef TA_DATA
		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
#endif
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
#endif
#ifdef IOQ3ZTM // FLAG_ANIMATIONS
		//
		// Setup flag animations.
		//
		// flag moving fast
		cgs.media.flag_animations[FLAG_RUN].firstFrame = 0;
		cgs.media.flag_animations[FLAG_RUN].numFrames = 16;
		cgs.media.flag_animations[FLAG_RUN].loopFrames = 16;
		cgs.media.flag_animations[FLAG_RUN].frameLerp = 1000 / 15;
		cgs.media.flag_animations[FLAG_RUN].initialLerp = 1000 / 15;
		cgs.media.flag_animations[FLAG_RUN].reversed = qfalse;
		// flag not moving or moving slowly
		cgs.media.flag_animations[FLAG_STAND].firstFrame = 16;
		cgs.media.flag_animations[FLAG_STAND].numFrames = 5;
		cgs.media.flag_animations[FLAG_STAND].loopFrames = 0;
		cgs.media.flag_animations[FLAG_STAND].frameLerp = 1000 / 20;
		cgs.media.flag_animations[FLAG_STAND].initialLerp = 1000 / 20;
		cgs.media.flag_animations[FLAG_STAND].reversed = qfalse;
		// flag speeding up
		cgs.media.flag_animations[FLAG_STAND2RUN].firstFrame = 16;
		cgs.media.flag_animations[FLAG_STAND2RUN].numFrames = 5;
		cgs.media.flag_animations[FLAG_STAND2RUN].loopFrames = 1;
		cgs.media.flag_animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
		cgs.media.flag_animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
		cgs.media.flag_animations[FLAG_STAND2RUN].reversed = qtrue;
		//FLAG_RUNUP
		//FLAG_RUNDOWN
#endif
	}

#ifdef MISSIONPACK
	if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
#ifndef TA_DATA // FLAG_MODEL
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
#endif
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}

	if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
#ifdef TA_WEAPSYS
		cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#endif
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}

#ifdef MISSIONPACK_HARVESTER
	if ( cgs.gametype == GT_HARVESTER || cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}
#endif

#ifdef TA_WEAPSYS // MELEE_TRAIL
	cgs.media.weaponTrailShader = trap_R_RegisterShader( "weaponTrail" );
	if (!cgs.media.flagFlapModel)
	{
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );
	}
#endif

#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
#endif
	cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );
#endif

#ifdef TURTLEARENA // LOCKON
	cgs.media.targetShader = trap_R_RegisterShader( "sprites/target" );
#endif

	if ( cgs.gametype >= GT_TEAM || cg_buildScript.integer ) {
#ifdef IOQ3ZTM // SHOW_TEAM_FRIENDS
		// Sprites used by STV: Elite Force
		cgs.media.blueFriendShader = trap_R_RegisterShader( "sprites/team_blue" );
		cgs.media.friendShader = trap_R_RegisterShader( "sprites/team_red" );
#if !defined TURTLEARENA || defined TA_SUPPORTQ3 // Fall back to Q3 friend shader
		if (!cgs.media.friendShader)
		{
			cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
		}
#endif
#else
		cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
#endif
#ifndef TURTLEARENA
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
#endif
#if defined MISSIONPACK && !defined TURTLEARENA // NO_KAMIKAZE_ITEM
		cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
#endif
	}

#ifdef TURTLEARENA
	cgs.media.hudHeadBackgroundShader = trap_R_RegisterShader( "gfx/2d/hudHeadBack" );
	cgs.media.hudBarShader = trap_R_RegisterShader( "gfx/2d/hudBar" );
	cgs.media.hudBar2Shader = trap_R_RegisterShader( "gfx/2d/hudBar2" );
	cgs.media.hudBarBackgroundShader = trap_R_RegisterShader( "gfx/2d/hudBarBack" );
	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar" );
#else
	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
#endif

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );
#ifdef TA_DATA // Fall back to machinegun shell.
	if (!cgs.media.shotgunBrassModel) {
		cgs.media.shotgunBrassModel = cgs.media.machinegunBrassModel;
	}
#endif

#ifndef NOTRATEDM // No gibs.
	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );
#endif

#ifdef TA_DATA // shaders
	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/talkBalloon" );
#else
	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );
#endif

#ifndef NOBLOOD
	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );
#endif
#ifdef TURTLEARENA // WEAPONS
	cgs.media.meleeHitShader[0] = trap_R_RegisterShader( "meleeHit1" );
	cgs.media.meleeHitShader[1] = trap_R_RegisterShader( "meleeHit2" );
	cgs.media.missileHitShader[0] = trap_R_RegisterShader( "missileHit1" );
	cgs.media.missileHitShader[1] = trap_R_RegisterShader( "missileHit2" );
#endif

#ifndef TA_WEAPSYS
	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#endif
#ifdef TURTLEARENA
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
#elif defined MISSIONPACK
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
#else
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
#endif
#ifdef MISSIONPACK
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
#ifndef TURTLEARENA // POWERS
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
#endif
	cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/regen.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );
#endif
#ifndef TURTLEARENA // AWARDS
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
#endif
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );

#ifdef TA_DATA // EXP_SCALE
	cgs.media.smokeModel = trap_R_RegisterModel("models/misc/smoke.md3");
#endif

#ifdef TA_MISC // MATERIALS
	// Load models for materials
	for (i = 0 ; i < NUM_MATERIAL_MODELS; i++)
	{
		for (j = 1; j < NUM_MATERIAL_TYPES; j++)
		{
			Com_sprintf (name, sizeof(name), "models/materials/%s%i.md3", materialInfo[j].name, i+1);
			cgs.media.matModels[j][i] = trap_R_RegisterModel(name);

			if (cgs.media.matModels[j][i]) {
				cgs.media.matNumModels[j]++;
			}
		}
	}
#endif


	memset( cg_items, 0, sizeof( cg_items ) );
#ifdef TA_WEAPSYS
	memset( cg_projectiles, 0, sizeof( cg_projectiles ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_weapongroups, 0, sizeof( cg_weapongroups ) );
#else
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
#endif

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < BG_NumItems() ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
#ifndef TURTLEARENA // NO_LOADING_ICONS
			CG_LoadingItem( i );
#endif
			CG_RegisterItemVisuals( i );
		}
	}

#ifdef TA_NPCSYS
	// only register the NPCs that the server says we need
	Q_strncpyz(npcs, CG_ConfigString( CS_NPCS), sizeof(npcs));

	for ( i = 1 ; i < BG_NumNPCs() ; i++ )
	{
		if ( npcs[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterNPCVisuals( i );
		}
	}
#endif

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
#ifndef NOTRATEDM // No gibs.
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );
#endif

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs, 0, 0, 0 );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

#ifdef MISSIONPACK
	// new stuff
#ifdef TURTLEARENA
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.png");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.png");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.png");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.png");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.png");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.png");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.png");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.png");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.png");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.png");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.png");
#else
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");
#endif

#ifndef TA_DATA // Don't percache models we don't have.
	trap_R_RegisterModel( "models/players/james/lower.md3" );
	trap_R_RegisterModel( "models/players/james/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/james/james.md3" );

	trap_R_RegisterModel( "models/players/janet/lower.md3" );
	trap_R_RegisterModel( "models/players/janet/upper.md3" );
	trap_R_RegisterModel( "models/players/heads/janet/janet.md3" );
#endif

#endif
	CG_ClearParticles ();
#ifdef IOQ3ZTM // Particles
	// In GAME call G_ParticleAreaIndex(str) to add a paritcle area.
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		int rval;

		rval = CG_NewParticleArea( CS_PARTICLES + i);
		if (!rval)
			break;
	}
#else
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
#endif
}


/*
==================
CG_LocalClientAdded
==================
*/
void CG_LocalClientAdded(int localClientNum, int clientNum) {
	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
		return;

	cg.localClients[localClientNum].clientNum = clientNum;
}

/*
==================
CG_LocalClientRemoved
==================
*/
void CG_LocalClientRemoved(int localClientNum) {
	if (cg.localClients[localClientNum].clientNum == -1)
		return;

	cg.localClients[localClientNum].clientNum = -1;
}

#ifdef MISSIONPACK
/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
}
#endif


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;
#ifndef TURTLEARENA // NO_LOADING_ICONS
	int		j;

	for (i = 0; i < CG_MaxSplitView(); i++) {
		if (cg.localClients[i].clientNum == -1) {
			continue;
		}
		CG_LoadingClient(cg.localClients[i].clientNum);
		CG_NewClientInfo(cg.localClients[i].clientNum);
	}
#endif

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

#ifndef TURTLEARENA // NO_LOADING_ICONS
		for (j = 0; j < CG_MaxSplitView(); j++) {
			if (cg.localClients[j].clientNum == i) {
				break;
			}
		}
		if (j != CG_MaxSplitView()) {
			continue;
		}
#endif

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
#ifndef TURTLEARENA // NO_LOADING_ICONS
		CG_LoadingClient( i );
#endif
		CG_NewClientInfo( i );
	}
#ifdef MISSIONPACK
	CG_BuildSpectatorString();
#endif
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}
#ifdef MISSIONPACK_HUD
char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
#ifdef IOQ3ZTM // FONT_REWRITE
			CG_LoadFont(&cgs.media.fontSmall, tempStr, "gfx/2d/bigchars", pointSize, pointSize/2, 0);
			CG_LoadFont(&cgs.media.fontBig, tempStr, "gfx/2d/bigchars", pointSize, pointSize, 0);
#else
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
#endif
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
#ifdef IOQ3ZTM // FONT_REWRITE
			CG_LoadFont(&cgs.media.fontTiny, tempStr, "gfx/2d/bigchars", pointSize, pointSize, 0);
#else
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
#endif
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
#ifdef IOQ3ZTM // FONT_REWRITE
			CG_LoadFont(&cgs.media.fontGiant, tempStr, "gfx/2d/bigchars", pointSize, pointSize*0.66f, 0);
#else
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
#endif
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse;
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile, NULL);
	if (!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu", NULL);
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( cgDC.menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean CG_Load_Menu(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);
    
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	char	*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	Init_Display(&cgDC);

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f ) {
		Com_Printf( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			CG_Error( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		trap_FS_FCloseFile( f );
		CG_Error( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	COM_Compress(buf);

	Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( cgDC.menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_DPrintf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);
}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = cg.cur_ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps && ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	bg_iteminfo_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle = cg_items[ BG_ItemNumForItem(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle = cg_items[ BG_ItemNumForItem(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle = cg_items[ BG_ItemNumForItem(item) ].icon;
				} else {
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else {
					*handle = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->pss[0].stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_TOURNAMENT) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}

static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width(CG_GameTypeString(), scale, 0);
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, 0);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(cg_redTeamName.string, scale, 0);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(cg_blueTeamName.string, scale, 0);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
	CG_AdjustFrom640( &x, &y, &w, &h );
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
	CG_AdjustFrom640( &x, &y, &w, &h );
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu( void ) {
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	cgDC.setBinding = &trap_Key_SetBinding;
	cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	cgDC.getKey = &trap_Key_GetKey;
	cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(cgDC.Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
#ifdef TA_DATA // MORE_COLOR_EFFECTS
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_ORANGE );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_LIME );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_VIVIDGREEN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[7] = trap_R_RegisterShaderNoMip( ART_FX_LIGHTBLUE );
	cgDC.Assets.fxPic[8] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[9] = trap_R_RegisterShaderNoMip( ART_FX_PURPLE );
	cgDC.Assets.fxPic[10] = trap_R_RegisterShaderNoMip( ART_FX_MAGENTA );
	cgDC.Assets.fxPic[11] = trap_R_RegisterShaderNoMip( ART_FX_PINK );
	cgDC.Assets.fxPic[12] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
#else
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_MAGENTA );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
#endif
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif
/*
=================
CG_Init

Called after every cgame load, such as main menu, level change, or subsystem restart
=================
*/
void CG_Init( qboolean inGameLoad, int maxSplitView ) {

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
#ifdef TA_WEAPSYS
	memset( cg_projectiles, 0, sizeof( cg_projectiles ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_weapongroups, 0, sizeof( cg_weapongroups ) );
#else
	memset( cg_weapons, 0, sizeof(cg_weapons) );
#endif
	memset( cg_items, 0, sizeof(cg_items) );
#ifdef TA_NPCSYS
	memset( cg_npcs, 0, sizeof(cg_npcs) );
#endif

	cg.connected = inGameLoad;

	cgs.maxSplitView = Com_Clamp(1, MAX_SPLITVIEW, maxSplitView);

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	// load a few needed things before we do any screen updates
#ifdef IOQ3ZTM // FONT_REWRITE
	CG_LoadFont(&cgs.media.fontTiny, "fonts/mplus-1c-regular.ttf", "gfx/2d/bigchars", 8, 8, 0);
	CG_LoadFont(&cgs.media.fontSmall, "fonts/mplus-1c-regular.ttf", "gfx/2d/bigchars", 16, 8, 0);
	CG_LoadFont(&cgs.media.fontBig, "fonts/mplus-1c-regular.ttf", "gfx/2d/bigchars", 16, 16, 0);
	CG_LoadFont(&cgs.media.fontGiant, "fonts/mplus-1c-regular.ttf", "gfx/2d/bigchars", 48, 32, 0);

	CG_LoadFont(&cgs.media.fontPropSmall, "fonts/mplus-1c-bold.ttf", "menu/art/font1_prop.tga", PROP_HEIGHT*PROP_SMALL_SIZE_SCALE, PROP_HEIGHT*PROP_SMALL_SIZE_SCALE*0.66f, 0);
	CG_LoadFont(&cgs.media.fontPropBig, "fonts/mplus-1c-bold.ttf", "menu/art/font1_prop.tga", PROP_HEIGHT, 22*0.66f, 0);
#ifndef TA_DATA
	CG_LoadFont(&cgs.media.fontPropGlowSmall, "fonts/mplus-1c-bold.ttf", "menu/art/font1_prop_glo.tga", PROP_HEIGHT*PROP_SMALL_SIZE_SCALE, PROP_HEIGHT*PROP_SMALL_SIZE_SCALE*0.66f, 0);
	CG_LoadFont(&cgs.media.fontPropGlowBig, "fonts/mplus-1c-bold.ttf", "menu/art/font1_prop_glo.tga", PROP_HEIGHT, 22*0.66f, 0);
#endif

	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
#else
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
#endif

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );

	// Viewport scale and offset
	cgs.screenXScaleStretch = cgs.glconfig.vidWidth * (1.0/640.0);
	cgs.screenYScaleStretch = cgs.glconfig.vidHeight * (1.0/480.0);
	if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
		cgs.screenXScale = cgs.glconfig.vidWidth * (1.0/640.0);
		cgs.screenYScale = cgs.glconfig.vidHeight * (1.0/480.0);
		// wide screen
		cgs.screenXBias = 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * (640.0/480.0) ) );
		cgs.screenXScale = cgs.screenYScale;
		// no narrow screen
		cgs.screenYBias = 0;
	}
	else {
		cgs.screenXScale = cgs.glconfig.vidWidth * (1.0/640.0);
		cgs.screenYScale = cgs.glconfig.vidHeight * (1.0/480.0);
		// narrow screen
		cgs.screenYBias = 0.5 * ( cgs.glconfig.vidHeight - ( cgs.glconfig.vidWidth * (480.0/640.0) ) );
		cgs.screenYScale = cgs.screenXScale;
		// no wide screen
		cgs.screenXBias = 0;
	}

#ifdef MISSIONPACK_HUD
	Init_Display(&cgDC);
	String_Init();
#endif

	UI_Init( inGameLoad, maxSplitView );
}

/*
=================
CG_Ingame_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Ingame_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int clientNum0, int clientNum1, int clientNum2, int clientNum3 ) {
	int	clientNums[MAX_SPLITVIEW];
	const char	*s;
	int			i;

	cgs.maxSplitView = Com_Clamp(1, MAX_SPLITVIEW, maxSplitView);
	cg.numViewports = 1;

	clientNums[0] = clientNum0;
	clientNums[1] = clientNum1;
	clientNums[2] = clientNum2;
	clientNums[3] = clientNum3;

	for (i = 0; i < CG_MaxSplitView(); i++) {
		if (clientNums[i] < 0 || clientNums[i] >= MAX_CLIENTS) {
			cg.localClients[i].clientNum = -1;
			continue;
		}

		trap_Mouse_SetState( i, MOUSE_CLIENT );
		trap_GetViewAngles( i, cg.localClients[i].viewangles );
		CG_LocalClientAdded(i, clientNums[i]);
	}

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	for (i = 0; i < CG_MaxSplitView(); i++) {
#ifdef TA_HOLDSYS/*2*/
		cg.localClients[i].holdableSelect = HI_NO_SELECT;
#endif
#if defined TA_PLAYERSYS && defined TA_WEAPSYS // DEFAULT_DEFAULT_WEAPON
		// Select our default weapon.
		cg.localClients[i].predictedPlayerState.stats[STAT_DEFAULTWEAPON] = cgs.clientinfo[cg.localClients[i].clientNum].playercfg.default_weapon;
#ifdef TA_WEAPSYS_EX
		cg.localClients[i].predictedPlayerState.stats[STAT_PENDING_WEAPON] = cg.localClients[i].predictedPlayerState.stats[STAT_DEFAULTWEAPON];
#else
		cg.localClients[i].weaponSelect = cg.localClients[i].predictedPlayerState.stats[STAT_DEFAULTWEAPON];
#endif
#else
		cg.localClients[i].weaponSelect = WP_MACHINEGUN;
#endif
	}

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	trap_SetMapTitle( CG_ConfigString( CS_MESSAGE ) );
	trap_SetNetFields( sizeof (entityState_t), bg_entityStateFields, bg_numEntityStateFields,
					   sizeof (playerState_t), bg_playerStateFields, bg_numPlayerStateFields );


	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "items" );

	BG_InitItemInfo();

#ifdef TA_ENTSYS // MISC_OBJECT
	CG_LoadingString( "objects" );

	BG_InitObjectConfig();
#endif

#ifdef TA_NPCSYS
	CG_LoadingString( "npcs" );

	BG_InitNPCInfo();
#endif

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef MISSIONPACK_HUD
	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff
#endif

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );

	CG_RestoreSnapshot();

#if defined IOQ3ZTM && defined TURTLEARENA // THIRD_PERSON LASERTAG
	if (cg_laserTag.integer)
		trap_Cvar_Set("cg_thirdPerson", "0");
	else
		trap_Cvar_Set("cg_thirdPerson", "1");
#endif
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	int i;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		trap_SetViewAngles( i, cg.localClients[ i ].viewangles );
	}

	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/*
=================
CG_Refresh

Draw the frame
=================
*/
void CG_Refresh( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, connstate_t state, int realTime ) {

	// update cvars
	CG_UpdateCvars();

	if ( state >= CA_LOADING && !UI_IsFullscreen() ) {
#ifdef MISSIONPACK_HUD
		Init_Display(&cgDC);
#endif
		CG_DrawActiveFrame( serverTime, stereoView, demoPlayback );
	}

	if ( state <= CA_LOADING || (trap_Key_GetCatcher() & KEYCATCH_UI) ) {
		if ( ui_stretch.integer ) {
			CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );
		} else {
			CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
		}
		UI_Refresh( realTime );
	}

	// connecting clients will show the connection dialog
	if ( state >= CA_CONNECTING && state < CA_ACTIVE ) {
		UI_DrawConnectScreen( ( state >= CA_LOADING ) );
	}
}

/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
#ifndef MISSIONPACK_HUD
void CG_EventHandling(int type) {
}



void CG_KeyEvent(int key, qboolean down) {
}

void CG_MouseEvent(int localClientNum, int x, int y) {
}
#endif

/*
=================
CG_JoystickEvent

Joystick values stay set until changed
=================
*/
void CG_JoystickEvent( int localClientNum, int axis, int value ) {
	if ( localClientNum < 0 || localClientNum >= MAX_SPLITVIEW) {
		return;
	}
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		CG_Error( "CG_JoystickEvent: bad axis %i", axis );
	}

	cg.localClients[localClientNum].joystickAxis[axis] = value;
}

/*
================
CG_VoIPString
================
*/
static char *CG_VoIPString( int localClientNum ) {
	// a generous overestimate of the space needed for 0,1,2...61,62,63
	static char voipString[ MAX_CLIENTS * 4 ];
	char voipSendTarget[ MAX_CVAR_VALUE_STRING ];

	if ( localClientNum < 0 || localClientNum > CG_MaxSplitView() || cg.localClients[localClientNum].clientNum == -1 ) {
		return NULL;
	}

	trap_Argv( 0, voipSendTarget, sizeof( voipSendTarget ) );

	if( Q_stricmpn( voipSendTarget, "team", 4 ) == 0 )
	{
		int i, slen, nlen;
		for( slen = i = 0; i < cgs.maxclients; i++ )
		{
			if( !cgs.clientinfo[ i ].infoValid || i == cg.localClients[ localClientNum ].clientNum )
				continue;
			if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.localClients[ localClientNum ].clientNum ].team )
				continue;

			nlen = Com_sprintf( &voipString[ slen ], sizeof( voipString ) - slen,
					"%s%d", ( slen > 0 ) ? "," : "", i );
			if( slen + nlen + 1 >= sizeof( voipString ) )
			{
				CG_Printf( S_COLOR_YELLOW "WARNING: voipString overflowed\n" );
				break;
			}

			slen += nlen;
		}

		// Notice that if the Com_sprintf was truncated, slen was not updated
		// so this will remove any trailing commas or partially-completed numbers
		voipString[ slen ] = '\0';
	}
	else if( Q_stricmpn( voipSendTarget, "crosshair", 9 ) == 0 )
		Com_sprintf( voipString, sizeof( voipString ), "%d",
				CG_CrosshairPlayer( localClientNum ) );
	else if( Q_stricmpn( voipSendTarget, "attacker", 8 ) == 0 )
		Com_sprintf( voipString, sizeof( voipString ), "%d",
				CG_LastAttacker( localClientNum ) );
	else
		return NULL;

	return voipString;
}
