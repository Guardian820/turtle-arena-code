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
#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../cgame/cg_local.h"
#include "../ui/ui_public.h"

#ifdef IOQ3ZTM // FONT_REWRITE
// ZTM: FIXME: cgame replaces height macros with function calls that aren't valid in main menu.

#undef TINYCHAR_HEIGHT
//#define TINYCHAR_HEIGHT (Com_FontCharHeight(&uis.fontTiny, 0))

#undef SMALLCHAR_HEIGHT
#define SMALLCHAR_HEIGHT (Com_FontCharHeight(&uis.fontSmall, 0))

#undef BIGCHAR_HEIGHT
#define BIGCHAR_HEIGHT (Com_FontCharHeight(&uis.fontBig, 0))

#undef GIANTCHAR_HEIGHT
#define GIANTCHAR_HEIGHT (Com_FontCharHeight(&uis.fontGiant, 0))
#endif

typedef void (*voidfunc_f)(void);

extern vmCvar_t	ui_ffa_fraglimit;
extern vmCvar_t	ui_ffa_timelimit;

extern vmCvar_t	ui_tourney_fraglimit;
extern vmCvar_t	ui_tourney_timelimit;

extern vmCvar_t	ui_team_fraglimit;
extern vmCvar_t	ui_team_timelimit;
extern vmCvar_t	ui_team_friendly;

extern vmCvar_t	ui_ctf_capturelimit;
extern vmCvar_t	ui_ctf_timelimit;
extern vmCvar_t	ui_ctf_friendly;

#ifdef MISSIONPACK
extern vmCvar_t	ui_1flag_capturelimit;
extern vmCvar_t	ui_1flag_timelimit;
extern vmCvar_t	ui_1flag_friendly;

extern vmCvar_t	ui_obelisk_capturelimit;
extern vmCvar_t	ui_obelisk_timelimit;
extern vmCvar_t	ui_obelisk_friendly;

#ifdef MISSIONPACK_HARVESTER
extern vmCvar_t	ui_harvester_capturelimit;
extern vmCvar_t	ui_harvester_timelimit;
extern vmCvar_t	ui_harvester_friendly;
#endif
#endif

extern vmCvar_t	ui_publicServer;

extern vmCvar_t	ui_arenasFile;
extern vmCvar_t	ui_botsFile;
extern vmCvar_t	ui_spScores1;
extern vmCvar_t	ui_spScores2;
extern vmCvar_t	ui_spScores3;
extern vmCvar_t	ui_spScores4;
extern vmCvar_t	ui_spScores5;
extern vmCvar_t	ui_spAwards;
extern vmCvar_t	ui_spVideos;
extern vmCvar_t	ui_spSkill;

#ifdef TA_SP
extern vmCvar_t	ui_singlePlayerActive;
extern vmCvar_t	ui_saveVersions;
extern vmCvar_t	ui_saveFilename;
#endif
extern vmCvar_t	ui_spSelection;

#ifdef TA_SP
extern vmCvar_t	ui_arcadeName;
#endif

extern vmCvar_t	ui_browserMaster;
extern vmCvar_t	ui_browserGameType;
extern vmCvar_t	ui_browserSortKey;
extern vmCvar_t	ui_browserShowFull;
extern vmCvar_t	ui_browserShowEmpty;
extern vmCvar_t	ui_browserShowBots;

extern vmCvar_t	ui_brassTime;
extern vmCvar_t	ui_drawCrosshair;
extern vmCvar_t	ui_drawCrosshairNames;
extern vmCvar_t	ui_marks;

extern vmCvar_t	ui_server1;
extern vmCvar_t	ui_server2;
extern vmCvar_t	ui_server3;
extern vmCvar_t	ui_server4;
extern vmCvar_t	ui_server5;
extern vmCvar_t	ui_server6;
extern vmCvar_t	ui_server7;
extern vmCvar_t	ui_server8;
extern vmCvar_t	ui_server9;
extern vmCvar_t	ui_server10;
extern vmCvar_t	ui_server11;
extern vmCvar_t	ui_server12;
extern vmCvar_t	ui_server13;
extern vmCvar_t	ui_server14;
extern vmCvar_t	ui_server15;
extern vmCvar_t	ui_server16;

