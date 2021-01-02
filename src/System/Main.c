/****************************/
/*    NEW GAME - MAIN 		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	Boolean			gDrawLensFlare,gGameIsRegistered,gSerialWasVerified,gSuccessfulHTTPRead,gDisableHiccupTimer,gDoDeathExit, gLittleSnitch;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac,gStartingLightTimer;
extern	float			gTileSlipperyFactor,gSpinningPlatformRot;
extern	OGLPoint3D	gCoord;
extern	ObjNode				*gFirstNodePtr, *gAlienSaucer;
extern	short		gNumSuperTilesDrawn;
extern	float		gGlobalTransparency;
extern	int			gMaxItemsAllocatedInAPass,gNumObjectNodes,gNumHumansRescuedTotal;
extern	SavePlayerType	gPlayerSaveData;
extern	PrefsType	gGamePrefs;
extern	short	gNumTerrainDeformations;
extern	DeformationType	gDeformationList[];
extern	long			gTerrainUnitWidth,gTerrainUnitDepth;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	Handle		gHTTPDataHandle;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void CleanupLevel(void);
static void PlayArea(void);
static void PlayGame(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	NORMAL_GRAVITY	5200.0f


#define	DEFAULT_ANAGLYPH_R	0xd8
#define	DEFAULT_ANAGLYPH_G	0x90
#define	DEFAULT_ANAGLYPH_B	0xff

#define kDefaultNibFileName "main"


/****************************/
/*    VARIABLES             */
/****************************/

Boolean				gOSX = false, gG4 = false;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

u_long				gAutoFadeStatusBits;
short				gMainAppRezFile;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;

FSSpec				gDataSpec;


OGLVector3D			gWorldSunDirection = { .5, -.35, .8};		// also serves as lense flare vector
OGLColorRGBA		gFillColor1 = { .9, .9, 0.85, 1};
OGLColorRGBA		gApocalypseColor = {.6, .6, .7,1};
OGLColorRGBA		gFireIceColor = {.7, .6, .6,1};

u_long				gGameFrameNum = 0;

Boolean				gPlayingFromSavedGame = false;
Boolean				gGameOver = false;
Boolean				gLevelCompleted = false;
float				gLevelCompletedCoolDownTimer = 0;

int					gLevelNum;

short				gBestCheckpointNum;
OGLPoint2D			gBestCheckpointCoord;
float				gBestCheckpointAim;

int					gScratch = 0;
float				gScratchF = 0;

Boolean				gShareware = false;

#if DEMO
float	gDemoVersionTimer = 0;
#endif

u_long	gScore,gLoadedScore;


CFBundleRef 		gBundle = nil;
IBNibRef 			gNibs = nil;


//======================================================================================
//======================================================================================
//======================================================================================

#include "serialVerify.h"


/****************** TOOLBOX INIT  *****************/

void ToolBoxInit(void)
{
long		response;
OSErr		iErr;
NumVersion	vers;
Boolean		noRTL,RTL;



	MyFlushEvents();
	
	gMainAppRezFile = CurResFile();


			/*****************/
			/* INIT NIB INFO */
			/*****************/
			
	gBundle = CFBundleGetMainBundle();
	if (gBundle == nil)
		DoFatalAlert("\pToolBoxInit: CFBundleGetMainBundle() failed!");
	if (CreateNibReferenceWithCFBundle(gBundle, CFSTR(kDefaultNibFileName), &gNibs) != noErr)
		DoFatalAlert("\pToolBoxInit: CreateNibReferenceWithCFBundle() failed!");
	




		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */
				
	VerifySystem();


			/* BOOT OGL */
			
	OGL_Boot();


			/******************/
			/* INIT QUICKTIME */
			/******************/

			/* SEE IF QT INSTALLED */
									
	iErr = Gestalt(gestaltQuickTime,&response);
	if(iErr != noErr)
		DoFatalAlert("\pThis application requires Quicktime 4 or newer");


			/* SEE IF HAVE 4 */

	iErr = Gestalt(gestaltQuickTimeVersion,(long *)&vers);
	if ((vers.majorRev < 4) ||
		((vers.majorRev == 4) && (vers.minorAndBugRev < 0x11)))
			DoFatalAlert("\pThis application requires Quicktime 4.1.1 or newer which you can download from www.apple.com/quicktime");


			/* START QUICKTIME */
			
	EnterMovies();
	
    


		/* SEE IF PROCESSOR SUPPORTS frsqrte */
	
	if (!Gestalt(gestaltNativeCPUtype, &response))
	{
		switch(response)
		{
			case	gestaltCPU601:				// 601 is only that doesnt support it
					DoFatalAlert("\pSorry, but this app will not run on a PowerPC 601, only on newer Macintoshes.");
					break;
		}
	}
	
			/* DETERMINE IF SHAREWARE OR BOXED VERSION */
			
#if DEMO || OEM
	gShareware = false;
	noRTL = RTL = false;
#else	
	{
		FSSpec	spc;
		
		noRTL = (FSMakeFSSpec(0, 0, "\p:Data:Images:NORTL", &spc) == noErr);		// the presence of this file will tell us if it's shareware or boxed
		RTL = (FSMakeFSSpec(0, 0, "\p:Data:Images:RTL", &spc) == noErr);		
				
		if (noRTL)
			gShareware = true;	
		else
			gShareware = false;
	}
#endif
	

 	InitInput();


			/********************/
			/* INIT PREFERENCES */
			/********************/
	
	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);	
	
	
			/************************************/
            /* SEE IF GAME IS REGISTERED OR NOT */
			/************************************/

	if (((!noRTL) && (!RTL)) || DEMO)								// if no RTL or NORTL found then it's the old Aspyr version, thus no serial#
	{
		gGameIsRegistered = true;
		gSerialWasVerified = true;
	}
	else
	    CheckGameSerialNumber(false);


		/*************************************/
		/* SEE IF SHOULD DO WEB UPDATE CHECK */
		/*************************************/

