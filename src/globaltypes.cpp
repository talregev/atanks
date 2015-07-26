#include "globaltypes.h"

/** @file globaltypes.cpp
  * @brief define helper operators for enum rotation
**/

/// @brief pre-increment for eDataStages
eDataStage &operator++ (eDataStage &ds)
{
	if (DS_DATA == ds)
		ds = DS_NAME;
	else if (DS_DESC == ds)
		ds = DS_DATA;
	else
		ds = DS_DESC;
	return ds;
}


eLanguages &operator+=(eLanguages &lang, int32_t val)
{
	int32_t cur = static_cast<int32_t>(lang) + val;
	if (cur > 0)
		cur %= EL_LANGUAGE_COUNT;
	if (cur < 0)
		cur = EL_LANGUAGE_COUNT - ((-1 * cur) % EL_LANGUAGE_COUNT);
	lang = static_cast<eLanguages>(cur);
	return lang;
}


eLanguages &operator-=(eLanguages &lang, int32_t val)
{
	return lang += -1 * val;
}


/// @brief pre-increment for the language
eLanguages &operator++(eLanguages &lang)
{
	return lang += 1;
}


/// @brief post-increment for the language type
eLanguages operator++(eLanguages &lang, int)
{
	eLanguages tmp = lang;
	lang += 1;
	return tmp;
}


/// @brief pre-decrement for the language
eLanguages &operator--(eLanguages &lang)
{
	return lang += -1;
}


/// @brief post-decrement for the language type
eLanguages operator--(eLanguages &lang, int)
{
	eLanguages tmp = lang;
	lang += -1;
	return tmp;
}