extern vmCvar_t	ui_ioq3;


//
// ui_qmenu.c
//

#define RCOLUMN_OFFSET			( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET			(-BIGCHAR_WIDTH )

#define SLIDER_RANGE			10

#define MAX_MENUDEPTH			8
#define MAX_MENUITEMS			64

#define MTYPE_NULL				0
#define MTYPE_SLIDER			1	
#define MTYPE_ACTION			2
#define MTYPE_SPINCONTROL		3
#define MTYPE_FIELD				4
#define MTYPE_RADIOBUTTON		5
#define MTYPE_BITMAP			6	
#define MTYPE_TEXT				7
#define MTYPE_SCROLLLIST		8
#define MTYPE_PTEXT				9
#define MTYPE_BTEXT				10

#define QMF_BLINK				((unsigned int) 0x00000001)
#define QMF_SMALLFONT			((unsigned int) 0x00000002)
#define QMF_LEFT_JUSTIFY		((unsigned int) 0x00000004)
#define QMF_CENTER_JUSTIFY		((unsigned int) 0x00000008)
#define QMF_RIGHT_JUSTIFY		((unsigned int) 0x00000010)
#define QMF_NUMBERSONLY			((unsigned int) 0x00000020)	// edit field is only numbers
#define QMF_HIGHLIGHT			((unsigned int) 0x00000040)
#define QMF_HIGHLIGHT_IF_FOCUS	((unsigned int) 0x00000080)	// steady focus
#define QMF_PULSEIFFOCUS		((unsigned int) 0x00000100)	// pulse if focus
#define QMF_HASMOUSEFOCUS		((unsigned int) 0x00000200)
#define QMF_NOONOFFTEXT			((unsigned int) 0x00000400)
#define QMF_MOUSEONLY			((unsigned int) 0x00000800)	// only mouse input allowed
#define QMF_HIDDEN				((unsigned int) 0x00001000)	// skips drawing
#define QMF_GRAYED				((unsigned int) 0x00002000)	// grays and disables
#define QMF_INACTIVE			((unsigned int) 0x00004000)	// disables any input
#define QMF_NODEFAULTINIT		((unsigned int) 0x00008000)	// skip default initialization
#define QMF_OWNERDRAW			((unsigned int) 0x00010000)
#define QMF_PULSE				((unsigned int) 0x00020000)
#define QMF_LOWERCASE			((unsigned int) 0x00040000)	// edit field is all lower case
#define QMF_UPPERCASE			((unsigned int) 0x00080000)	// edit field is all upper case
#define QMF_SILENT				((unsigned int) 0x00100000)

// callback notifications
#define QM_GOTFOCUS				1
#define QM_LOSTFOCUS			2
#define QM_ACTIVATED			3

typedef struct _tag_menuframework
{
	int	cursor;
	int cursor_prev;

	int	nitems;
	void *items[MAX_MENUITEMS];

	void (*draw) (void);
	sfxHandle_t (*key) (int key);

	qboolean	wrapAround;
	qboolean	fullscreen;
#ifndef TA_DATA
	qboolean	showlogo;
#endif
#ifdef IOQ3ZTM
	qboolean	noEscape; // Don't run UI_PopMenu when Esc is pressed
#endif
} menuframework_s;

typedef struct
{
	int type;
	const char *name;
	int	id;
	int x, y;
	int left;
	int	top;
	int	right;
	int	bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned int flags;

	void (*callback)( void *self, int event );
	void (*statusbar)( void *self );
	void (*ownerdraw)( void *self );
} menucommon_s;

typedef struct
{
	menucommon_s	generic;
	mfield_t		field;
} menufield_s;

typedef struct 
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

typedef struct
{
	menucommon_s generic;

	int	oldvalue;
	int curvalue;
	int	numitems;
	int	top;
		
	const char **itemnames;

	int width;
	int height;
	int	columns;
	int	seperation;
} menulist_s;