#if !DEMO
	if (gGameIsRegistered)											// only do HTTP if running full version in registered mode
	{
		DateTimeRec	dateTime;
		u_long		seconds, seconds2;
	
		GetTime(&dateTime);											// get date time
		DateToSeconds(&dateTime, &seconds);			
				
		DateToSeconds(&gGamePrefs.lastVersCheckDate, &seconds2);			
		
		if ((seconds - seconds2) > 86400)							// see if 1 days have passed since last check
		{
			MyFlushEvents();
			gGamePrefs.lastVersCheckDate = dateTime;				// update time

			
					/* READ THE UPDATEDATA FILE & PARSE */
					
			gSuccessfulHTTPRead = false;							// assume the HTTP read will fail
			ReadHTTPData_VersionInfo();								// do version check (also checks serial #'s)
			
			
				/* FOR UNKNOWN REASONS THE HTTP READ FAILED - POSSIBLE PIRATE ACTIVITY */
				//
				// If trying to read our UpdateData file from all of the URL's in the list fails then that's very suspicious.
				// This is probably caused by pirates trying to firefall those sites out, however, it could simply be that the
				// internet connection is down.  So, to verify that the internet is working, just try to read apple.com.
				// If apple.com fails, then the internet connection is really down, but if it loads, then someone is probably
				// blocking our URL's.  We'll let this happen a few times before we warn the user.
				//
					
			if (!gSuccessfulHTTPRead)
			{
				if (gHTTPDataHandle)								// only bother if we've got a handle to read into			
				{
					if (DownloadURL("http://www.apple.com") == noErr) // let's be safe and verify we have an internet connection by reading apple.com
					{
						gGamePrefs.numHTTPReadFails++;
						if (gGamePrefs.numHTTPReadFails > 5)		// if we've failed too many times then tell the user
						{
							gGamePrefs.lastVersCheckDate.year = 0;	// make sure it checks the next time we run the game
							SavePrefs();
						
							DoHTTPMultipleFailureWarning();
						}
					}
				}
			}
			else
				gGamePrefs.numHTTPReadFails = 0;					// we read the UpdateData fine, so reset our failure counter

			SavePrefs();
		}	
	}
#endif	
	
	
	
	
			/*********************************/
			/* DO BOOT CHECK FOR SCREEN MODE */
			/*********************************/
			
	DoScreenModeDialog();	
						
			
	MyFlushEvents();
}


/********** INIT DEFAULT PREFS ******************/

