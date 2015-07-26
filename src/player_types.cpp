#include "player_types.h"

playerType &operator+=(playerType &src, int32_t val)
{
	int32_t cur = static_cast<int32_t>(src) + val;
	if (cur > 0)
		cur %= LAST_PLAYER_TYPE;
	if (cur < 0)
		cur = LAST_PLAYER_TYPE - ((-1 * cur) % LAST_PLAYER_TYPE);
	src = static_cast<playerType>(cur);
	return src;
}

playerType &operator-=(playerType &src, int32_t val)
{
	return src += -1 * val;
}

playerType &operator++(playerType &src)
{
	return src += 1;
}

playerType operator++(playerType &src, int32_t)
{
	playerType old_val = src;
	src += 1;
	return old_val;
}



playerPrefType &operator+=(playerPrefType &src, int32_t val)
{
	int32_t cur = static_cast<int32_t>(src) + val;
	if (cur > 0)
		cur %= PREF_COUNT;
	if (cur < 0)
		cur = PREF_COUNT - ((-1 * cur) % PREF_COUNT);
	src = static_cast<playerPrefType>(cur);
	return src;
}

playerPrefType &operator-=(playerPrefType &src, int32_t val)
{
	return src += -1 * val;
}

playerPrefType &operator++(playerPrefType &src)
{
	return src += 1;
}

playerPrefType  operator++(playerPrefType &src, int32_t)
{
	playerPrefType old_val = src;
	src += 1;
	return old_val;
}



ePlayerStages &operator+=(ePlayerStages &src, int32_t val)
{
	int32_t cur = static_cast<int32_t>(src) + val;
	if (cur > 0)
		cur %= PS_STAGE_COUNT;
	if (cur < 0)
		cur = PS_STAGE_COUNT - ((-1 * cur) % PS_STAGE_COUNT);
	src = static_cast<ePlayerStages>(cur);
	return src;
}

ePlayerStages &operator-=(ePlayerStages &src, int32_t val)
{
	return src += -1 * val;
}

ePlayerStages &operator++(ePlayerStages &src)
{
	return src += 1;
}

ePlayerStages  operator++(ePlayerStages &src, int32_t)
{
	ePlayerStages old_val = src;
	src += 1;
	return old_val;
}



eTeamTypes &operator+=(eTeamTypes &src, int32_t val)
{
	int32_t cur = static_cast<int32_t>(src) + val;
	if (cur > 0)
		cur %= TEAM_COUNT;
	if (cur < 0)
		cur = TEAM_COUNT - ((-1 * cur) % TEAM_COUNT);
	src = static_cast<eTeamTypes>(cur);
	return src;
}

eTeamTypes &operator-=(eTeamTypes &src, int32_t val)
{
	return src += -1 * val;
}

eTeamTypes &operator++(eTeamTypes &src)
{
	return src += 1;
}

eTeamTypes  operator++(eTeamTypes &src, int32_t)
{
	eTeamTypes old_val = src;
	src += 1;
	return old_val;
}



eTankTypes &operator+=(eTankTypes &src, int32_t val)
{
	int32_t cur = static_cast<int32_t>(src) + val;
	if (cur > 0)
		cur %= TT_TANK_COUNT;
	if (cur < 0)
		cur = TT_TANK_COUNT - ((-1 * cur) % TT_TANK_COUNT);
	src = static_cast<eTankTypes>(cur);
	return src;
}

eTankTypes &operator-=(eTankTypes &src, int32_t val)
{
	return src += -1 * val;
}

eTankTypes &operator++(eTankTypes &src)
{
	return src += 1;
}

eTankTypes  operator++(eTankTypes &src, int32_t)
{
	eTankTypes old_val = src;
	src += 1;
	return old_val;
}

