#include "optiontypes.h"


/** @brief get the name of an entry type name
  *
  * This function returns a string, as it is most secure and
  * only really needed for error and/or debugging messages
  * where speed and efficiency are as unimportant as they
  * can get.
  *
  * @param[in] etype The enum entry to get the name of.
  * @return A string with the name or "UNIMPLEMENTED" if a new entry hasn't been
  * added here, yet.
  */
std::string getEntryTypeName(eEntryType etype)
{
	switch (etype)
	{
		case ET_NONE:
			return std::string("ET_NONE");
			break;
		case ET_ACTION:
			return std::string("ET_ACTION");
			break;
		case ET_BUTTON:
			return std::string("ET_BUTTON");
			break;
		case ET_COLOR:
			return std::string("ET_COLOR");
			break;
		case ET_MENU:
			return std::string("ET_MENU");
			break;
		case ET_OPTION:
			return std::string("ET_OPTION");
			break;
		case ET_TEXT:
			return std::string("ET_NONE");
			break;
		case ET_TOGGLE:
			return std::string("ET_TOGGLE");
			break;
		case ET_VALUE:
			return std::string("ET_VALUE");
			break;
		default:
			break;
	}
	return std::string("UNIMPLEMENTED");
}


/** @brief get the name of menu class name
  *
  * This function returns a string, as it is most secure and
  * only really needed for error and/or debugging messages
  * where speed and efficiency are as unimportant as they
  * can get.
  *
  * @param[in] mclass The enum entry to get the name of.
  * @return A string with the name or "UNIMPLEMENTED" if a new entry hasn't been
  * added here, yet.
  */
std::string getMenuClassName(eMenuClass mclass)
{
	switch(mclass)
	{
		case MC_FINANCE:
			return std::string("MC_FINANCE");
			break;
		case MC_GRAPHICS:
			return std::string("MC_GRAPHICS");
			break;
		case MC_MAIN:
			return std::string("MC_MAIN");
			break;
		case MC_NETWORK:
			return std::string("MC_NETWORK");
			break;
		case MC_PHYSICS:
			return std::string("MC_PHYSICS");
			break;
		case MC_PLAY:
			return std::string("MC_PLAY");
			break;
		case MC_PLAYERS:
			return std::string("MC_PLAYERS");
			break;
		case MC_SOUND:
			return std::string("MC_SOUND");
			break;
		case MC_WEATHER:
			return std::string("MC_WEATHER");
			break;
		case MC_MENUCLASS_COUNT:
			return std::string("MC_MENUCLASS_COUNT");
			break;
		default:
			break;
	}
	return std::string("UNIMPLEMENTED");
}


/** @brief get the name of a text class name
  *
  * This function returns a string, as it is most secure and
  * only really needed for error and/or debugging messages
  * where speed and efficiency are as unimportant as they
  * can get.
  *
  * @param[in] tclass The enum entry to get the name of.
  * @return A string with the name or "UNIMPLEMENTED" if a new entry hasn't been
  * added here, yet.
  */
std::string getTextClassName(eTextClass tclass)
{
	switch (tclass)
	{
		case TC_COLOUR:
			return std::string("TC_COLOUR");
			break;
		case TC_LANDSLIDE:
			return std::string("TC_LANDSLIDE");
			break;
		case TC_LANDTYPE:
			return std::string("TC_LANDTYPE");
			break;
		case TC_LANGUAGE:
			return std::string("TC_LANGUAGE");
			break;
		case TC_LIGHTNING:
			return std::string("TC_LIGHTNING");
			break;
		case TC_METEOR:
			return std::string("TC_METEOR");
			break;
		case TC_MOUSE:
			return std::string("TC_MOUSE");
			break;
		case TC_OFFON:
			return std::string("TC_OFFON");
			break;
		case TC_OFFONRANDOM:
			return std::string("TC_OFFONRANDOM");
			break;
		case TC_PLAYERPREF:
			return std::string("TC_PLAYERPREF");
			break;
		case TC_PLAYERTEAM:
			return std::string("TC_PLAYERTEAM");
			break;
		case TC_PLAYERTYPE:
			return std::string("TC_PLAYERTYPE");
			break;
		case TC_SATELLITE:
			return std::string("TC_SATELLITE");
			break;
		case TC_SKIPTYPE:
			return std::string("TC_SKIPTYPE");
			break;
		case TC_SOUNDDRIVER:
			return std::string("TC_SOUNDDRIVER");
			break;
		case TC_TANKTYPE:
			return std::string("TC_TANKTYPE");
			break;
		case TC_TURNTYPE:
			return std::string("TC_TURNTYPE");
			break;
		case TC_WALLTYPE:
			return std::string("TC_WALLTYPE");
			break;
		case TC_TEXTCLASS_COUNT:
			return std::string("TC_TEXTCLASS_COUNT");
			break;
		case TC_FREETEXT:
			return std::string("TC_FREETEXT");
			break;
		case TC_NONE:
			return std::string("TC_NONE");
			break;
		default:
			break;
	}
	return std::string("UNIMPLEMENTED");
}