void InitDefaultPrefs(void)
{
long 		keyboardScript, languageCode, i;

		/* DETERMINE WHAT LANGUAGE IS ON THIS MACHINE */
					
	keyboardScript = GetScriptManagerVariable(smKeyScript);
	languageCode = GetScriptVariable(keyboardScript, smScriptLang);			
		
	switch(languageCode)
	{
		case	langFrench:
				gGamePrefs.language 			= LANGUAGE_FRENCH;
				break;

		case	langGerman:
				gGamePrefs.language 			= LANGUAGE_GERMAN;
				break;

		case	langSpanish:
				gGamePrefs.language 			= LANGUAGE_SPANISH;
				break;

		case	langItalian:
				gGamePrefs.language 			= LANGUAGE_ITALIAN;
				break;
	
		default:
				gGamePrefs.language 			= LANGUAGE_ENGLISH;
	}
			
	gGamePrefs.difficulty			= 0;	
	gGamePrefs.showScreenModeDialog = true;
	gGamePrefs.depth				= 32;
	gGamePrefs.screenWidth			= 1024;
	gGamePrefs.screenHeight			= 768;
	gGamePrefs.hz					= 0;
	gGamePrefs.monitorNum			= 0;			// main monitor by default
	gGamePrefs.playerRelControls	= false;
	
	gGamePrefs.lastVersCheckDate.year = 0;
	gGamePrefs.customerHasRegistered = false;
	gGamePrefs.numHTTPReadFails		= 0;

	gGamePrefs.anaglyph				= false;
	gGamePrefs.anaglyphColor		= true;
	gGamePrefs.anaglyphCalibrationRed = DEFAULT_ANAGLYPH_R;
	gGamePrefs.anaglyphCalibrationGreen = DEFAULT_ANAGLYPH_G;
	gGamePrefs.anaglyphCalibrationBlue = DEFAULT_ANAGLYPH_B;
	gGamePrefs.doAnaglyphChannelBalancing = true;
	
	gGamePrefs.dontUseHID			= false;
	
	gGamePrefs.reserved[0] 			= 0;
	gGamePrefs.reserved[1] 			= 0;				
	gGamePrefs.reserved[2] 			= 0;
	gGamePrefs.reserved[3] 			= 0;				
	gGamePrefs.reserved[4] 			= 0;
	gGamePrefs.reserved[5] 			= 0;				
	gGamePrefs.reserved[6] 			= 0;				
	gGamePrefs.reserved[7] 			= 0;				

	for (i = 0; i < MAX_HTTP_NOTES; i++)
		gGamePrefs.didThisNote[i] = false;				
}






#pragma mark -

/******************** PLAY GAME ************************/

static void PlayGame(void)
{	
	if (!gSerialWasVerified)							// check if hackers bypassed the reg verify - if so, de-register us
	{
		FSSpec	spec;
		
		gGameIsRegistered = false;

		if (FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gSerialFileName, &spec) == noErr)	// delete the serial # file
			FSpDelete(&spec);
		ExitToShell();
	}

			/***********************/
			/* GAME INITIALIZATION */
			/***********************/
			
	gDoDeathExit = false;
	
	InitPlayerInfo_Game();					// init player info for entire game
	InitHelpMessages();						// init all help messages


			/*********************************/
			/* PLAY THRU LEVELS SEQUENTIALLY */
			/*********************************/

	if (!gPlayingFromSavedGame)				// start on Level 0 if not loading from saved game
	{
		gLevelNum = 0;
		
#if !DEMO		
		if (GetKeyState(KEY_F10))		// see if do Level cheat
			if (DoLevelCheatDialog())
				CleanQuit();
#endif				
	}

	GammaFadeOut();
		    
	for (;gLevelNum < 10; gLevelNum++)
	{       
				/* DO LEVEL INTRO */
				
		DoLevelIntro();
			        
		MyFlushEvents();
	
	
	        /* LOAD ALL OF THE ART & STUFF */

		InitArea();

		
			/***********/
	        /* PLAY IT */
	        /***********/

		PlayArea();
	    
			/* CLEANUP LEVEL */
						
		MyFlushEvents();
		GammaFadeOut();
		CleanupLevel();
		GameScreenToBlack();	
		
#if DEMO		
		break;
#else
		
			/***************/
			/* SEE IF LOST */
			/***************/
			
		if (gGameOver)									// bail out if game has ended
		{
			DoLoseScreen();
			break;
		}
			
			
		/* DO END-LEVEL BONUS SCREEN */
		
		DoBonusScreen();
#endif	
			
	}
	
#if !DEMO		
	
			/**************/
			/* SEE IF WON */
			/**************/
			
	if (gLevelNum == 10)
	{
		DoWinScreen();	
	}
	
	
			/* DO HIGH SCORES */
			
	NewScore();
#endif	
}




/**************** PLAY AREA ************************/