typedef struct
{
	menucommon_s generic;
} menuaction_s;

typedef struct
{
	menucommon_s generic;
	int curvalue;
} menuradiobutton_s;

typedef struct
{
	menucommon_s	generic;
	char*			focuspic;	
	char*			errorpic;
	qhandle_t		shader;
	qhandle_t		focusshader;
	int				width;
	int				height;
	float*			focuscolor;
} menubitmap_s;

typedef struct
{
	menucommon_s	generic;
	char*			string;
	int				style;
	float*			color;
} menutext_s;

extern void			Menu_Cache( void );
extern void			Menu_Focus( menucommon_s *m );
extern void			Menu_AddItem( menuframework_s *menu, void *item );
extern void			Menu_AdjustCursor( menuframework_s *menu, int dir );
extern void			Menu_Draw( menuframework_s *menu );
extern void			*Menu_ItemAtCursor( menuframework_s *m );
extern sfxHandle_t	Menu_ActivateItem( menuframework_s *s, menucommon_s* item );
extern void			Menu_SetCursor( menuframework_s *s, int cursor );
extern void			Menu_SetCursorToItem( menuframework_s *m, void* ptr );
extern sfxHandle_t	Menu_DefaultKey( menuframework_s *s, int key );
extern void			Bitmap_Init( menubitmap_s *b );
extern void			Bitmap_Draw( menubitmap_s *b );
extern void			ScrollList_Draw( menulist_s *l );
extern sfxHandle_t	ScrollList_Key( menulist_s *l, int key );
extern sfxHandle_t	menu_in_sound;
extern sfxHandle_t	menu_move_sound;
extern sfxHandle_t	menu_out_sound;
extern sfxHandle_t	menu_buzz_sound;
extern sfxHandle_t	menu_null_sound;
#ifndef TA_WEAPSYS_EX
extern sfxHandle_t	weaponChangeSound;
#endif
extern vec4_t		menu_text_color;
extern vec4_t		menu_grayed_color;
extern vec4_t		menu_dark_color;
extern vec4_t		menu_highlight_color;
extern vec4_t		menu_red_color;
extern vec4_t		menu_black_color;
extern vec4_t		menu_dim_color;
extern vec4_t		color_black;
extern vec4_t		color_white;
extern vec4_t		color_yellow;
extern vec4_t		color_blue;
extern vec4_t		color_orange;
extern vec4_t		color_red;
#ifdef TURTLEARENA
extern vec4_t		color_green;
#endif
extern vec4_t		color_dim;
extern vec4_t		name_color;
extern vec4_t		list_color;
extern vec4_t		listbar_color;
extern vec4_t		text_color_disabled; 
extern vec4_t		text_color_normal;
extern vec4_t		text_color_highlight;
extern vec4_t		text_banner_color;
extern vec4_t		text_big_color;
extern vec4_t		text_small_title_color;

extern char	*ui_medalNames[];
extern char	*ui_medalPicNames[];
extern char	*ui_medalSounds[];

//
// ui_mfield.c
//
extern void			UI_Field_Draw( mfield_t *edit, int x, int y, int style, vec4_t color );
extern void			MenuField_Init( menufield_s* m );
extern void			MenuField_Draw( menufield_s *f );
extern sfxHandle_t	MenuField_Key( menufield_s* m, int* key );

//
// ui_menu.c
//
extern void MainMenu_Cache( void );
extern void UI_MainMenu(void);
extern void UI_RegisterCvars( void );
extern void UI_UpdateCvars( void );

//
// ui_credits.c
//
extern void UI_CreditMenu( void );
#ifdef TURTLEARENA // LONG_CREDITS
extern void UI_LongCreditMenu( void );
#endif

//
// ui_ingame.c
//
extern void InGame_Cache( void );
extern void UI_InGameMenu(void);

#ifdef TA_MISC // INGAME_SERVER_MENU
//
// ui_ingame_server.c
//
extern void InServer_Cache( void );
extern void UI_InServerMenu(void);
#endif

