// OTTO MATIC ENTRY POINT
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "version.h"

#include <SDL.h>

//#include "gamepatches.h"

#include <iostream>
#include <cstring>

#if __APPLE__
#include "killmacmouseacceleration.h"
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	SDL_Window* gSDLWindow;

	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if _WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

	int GameMain(void);
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System" / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);
	UseResFile(resFileRefNum);

	return dataPath;
}

static const char* GetWindowTitle()
{
	static char windowTitle[256];
	snprintf(windowTitle, sizeof(windowTitle), "Otto Matic %s", PROJECT_VERSION);
	return windowTitle;
}

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init();

	// Uncomment to dump the game's resources to a temporary directory.
//	Pomme_StartDumpingResources("/tmp/OttoRezDump");

	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
//#ifndef _WIN32
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
//#endif

//	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
//	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaSamples);

	gSDLWindow = SDL_CreateWindow(
			"Otto Matic",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280,
			720,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

	if (!gSDLWindow)
	{
		throw std::runtime_error("Couldn't create SDL window.");
	}

	fs::path dataPath = FindGameData();
#if !(__APPLE__)
	Pomme::Graphics::SetWindowIconFromIcl8Resource(gSDLWindow, 500);
#endif

	// Init joystick subsystem
	/*
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Otto Matic", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}
	 */

	// Start the game
	try
	{
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	Pomme::Shutdown();

	SDL_DestroyWindow(gSDLWindow);
	gSDLWindow = nullptr;

	return 0;
}

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

#if _DEBUG
	// In debug builds, if CommonMain throws, don't catch.
	// This way, it's easier to get a clean stack trace.
	returnCode = CommonMain(argc, const_cast<const char**>(argv));
#else
	// In release builds, catch anything that might be thrown by CommonMain
	// so we can show an error dialog to the user.
	try
	{
		returnCode = CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)		// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = ex.what();
		showFinalErrorMessage = true;
	}
	catch (...)						// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = "unknown";
		showFinalErrorMessage = true;
	}
#endif

#if __APPLE__
	// Whether we failed or succeeded, always restore the user's mouse acceleration before exiting.
	// (NOTE: in debug builds, we might not get here because we don't catch what CommonMain throws.)
	RestoreMacMouseAcceleration();
#endif

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Uncaught exception", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}