static void PlayArea(void)
{	

			/* PREP STUFF */

	UpdateInput();
	CalcFramesPerSecond();
	CalcFramesPerSecond();
	gDisableHiccupTimer = true;
	
		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
				/* GET CONTROL INFORMATION FOR THIS FRAME */
				//
				// Also gathers frame rate info for the net clients.
				//

#if DEMO
		gDemoVersionTimer += gFramesPerSecondFrac;							// count the seconds for DEMO
#endif	

						
		UpdateInput();									// read local keys
		
				/* MOVE OBJECTS */
				
		MoveEverything();
		
	
			/* UPDATE THE TERRAIN */

		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);


			/* DRAW IT ALL */

					
		OGL_DrawScene(gGameViewInfoPtr,DrawArea);


						
			/**************/					
			/* MISC STUFF */
			/**************/					
		
			/* SEE IF PAUSED */
		
		if (GetNewKeyState(KEY_ESC))
			DoPaused();
			
		CalcFramesPerSecond();		
		
		if (gGameFrameNum == 0)						// if that was 1st frame, then create a fade event
			MakeFadeEvent(true, 1.0);
				
		gGameFrameNum++;
		
		
				/* SEE IF TRACK IS COMPLETED */

		if (GetNewKeyState(KEY_F10))			//------- win level cheat
			if (GetKeyState(KEY_APPLE))
				break;

		if (gGameOver)													// if we need immediate abort, then bail now
			break;
				
		if (gLevelCompleted)
		{
			gLevelCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gLevelCompletedCoolDownTimer <= 0.0f)
				break;
		}
		
		gDisableHiccupTimer = false;									// reenable this after the 1st frame
		
	}
	
}


/****************** DRAW AREA *******************************/

void DrawArea(OGLSetupOutputType *setupInfo)
{
		/* DRAW OBJECTS & TERAIN */
			
	DrawObjects(setupInfo);												// draw objNodes which includes fences, terrain, etc.


			/* DRAW MISC */

	DrawShards(setupInfo);												// draw shards			
	DrawVaporTrails(setupInfo);											// draw vapor trails
	DrawSparkles(setupInfo);											// draw light sparkles
	DrawInfobar(setupInfo);												// draw infobar last		
	DrawLensFlare(setupInfo);											// draw lens flare
	DrawDeathExit(setupInfo);											// draw death exit stuff


}


/******************** MOVE EVERYTHING ************************/

void MoveEverything(void)
{
float	fps = gFramesPerSecondFrac;

	MoveVaporTrails();
	MoveObjects();
	MoveSplineObjects();
	MoveShards();
	UpdateCamera();								// update camera
	
	/* LEVEL SPECIFIC UPDATES */
	
	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				MO_Object_OffsetUVs(gBG3DGroupList[MODEL_GROUP_LEVELSPECIFIC][SLIME_ObjType_BumperBubble], fps * .3, 0);	// scroll bumper bubble
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_BlobArrow, 0, fps * -.4, 0);	// scroll arrow texture
				break;
				
		case	LEVEL_NUM_BLOBBOSS:
				gSpinningPlatformRot += SPINNING_PLATFORM_SPINSPEED * fps;
				if (gSpinningPlatformRot > PI2)																			// wrap back rather than getting bigger and bigger
					gSpinningPlatformRot -= PI2;

				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Bent, 2,  	0, -fps);			// scroll pipe slime texutre ooze
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop1, 2,  	0, fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop2, 1,  	0, fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight1,1, 0, -fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight2,1, 0, -fps);

				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_TubeSegment,1, 0, -fps);
				break;
								
		case	LEVEL_NUM_CLOUD:
				UpdateZigZagSlats();
				break;

	}
	
}


/***************** INIT AREA ************************/