//
// ui_ingame_selectplayer.c
//
extern void InSelectPlayer_Cache( void );
extern void InSelectPlayerMenu( void (*playerfunc)(int), const char *banner, qboolean disableMissingPlayers );

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache( void );
extern void UI_ConfirmMenu( const char *question, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_ConfirmMenu_Style( const char *question, int style, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_Message( const char **lines );

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache( void );
extern void UI_SetupMenu(void);

//
// ui_team.c
//
extern void UI_TeamMainMenu( int localPlayerNum );
extern void TeamMain_Cache( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_selectplayer.c
//
extern void UI_SelectPlayerMenu( void (*playerfunc)(int), const char *banner );
extern void UI_SelectPlayer_Cache( void );

//
// ui_controls2.c
//
extern void UI_ControlsMenu( int localPlayerNum );
extern void Controls_Cache( void );
extern qboolean Controls_WantsBindKeys( void );

//
// ui_joystick.c
//
void UI_JoystickMenu( int localPlayerNum );
void UI_Joystick_Cache( void );

//
// ui_demo2.c
//
extern void UI_DemosMenu( void );
extern void Demos_Cache( void );

//
// ui_cinematics.c
//
extern void UI_CinematicsMenu( void );
extern void UI_CinematicsMenu_f( void );
extern void UI_CinematicsMenu_Cache( void );

//
// ui_mods.c
//
extern void UI_ModsMenu( void );
extern void UI_ModsMenu_Cache( void );

#ifdef TA_MISC
//
// ui_multiplayer.c
//
extern void UI_MultiplayerMenu( void );
extern void UI_Multiplayer_Cache( void );

//
// ui_playersetup.c
//
void UI_PlayerSetupMenu( int maxLocalClients, void (*action)(void), qboolean playButton );
void UI_PlayerSetupMenu_Cache( void );
#else
//
// ui_playermodel.c
//
extern void UI_PlayerModelMenu( int localPlayerNum );
extern void PlayerModel_Cache( void );

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu( int localPlayerNum );
extern void PlayerSettings_Cache( void );
#endif

//
// ui_preferences.c
//
extern void UI_PreferencesMenu( void );
extern void Preferences_Cache( void );

//
// ui_specifyserver.c
//
#ifdef IOQ3ZTM
extern void UI_SpecifyServerMenu( qboolean fav );
#else
extern void UI_SpecifyServerMenu( void );
#endif
extern void SpecifyServer_Cache( void );

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu( void );
extern void ArenaServers_Cache( void );

//
// ui_startserver.c
//
extern void UI_StartServerMenu( qboolean multiplayer );
extern void StartServer_Cache( void );
#ifndef TA_SP
extern void ServerOptions_Cache( void );
extern void UI_BotSelectMenu( char *bot );
extern void UI_BotSelectMenu_Cache( void );
#endif

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu( void );
extern void ServerInfo_Cache( void );

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu( void );
extern void GraphicsOptions_Cache( void );
extern void DriverInfo_Cache( void );

//
// ui_players.c
//

#ifndef TA_MISC
typedef struct {
	// model info
	qhandle_t		legsModel;
	lerpFrame_t		legs;

	qhandle_t		torsoModel;
	lerpFrame_t		torso;

	qhandle_t		headModel;

	cgSkin_t		modelSkin;

#ifdef TA_PLAYERSYS
	bg_playercfg_t	playercfg;
#else
	animation_t		animations[MAX_ANIMATIONS];
#endif

#ifdef TA_WEAPSYS
	qhandle_t		weaponModel[MAX_HANDS];
	qhandle_t		barrelModel[MAX_HANDS];
	qhandle_t		flashModel[MAX_HANDS];
	vec3_t			flashDlightColor[MAX_HANDS];
#else
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	vec3_t			flashDlightColor;
#endif
	int				muzzleFlashTime;

	vec3_t			color1;
	byte			c1RGBA[4];

	// currently in use drawing parms
	vec3_t			viewAngles;
	vec3_t			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;

	int				pendingTorsoAnim;
	int				legsAnimationTimer;

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;
} uiPlayerInfo_t;

void UI_DrawPlayer( float x, float y, float w, float h, uiPlayerInfo_t *pi, int time );
void UI_PlayerInfo_SetModel( uiPlayerInfo_t *pi, const char *model, const char *headmodel, char *teamName );
void UI_PlayerInfo_SetInfo( uiPlayerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, qboolean chat );
qboolean UI_RegisterPlayerModelname( uiPlayerInfo_t *pi, const char *modelSkinName, const char *headModelSkinName, const char *teamName );
#endif

//
// ui_atoms.c
//
typedef struct {
	int					frametime;
	int					realtime;
	int					cursorx;
	int					cursory;
	int					menusp;
	menuframework_s*	activemenu;
	menuframework_s*	stack[MAX_MENUDEPTH];
	qboolean			debug;
	qhandle_t			whiteShader;
	qhandle_t			menuBackShader;
#ifdef TA_DATA
	qhandle_t			menuBackInGameShader;
#else
	qhandle_t			menuBackNoLogoShader;
#endif
#ifdef IOQ3ZTM // FONT_REWRITE
	font_t				fontSmall;
	font_t				fontBig;
	font_t				fontGiant;
	font_t				fontPropSmall;
	font_t				fontPropBig;
#ifndef TA_DATA
	font_t				fontPropGlowSmall;
	font_t				fontPropGlowBig;
#endif
	font_t				fontBanner;
#else
	qhandle_t			charset;
	qhandle_t			charsetProp;
#ifndef TA_DATA
	qhandle_t			charsetPropGlow;
#endif
	qhandle_t			charsetPropB;
#endif
	qhandle_t			cursor;
	qhandle_t			rb_on;
	qhandle_t			rb_off;
	qboolean			demoversion;
	qboolean			firstdraw;
	int					maxSplitView;
} uiStatic_t;

#ifdef TA_DATA
extern void			UI_DrawPicFullScreen(qhandle_t hShader);
#endif
extern void			UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
#ifdef IOQ3ZTM // FONT_REWRITE
extern qboolean		UI_LoadFont(font_t *font, const char *ttfName, const char *shaderName, int pointSize,
							int shaderCharWidth, float fontKerning);
extern void			UI_DrawFontChar( font_t *font, float x, float y, int ch, qboolean adjustFrom640 );
extern void			UI_DrawFontString( font_t *font, int x, int y, const char *s, float alpha );
extern void			UI_DrawFontStringColor( font_t *font, int x, int y, const char *s, vec4_t color );
font_t *UI_FontForStyle( int style );
font_t *UI_ProportionalFontForStyle( int style );
#ifndef TA_DATA
font_t *UI_ProportionalGlowFontForStyle( int style );
#endif
#endif
extern void			UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color );
extern float		UI_ProportionalSizeScale( int style );
extern void			UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
extern void			UI_DrawProportionalString_AutoWrapped( int x, int ystart, int xmax, int ystep, const char* str, int style, vec4_t color );
extern int			UI_ProportionalStringWidth( const char* str, int style );
extern void			UI_DrawString( int x, int y, const char* str, int style, vec4_t color );
extern int			UI_DrawChar( int x, int y, int ch, int style, vec4_t color );
extern qboolean 	UI_CursorInRect (int x, int y, int width, int height);
extern qboolean		UI_IsFullscreen( void );
extern void			UI_SetActiveMenu( uiMenuCommand_t menu );
extern void			UI_PushMenu ( menuframework_s *menu );
extern void			UI_PopMenu (void);
extern void			UI_ForceMenuOff (void);
extern void			UI_Refresh( int time );
extern int			UI_MaxSplitView(void);
extern qboolean		m_entersound;
extern uiStatic_t	uis;

#ifndef TA_SP
//
// ui_spLevel.c
//
void UI_SPLevelMenu_Cache( void );
void UI_SPLevelMenu( void );
void UI_SPLevelMenu_f( void );
void UI_SPLevelMenu_ReInit( void );
#endif

//
// ui_spArena.c
//
void UI_SPArena_Start( const char *arenaInfo );
#ifdef TA_SP
void UI_LoadGameMenu( void );
void LoadGame_Cache( void );
void UI_SPMenu( void );
void UI_SPMenu_f( void );
void UI_SPMenu_Cache( void );
#endif

//
// ui_spPostgame.c
//
void UI_SPPostgameMenu_Cache( void );
void UI_SPPostgameMenu_f( void );

#ifdef TA_SP
//
// ui_spPlayer.c
//
#define NUM_SP_CHARACTERS 4
extern const char *spCharacterNames[NUM_SP_CHARACTERS];
void UI_SPPlayerMenu( int maxLocalClients, void (*action)(void) );
void UI_SPPlayerMenu_Cache( void );
#else
//
// ui_spSkill.c
//
void UI_SPSkillMenu( const char *arenaInfo );
void UI_SPSkillMenu_Cache( void );
#endif

//
// ui_addbots.c
//
void UI_AddBots_Cache( void );
void UI_AddBotsMenu( void );

//
// ui_removebots.c
//
void UI_RemoveBots_Cache( void );
void UI_RemoveBotsMenu( void );

//
// ui_teamorders.c
//
extern void UI_TeamOrdersMenu( void );
extern void UI_TeamOrdersMenu_f( void );
extern void UI_TeamOrdersMenu_Cache( void );

//
// ui_loadconfig.c
//
void UI_LoadConfig_Cache( void );
void UI_LoadConfigMenu( void );

//
// ui_saveconfig.c
//
void UI_SaveConfigMenu_Cache( void );
void UI_SaveConfigMenu( void );

//
// ui_display.c
//
void UI_DisplayOptionsMenu_Cache( void );
void UI_DisplayOptionsMenu( void );

//
// ui_sound.c
//
void UI_SoundOptionsMenu_Cache( void );
void UI_SoundOptionsMenu( void );

//
// ui_network.c
//
void UI_NetworkOptionsMenu_Cache( void );
void UI_NetworkOptionsMenu( void );

//
// ui_gameinfo.c
//
typedef enum {
	AWARD_ACCURACY,
#ifndef TURTLEARENA // AWARDS
	AWARD_IMPRESSIVE,
	AWARD_EXCELLENT,
	AWARD_GAUNTLET,
#endif
	AWARD_FRAGS,
	AWARD_PERFECT
} awardType_t;

const char *UI_GetArenaInfoByNumber( int num );
const char *UI_GetArenaInfoByMap( const char *map );
const char *UI_GetSpecialArenaInfo( const char *tag );
int UI_GetNumArenas( void );
int UI_GetNumSPArenas( void );
int UI_GetNumSPTiers( void );

char *UI_GetBotInfoByNumber( int num );
char *UI_GetBotInfoByName( const char *name );
int UI_GetNumBots( void );

#ifndef TA_SP
void UI_GetBestScore( int level, int *score, int *skill );
void UI_SetBestScore( int level, int score );
int UI_TierCompleted( int levelWon );
qboolean UI_ShowTierVideo( int tier );
qboolean UI_CanShowTierVideo( int tier );
int  UI_GetCurrentGame( void );
void UI_NewGame( void );
void UI_LogAwardData( int award, int data );
int UI_GetAwardLevel( int award );
#endif

void UI_SPUnlock_f( void );
#ifndef TA_SP
void UI_SPUnlockMedals_f( void );
#endif

void UI_InitGameinfo( void );

#ifdef TA_SP
// Arcade map gamedata
#define ARCADE_GAMEDATA_MAGIC "EBXARCADE"
#define ARCADE_GAMEDATA_VERSION 0
#define NUM_ARCADE_SCORES 5

typedef struct
{
	char name[9];
	char character[17];
	int score;
	int time;

	// Additional CTF data
	int captures;
	int redScore;
	int blueScore;
} arcadeScore_t;

typedef struct
{
	char magic[9];
	int version;
	arcadeScore_t scores[NUM_ARCADE_SCORES];
} arcadeGameData_t;

qhandle_t UI_GetScoreIcon( arcadeScore_t score );
#endif

#endif
