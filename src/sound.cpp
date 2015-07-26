#include "sound.h"

// max volume factor: means that the interval 0% -> 100% is split in 5
int32_t MAX_VOLUME_FACTOR = 5;

// General helper that unifies the playing
static void play_sound (eSounds sound, int32_t x, int32_t vol, int32_t freq);


/** @brief play a weapon or item fire sample according to @a type, panned using @a x.
  *
  * This just plays what weapon[]/item[] globals hold for a sound.
  *
  * @param[in] type The weapon or item type.
  * @param[in] x The x-coordinate where the sound is played.
  * @param[in] vol The volume (0 - 255)
  * @param[in] freq Frequency, 1000 is normal, 500 is half, 2000 is double and so on.
**/
void play_fire_sound(int32_t type, int32_t x, int32_t vol, int32_t freq)
{
	int32_t sndNum = -1;

	if (type >= WEAPONS) {
		if (item[type - WEAPONS].sound > -1)
			sndNum = item[type - WEAPONS].sound;
	} else {
		if (weapon[type].sound > -1)
			sndNum = weapon[type].sound;
	}

	if ( (sndNum > -1) && (sndNum < SND_COUNT) )
		play_sound(static_cast<eSounds>(sndNum), x, vol, freq);
}


/** @brief play a weapon or item explosion sample according to @a type, panned using @a x.
  *
  * This plays the sound listed in weapon[]/item[] globals unless special rules
  * apply.
  *
  * @param[in] type The weapon or item type.
  * @param[in] x The x-coordinate where the sound is played.
  * @param[in] vol The volume (0 - 255)
  * @param[in] freq Frequency, 1000 is normal, 500 is half, 2000 is double and so on.
**/
void play_explosion_sound(int32_t type, int32_t x, int32_t vol, int32_t freq)
{
	int32_t sndNum = -1;

	if (SHAPED_CHARGE == type)
		sndNum = SND_EXPL_SHAPED_CHARGE;
	else if (WIDE_BOY == type)
		sndNum = SND_EXPL_WIDE_BOY;
	else if (CUTTER == type)
		sndNum = SND_EXPL_CUTTER;
	else if ( (SML_NAPALM <= type) && (LRG_NAPALM >= type)) {
		sndNum = SND_EXPL_NAPALM;
		freq += 333 * (LRG_NAPALM - type);
	} else if (NAPALM_JELLY == type) {
		sndNum = SND_EXPL_NAPALM_BURN;
		freq  += (rand() % 200) - 100;
		vol   -= rand() % 64;
	} else if (PERCENT_BOMB == type)
		sndNum = SND_EXPL_PER_CENT_BOMB;
	else if (REDUCER == type)
		sndNum = SND_EXPL_REDUCER;
	else if ( ((DIRT_BALL   <= type) && (SMALL_DIRT_SPREAD >= type))
	       || ((RIOT_CHARGE <= type) && (RIOT_BLAST        >= type)) )
		sndNum = SND_EXPL_DIRT_BALL_BOMB;
	else {
		// Default handling
		if (type >= WEAPONS)
			sndNum = item[type - WEAPONS].sound + SND_EXPL_MISS_SML;
		else
			sndNum = weapon[type].sound + SND_EXPL_MISS_SML;
	}

	if ( (sndNum > -1) && (sndNum < SND_COUNT) )
		play_sound(static_cast<eSounds>(sndNum), x, vol, freq);
}


/** @brief plays the currently set background music modified by set volume factor
**/
void play_music()
{
    if (env.loadBackgroundMusic())
       play_sound(SND_BG_MUSIC, 128, 255, 1000);
}


/** @brief play a natural sample according to @a type, panned using @a x.
  *
  * @param[in] type The natural type.
  * @param[in] x The x-coordinate where the sound is played.
  * @param[in] vol The volume (0 - 255)
  * @param[in] freq Frequency, 1000 is normal, 500 is half, 2000 is double and so on.
**/

void play_natural_sound(int32_t type, int32_t x, int32_t vol, int32_t freq)
{
	int32_t sndNum = -1;

	if (SML_METEOR == type) {
		sndNum = naturals[SML_METEOR - WEAPONS].sound;
		vol   -= rand() % 128;
		freq  += 100 + (rand() % 100);
	} else if (MED_METEOR == type) {
		sndNum = naturals[MED_METEOR - WEAPONS].sound;
		vol   -= rand() % 64;
		freq  += rand() % 100;
	} else if (LRG_METEOR == type) {
		sndNum = naturals[LRG_METEOR - WEAPONS].sound;
		vol   -= rand() % 64;
		freq  += rand() % 250;
	} else if (SML_LIGHTNING == type) {
		sndNum = naturals[SML_LIGHTNING - WEAPONS].sound;
		vol   -= rand() % 128;
		freq  += 100 + (rand() % 100);
	} else if (MED_LIGHTNING == type) {
		sndNum = naturals[MED_LIGHTNING - WEAPONS].sound;
		vol   -= rand() % 64;
		freq  += rand() % 100;
	} else if (LRG_LIGHTNING == type) {
		sndNum = naturals[LRG_LIGHTNING - WEAPONS].sound;
		vol   -= rand() % 64;
		freq  += rand() % 250;
	} else if (DIRT_FRAGMENT == type) {
		sndNum = SND_NATU_DIRT_FALL;
		vol   -= rand() % 64;
		// freq is manipulated by the call
	}

	if ( (sndNum > -1) && (sndNum < SND_COUNT)
	  && ( (SND_NATU_DIRT_FALL != sndNum)
		|| (global.used_voices < (env.voices - 8))) )
		play_sound(static_cast<eSounds>(sndNum), x, vol, freq);
}


/** @brief play an interface sample according to @a sound.
  *
  * @param[in] sound The sound to play.
**/

void play_interface_sound(eSounds sound)
{
	if (SND_INTE_BUTTON_CLICK == sound)
		play_sound(sound, env.halfWidth, 128, 1000);
}


// Global helpers implementation
static void play_sound(eSounds sound, int32_t x, int32_t vol, int32_t freq)
{
	if (global.used_voices < env.voices) {
		int32_t xVol = (vol * env.volume_factor) / MAX_VOLUME_FACTOR;

		if ( (sound < SND_BG_MUSIC)
		  && env.sounds[sound]
		  && (vol  > 0)
		  && (freq > 0) ) {
			play_sample(env.sounds[sound],
			            xVol,
			            x <= 0 ? 31
			            : x >= env.screenWidth ? 192
			            : 31 + (x * 191 / env.screenWidth),
			            freq, false);
		} else if (SND_BG_MUSIC == sound)
			play_sample(env.background_music, xVol, x, freq, true);

		++global.used_voices;
	}
}