static void InitArea(void)
{
OGLSetupInputType	viewDef;
DeformationType		defData;



			/* INIT SOME PRELIM STUFF */

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;
	gBestCheckpointNum	= -1;


	gPlayerInfo.objNode = nil;




			/*************/
			/* MAKE VIEW */
			/*************/
				
				

			/* SETUP VIEW DEF */
			
	OGL_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 50;
	viewDef.camera.yon 				= (SUPERTILE_ACTIVE_RANGE * SUPERTILE_SIZE * TERRAIN_POLYGON_SIZE) * .95f;
	viewDef.camera.fov 				= GAME_FOV;
				
	
	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				viewDef.camera.yon				*= .6f;								// bring in the yon
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .8;
				viewDef.view.clearColor.g 		= .6;
				viewDef.view.clearColor.b		= .8;	
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .1f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .95f;				
				gDrawLensFlare = true;				
				break;


		case	LEVEL_NUM_BLOBBOSS:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .17;
				viewDef.view.clearColor.g 		= .05;
				viewDef.view.clearColor.b		= .29;	
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_APOCALYPSE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= 0;
				viewDef.view.clearColor.b		= 0;	
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .15f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.05f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_CLOUD:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .686;
				viewDef.view.clearColor.g 		= .137;
				viewDef.view.clearColor.b		= .431;	
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .3f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .8f;
				gDrawLensFlare = false;
				break;


		case	LEVEL_NUM_JUNGLE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .6;
				viewDef.view.clearColor.g 		= .6;
				viewDef.view.clearColor.b		= .3;	
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_FIREICE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= 0;
				viewDef.view.clearColor.b		= 0;	
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;
	
		case	LEVEL_NUM_SAUCER:
				viewDef.camera.yon				*= .8f;						// bring it in a little for this
				
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .2;
				viewDef.view.clearColor.g 		= .4;
				viewDef.view.clearColor.b		= .7;	
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .2f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_BRAINBOSS:
				viewDef.camera.yon				*= .7f;								// bring in the yon
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .1; //.4;
				viewDef.view.clearColor.g 		= 0; //.7;
				viewDef.view.clearColor.b		= 0; //.2;	
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
				gDrawLensFlare = false;
				break;
	
	
		default:
				viewDef.styles.useFog			= false;
				viewDef.view.clearColor.r 		= .1;
				viewDef.view.clearColor.g 		= .5;
				viewDef.view.clearColor.b		= .1;	
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .8f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
				gDrawLensFlare = true;
	}	
		
		/**************/
		/* SET LIGHTS */
		/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOBBOSS:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.35;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .2;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_APOCALYPSE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.6;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .2;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gApocalypseColor;
				break;



		case	LEVEL_NUM_JUNGLE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.8;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;
				
		case	LEVEL_NUM_JUNGLEBOSS:
				gWorldSunDirection.x 	= .1;
				gWorldSunDirection.y 	= -.5;
				gWorldSunDirection.z 	= -1;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;				

		case	LEVEL_NUM_FIREICE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -1;
				gWorldSunDirection.z 	= 0;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0]			= gFireIceColor;
				break;
				
		case	LEVEL_NUM_SAUCER:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.35;
				gWorldSunDirection.z 	= -.8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .25;
				viewDef.lights.ambientColor.b 		= .25;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_BRAINBOSS:
				gWorldSunDirection.x 	= -.5;
				gWorldSunDirection.y 	= -.7;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;
				
		default:
				
				gWorldSunDirection.x = .5;
				gWorldSunDirection.y = -.35;
				gWorldSunDirection.z = .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;	
				viewDef.lights.ambientColor.r 		= .4;
				viewDef.lights.ambientColor.g 		= .4;
				viewDef.lights.ambientColor.b 		= .36;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
	}


			/*********************/
			/* SET ANAGLYPH INFO */
			/*********************/			
			
	if (gGamePrefs.anaglyph)
	{
		if (!gGamePrefs.anaglyphColor)
		{
			viewDef.lights.ambientColor.r 		+= .1f;					// make a little brighter
			viewDef.lights.ambientColor.g 		+= .1f;
			viewDef.lights.ambientColor.b 		+= .1f;
		}
				
		gAnaglyphFocallength	= 300.0f;
		gAnaglyphEyeSeparation 	= 40.0f;
	}



	
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/**********************/
			/* SET AUTO-FADE INFO */
			/**********************/
			
	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				gAutoFadeStartDist = 0;						// no auto fade here - we gots fog
				break;

		case	LEVEL_NUM_APOCALYPSE:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .75;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * .85f;
				break;

		case	LEVEL_NUM_SAUCER:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .90;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * 1.0f;
				break;
					
		default:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .80;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * .90f;
				
	}
	
	
	if (!gG4)												// bring it in a little for slow machines
	{
		gAutoFadeStartDist *= .85f;
		gAutoFadeEndDist *= .85f;
	}
	
	gAutoFadeRange_Frac	= 1.0f / (gAutoFadeEndDist - gAutoFadeStartDist);

	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	else
		gAutoFadeStatusBits = 0;
		

	
			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			//
			// NOTE: only call this *after* draw context is created!
			//
	
	LoadLevelArt(gGameViewInfoPtr);			
	InitInfobar(gGameViewInfoPtr);
	gAlienSaucer = nil;

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitHumans();
	InitEffects(gGameViewInfoPtr);
	InitVaporTrails();
	InitSparkles();
	InitItemsManager();
	InitSky(gGameViewInfoPtr);

	
	
			/****************/
			/* INIT SPECIAL */
			/****************/
	
	gGravity = NORMAL_GRAVITY;					// assume normal gravity
	gTileSlipperyFactor = 0.0f;					// assume normal slippery
	
	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:

				gTileSlipperyFactor = .1f;

				/* INIT DEFORMATIONS */
				
				if (gG4)										// only do this on a G4 since we need the horsepower
				{
					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 15; 
					defData.radius 				= 0; 
					defData.speed 				= 200; 
					defData.origin.x			= gTerrainUnitWidth; 
					defData.origin.y			= 0; 
					defData.oneOverWaveLength 	= 1.0f / 300.0f;
					NewSuperTileDeformation(&defData);

					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 10; 
					defData.radius 				= 0; 
					defData.speed 				= 300; 
					defData.origin.x			= 0; 
					defData.origin.y			= 0; 
					defData.oneOverWaveLength 	= 1.0f / 200.0f;
					NewSuperTileDeformation(&defData);
				}
				break;
	
		case	LEVEL_NUM_BLOBBOSS:
						
				gGravity = NORMAL_GRAVITY*3/4;						// low gravity
						
					/* MAKE THE BLOB BOSS MACHINE */
					
				MakeBlobBossMachine();
						
				
				if (gG4)										// only do this on a G4 since we need the horsepower
				{
					/* INIT DEFORMATIONS */
					
					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 90; 
					defData.radius 				= 0; 
					defData.speed 				= 900; 
					defData.origin.x			= gTerrainUnitWidth; 
					defData.origin.y			= 0; 
					defData.oneOverWaveLength 	= 1.0f / 300.0f;
					NewSuperTileDeformation(&defData);
					
					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 60; 
					defData.radius 				= 0; 
					defData.speed 				= 600; 
					defData.origin.x			= 0; 
					defData.origin.y			= 0; 
					defData.oneOverWaveLength 	= 1.0f / 400.0f;
					NewSuperTileDeformation(&defData);
				}
				break;

		case	LEVEL_NUM_APOCALYPSE:
				InitZipLines();
				InitTeleporters();
				InitSpacePods();
				break;
				
		case	LEVEL_NUM_CLOUD:
				InitBumperCars();
				break;
					 
		case	LEVEL_NUM_JUNGLEBOSS:
				InitJungleBossStuff();
				break;
				
		case	LEVEL_NUM_FIREICE:
				InitZipLines();
				break;
				
		case	LEVEL_NUM_BRAINBOSS:
				InitBrainBoss();
				break;

	}
		
		/* INIT THE PLAYER & RELATED STUFF */
	
	PrimeWater();								// NOTE:  must do this before items get added since some items may be on the water
	InitPlayersAtStartOfLevel();				// NOTE:  this will also cause the initial items in the start area to be created
			
	PrimeSplines();
	PrimeFences();
						
			/* INIT CAMERAS */
			
	InitCamera();
			
	HideCursor();								// do this again to be sure!
	
	GammaFadeOut();
	
	
 }


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	StopAllEffectChannels();
 	EmptySplineObjectList();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSuperTileMemoryList();
	DisposeTerrain();
	DisposeSky();
	DeleteAllParticleGroups();
	DisposeInfobar();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	DisposeFences();
	
	DisposeAllBG3DContainers();
	
	
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
	
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);	// do this last!			
}




/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


int main(void)
{
unsigned long	someLong;


				/**************/
				/* BOOT STUFF */
				/**************/
				
	ToolBoxInit();

			/* INIT SOME OF MY STUFF */

	InitSpriteManager();
	InitBG3DManager();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();


			/* INIT MORE MY STUFF */
					
	InitObjectManager();
	
	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);
	HideCursor();

			/* SEE IF DEMO VERSION EXPIRED */
			
#if DEMO
	GetDemoTimer();

#else
//	DoWinScreen();	//---------
#endif	



		/* SHOW LEGAL SCREEN */
		
	DoLegalScreen();


		/* MAIN LOOP */
			
	while(true)
	{		
		MyFlushEvents();
		DoMainMenuScreen();
					
		PlayGame();
	}
	
}




