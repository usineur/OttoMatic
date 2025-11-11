#include "nx.h"

#ifdef __SWITCH__
#include <switch.h>
#include <sys/syslimits.h>

long pathconf(const char *path, int name)
{
	return NAME_MAX;
}

GameLanguageID GetSwitchBestLanguage(void)
{
	GameLanguageID languageID = LANGUAGE_ENGLISH;

	u64 languageCode = 0;
	SetLanguage language = SetLanguage_ENUS;

	setInitialize();
	setGetSystemLanguage(&languageCode);
	setMakeLanguage(languageCode, &language);

	switch (language) {
		case SetLanguage_FR:
			languageID = LANGUAGE_FRENCH;
			break;
		case SetLanguage_DE:
			languageID = LANGUAGE_GERMAN;
			break;
		case SetLanguage_ES:
			languageID = LANGUAGE_SPANISH;
			break;
		case SetLanguage_IT:
			languageID = LANGUAGE_ITALIAN;
			break;
		default:
			languageID = LANGUAGE_ENGLISH;
			break;
	}

	return languageID;
}
#endif
