#include "shop.h"
#include "player.h"
#include "files.h"
#include "gameloop.h" // For the LevelCreator declaration.
#include "text.h" // for draw_text_in_box()

#define SHOP_BAR_HEIGHT 29


/// ==== helper functions ====
static int32_t calcPotentialDmg (int32_t weapNum);
static void    divide_team_money();
static void    do_ai_shopping   (PLAYER* pl, int32_t maxBoost, int32_t maxScore);
static void    draw_shop        (PLAYER* pl);
static void    draw_weapon_list (PLAYER* pl, int32_t* trolley,
                                 int32_t scroll_old, int32_t scroll_new,
                                 int32_t over_old, int32_t over_new);

/// ==== Helper values ====
static int32_t btps = 0;


/// ==== External functions used ====
void draw_simple_bg(bool drawImage);

bool shop(LevelCreator* lvl_creator)
{
	bool    performed_save_game = false;
	char    buf[50]             = { 0 };
	char    description[1024]   = { 0x20, 0x0 };
	const
	int32_t scrollArrowPos      = env.screenWidth - STUFF_BAR_WIDTH - 30;

	draw_shop (nullptr);

	// Determine btps:
	btps = ROUNDu((env.screenHeight - SHOP_BAR_HEIGHT) / STUFF_BAR_HEIGHT);

	// Init global for drawing the shop:
	global.do_updates();
	global.stopwindow = true;

	// before we do anything else, put a cap on money
	for (int32_t z = 0; z < env.numGamePlayers; ++z) {
		if (env.players[z]->money > 1000000000)
			env.players[z]->money = 1000000000;
		if (env.players[z]->money < 0)
			env.players[z]->money = 0;
	}


	if (env.isGameLoaded)
		// after the first shopping loop the game isn't fresh any more
		env.isGameLoaded = false;
	else
		// Money dividing within the non-neutral teams only happens if no game
		// was loaded. Game saving is done after that rounds money dividing.
		divide_team_money();


	// Determine maximum boost value and score
	int32_t maxBoost = 0;
	int32_t maxScore = 0;
	for (int32_t z = 0; z < env.numGamePlayers;++z) {
		int32_t boostValue = env.players[z]->getBoostValue();
		if (boostValue > maxBoost)
			maxBoost = boostValue;
		if (env.players[z]->score > maxScore)
			maxScore = env.players[z]->score;
	}

	// If this is demo mode, raise the max boost level, as there
	// are no human players to define a maximum value
	if (global.demo_mode)
		maxBoost += env.rounds - global.currentround;

	// Loop all players to let them do their shopping
	for (int32_t pl = 0; pl < env.numGamePlayers; pl++) {
		// computer players have their own function for their shopping
		if ( HUMAN_PLAYER != env.players[pl]->type ) {
			do_ai_shopping(env.players[pl], maxBoost, maxScore);
			continue; // next one.
		}

		// Be sure no input from previous human players or from pressing
		// the "Play" button on the player selection screen carry over:
		flush_inputs();

		int32_t money           = env.players[pl]->money;
		int32_t trolley[THINGS] = { 0 };
		bool    done            = false;
		bool    need_draw       = false;
		int32_t scroll          = 1;
		int32_t scroll_old      = -1;
		int32_t pressed         = -1;
		int32_t prev_wheel      = mouse_z;
		int32_t curr_wheel      = 0;
		int32_t lastMouse_b     = 0;
		int32_t lastMouse_x     = 0;
		int32_t lastMouse_y     = 0;
		int32_t lb              = 0;
		int32_t itemindex       = 1;
		int32_t hoverOver       = -1;
		int32_t hoverOver_old   = -1;
		int32_t cost, amt, inInv; // short cuts
		BOX     area(20, 60, 300, 400);

		env.mouseclock = 0;

		draw_shop (env.players[pl]);

		while (!done) {
			while (!done && !need_draw) {
				if (global.isCloseBtnPressed()) {
					done = true;
					continue;
				}

				if ( (lastMouse_x != mouse_x) || (lastMouse_y != mouse_y) ) {
					lastMouse_x = mouse_x;
					lastMouse_y = mouse_y;
					if (!env.osMouse)
						need_draw = true;
				}

				int32_t newlyOver = -1;

				// Check mouse button
				if (!lb && (mouse_b & 1)) {
					// Check close shop button:
					if ( (mouse_x >= (env.halfWidth - 100))
					  && (mouse_x <  (env.halfWidth + 100))
					  && (mouse_y >= (env.screenHeight - 50))
					  && (mouse_y <  (env.screenHeight - 25)) )
						done = true;
					env.mouseclock = 0;
				}
				lb = (mouse_b & 1) ? 1 : 0;

				/* ========================
				 * === Keyboard control ===
				 * ========================
				 */
				if ( keypressed() ) {
					k = readkey();
					K = k >> 8;
				} else
					k = K = 0;

				// Move up the list
				if  ( (K == KEY_UP) || (K == KEY_W) ) {
					if (itemindex > 1)
						itemindex--;
					if (itemindex < scroll)
						scroll = itemindex;
					need_draw = true;
				} else if ( (K == KEY_PGUP) || (K == KEY_R) ) {
					itemindex -= btps;
					if (itemindex < 1)
						itemindex = 1;
					if (itemindex < scroll)
						scroll = itemindex;
					need_draw = true;
				}

				// Move down the list
				else if ( (K == KEY_DOWN) || (K == KEY_S) ) {
					if (itemindex < (env.numAvailable - 1))
						itemindex++;
					if ( (itemindex - scroll) >= btps )
						scroll = itemindex - (btps - 1);
					need_draw = true;
				} else if ( ( (K == KEY_PGDN) || (K == KEY_F) )
				         && (scroll <= (env.numAvailable - btps) ) ) {
					itemindex += btps;
					if (itemindex > env.numAvailable - 1)
						itemindex = env.numAvailable - 1;
					if ( (itemindex - scroll) >= btps)
						scroll = itemindex - (btps - 1);
					need_draw = true;
				}

				// make sure the selected item is on the visible screen
				if (itemindex < scroll)
					itemindex = scroll;
				else if ( itemindex >= (scroll + btps) )
					itemindex = scroll + btps - 1;

				// buy or sell an item
				if ( (K == KEY_RIGHT) || (K == KEY_D) ) {
					pressed = env.availableItems[itemindex];
					if (pressed >= WEAPONS) {
						cost  = item[pressed - WEAPONS].cost;
						amt   = item[pressed - WEAPONS].amt;
						inInv = env.players[pl]->ni[pressed - WEAPONS];
					} else {
						cost  = weapon[pressed].cost;
						amt   = weapon[pressed].amt;
						inInv = env.players[pl]->nm[pressed];
					}

					if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
						cost *= 10;
						amt  *= 10;
					}

					if ( (money >= cost)
					  && ( (inInv + trolley[pressed]) < (MAX_ITEMS_IN_STOCK - amt)) ) {
						if (trolley[pressed] <= -amt) {
							if (env.sellpercent > 0.01) {
								money -= ROUNDu(cost * env.sellpercent);
								trolley[pressed] += amt;
								need_draw = true;
							}
						} else {
							money -= cost;
							trolley[pressed] += amt;
							need_draw = true;
							if (inInv + trolley[pressed] > MAX_ITEMS_IN_STOCK)
								trolley[pressed] = MAX_ITEMS_IN_STOCK;
						}
					}
					pressed = -1;
				} // end of buying

				else if ( (K == KEY_LEFT) || (K == KEY_A) ) {
					pressed = env.availableItems[itemindex];
					if (pressed >= WEAPONS) {
						cost  = item[pressed - WEAPONS].cost;
						amt   = item[pressed - WEAPONS].amt;
						inInv = env.players[pl]->ni[pressed - WEAPONS];
					} else {
						cost  = weapon[pressed].cost;
						amt   = weapon[pressed].amt;
						inInv = env.players[pl]->nm[pressed];
					}

					if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
						cost *= 10;
						amt  *= 10;
					}

					if (inInv + trolley[pressed] >= amt) {
						if (trolley[pressed] >= amt) {
							money += cost;
							trolley[pressed] -= amt;
							need_draw = true;
						} else {
							if (env.sellpercent > 0.01) {
								money += ROUNDu(cost * env.sellpercent);
								trolley[pressed] -= amt;
								need_draw = true;
							}
						}
					}
					pressed = -1;
				} // end of selling


				// check for adding or removing rounds
				else if ( (K == KEY_PLUS_PAD) || (K == KEY_EQUALS) ) {
					if ( (env.rounds < MAX_ROUNDS) && (! env.mouseclock) ) {
						env.rounds++;
						global.currentround++;
						need_draw = true;
					}
				} else if ( (K == KEY_MINUS_PAD) || (K == KEY_MINUS) ) {
					if ( (env.rounds > 1)
					  && (global.currentround > 1)
					  && (! env.mouseclock) ) {
						env.rounds--;
						global.currentround--;
						need_draw = true;
					}
				}

				// check for saving the game
				else if ( K == KEY_F10 ) {
					if (!performed_save_game
					  && Save_Game() )
						performed_save_game = true;
					if (performed_save_game)
						snprintf(description, 64,
						         "%s \"%s\".",
						         env.ingame->Get_Line(17),
						         env.game_name);
					else
						strncpy(description, env.ingame->Get_Line(41), 1023);
					draw_text_in_box (&area, description, true);
					need_draw = true;
				}

				// Keyboard exit shop:
				if (K == KEY_ENTER)
					done = true;

				/* ========================
				 * === Mouse control    ===
				 * ========================
				 */

				// check mouse wheel
				curr_wheel = mouse_z;
				if (curr_wheel < prev_wheel) {
					if (++scroll >= (env.numAvailable - btps) )
						scroll = env.numAvailable - btps;
					if (scroll > itemindex)
						itemindex = scroll;
					need_draw = true;
				} else if (curr_wheel > prev_wheel) {
					if (--scroll < 1)
						scroll = 1;
					if (itemindex >= (scroll + btps) )
						itemindex = scroll + btps - 1;
					need_draw = true;
				}
				prev_wheel = curr_wheel;

				// Ensure the description shows what is selected
				newlyOver = env.availableItems[itemindex];

				// check mouse over items
				if ( (mouse_x >= (env.screenWidth - STUFF_BAR_WIDTH))
				  && (mouse_x <   env.screenWidth) ) {
					bool    isOver    = false;
					int32_t zzz       = scroll;

					for (int32_t z = 1; (z <= btps) && !isOver; ++z) {
						if ( (mouse_y >= (  z * STUFF_BAR_HEIGHT))
						  && (mouse_y <  ( (z * STUFF_BAR_HEIGHT) + 30) ) )
							isOver = true;
						else
							++zzz;
					}

					if (isOver && (hoverOver != env.availableItems[zzz])) {
						newlyOver = env.availableItems[zzz];
						itemindex = zzz;
						need_draw = true;
					}
				} // End of mouse_x in stuff bar

				// Switch description if necessary
				if (hoverOver != newlyOver) {
					if (newlyOver > -1) {
						if (newlyOver < WEAPONS) {
							WEAPON *weap = &weapon[newlyOver];
							snprintf (description, 1023,
									  "Radius: %d\nYield: %d\n\n%s",
									  weap->radius,
									  calcPotentialDmg (newlyOver) * weap->spread,
									  weap->getDesc());
						} else {
							int32_t itemNum = newlyOver - WEAPONS;
							ITEM *it = &item[itemNum];
							if ( (itemNum >= ITEM_VENGEANCE)
							  && (itemNum <= ITEM_FATAL_FURY) ) {
								double potDmg = calcPotentialDmg(it->vals[0])
											  * it->vals[1];
								snprintf (description, 1023,
										  "Potential Damage: %d\n\n%s",
										  ROUND(potDmg), it->getDesc());
							} else
								snprintf (description, 1023, "%s",
										  it->getDesc());
						}
					} else
						description[0] = 0;
					hoverOver     = newlyOver;
					need_draw     = true;

					draw_text_in_box (&area, description, true);
				} // end of hovering on a different item

				// Check mouse buttons against scrolling, buying and selling.
				if ( (mouse_b & 1) && !env.mouseclock) {
					if ( (mouse_x >=  scrollArrowPos)
					  && (mouse_x <  (scrollArrowPos + 24)) ) {

						// Fast up
						if ( (mouse_y >= (env.halfHeight - 50))
						  && (mouse_y <  (env.halfHeight - 25))
						  && (scroll > 1) ) {
							scroll -= btps / 2;
							if (scroll < 1)
								scroll = 1;
							need_draw = true;
						}

						// Up one item
						if ( (mouse_y >= (env.halfHeight - 24))
						  && (mouse_y <   env.halfHeight)
						  && (scroll > 1) ) {
							--scroll;
							need_draw = true;
						}

						// Down one item
						if ( (mouse_y >= (env.halfHeight + 1))
						  && (mouse_y <  (env.halfHeight + 25))
						  && (scroll  <  (env.numAvailable - btps)) ) {
							++scroll;
							need_draw = true;
						}

						// Fast down
						if ( (mouse_y >= (env.halfHeight + 25))
						  && (mouse_y <  (env.halfHeight + 50))
						  && (scroll  <  (env.numAvailable)) ) {
							scroll += btps / 2;
							if (scroll >= env.numAvailable - btps)
								scroll = env.numAvailable - btps;
							need_draw = true;
						}
					}
					if (itemindex < scroll)
						itemindex = scroll;
					else if ( itemindex > (scroll + btps) )
						itemindex = scroll + btps - 1;
                }

                // Check mouse buttons for clicks
				if ( ( (mouse_b & 1) || (mouse_b & 2) )
				  && (mouse_x >= (env.screenWidth - STUFF_BAR_WIDTH))
				  && (mouse_x <   env.screenWidth) )
					pressed = env.availableItems[itemindex];

				// Only do the buying / selling when the mouse button is
				// released. This way users can pull the mouse off the item
				if ( (pressed > -1) && !((mouse_b & 1) || (mouse_b & 2)) ) {
					if (pressed < WEAPONS) {
						cost  = weapon[pressed].cost;
						amt   = weapon[pressed].amt;
						inInv = env.players[pl]->nm[pressed];
					} else {
						cost  = item[pressed - WEAPONS].cost;
						amt   = item[pressed - WEAPONS].amt;
						inInv = env.players[pl]->ni[pressed - WEAPONS];
					}

					if (key[KEY_LCONTROL] || key[KEY_RCONTROL]) {
						cost *= 10;
						amt  *= 10;
					}

					// RMB sells items and takes precedence over LMB
					if (lastMouse_b & 2) {
						if ( (inInv + trolley[pressed]) >= amt) {
							if (trolley[pressed] >= amt) {
								money += cost;
								trolley[pressed] -= amt;
								need_draw = true;
							} else if (env.sellpercent > 0.01) {
								money += ROUNDu(cost * env.sellpercent);
								trolley[pressed] -= amt;
								need_draw = true;
							}
						}
					} else if ( (money >= cost)
						     && ( (inInv + trolley[pressed])
					            < (MAX_ITEMS_IN_STOCK - amt)) ) {
						if (trolley[pressed] <= -amt) {
							if (env.sellpercent > 0.01) {
								money -= ROUNDu(cost * env.sellpercent);
								trolley[pressed] += amt;
								need_draw = true;
							}
						} else {
							money -= cost;
							trolley[pressed] += amt;
							need_draw = true;
							if ( (inInv + trolley[pressed]) > MAX_ITEMS_IN_STOCK)
								trolley[pressed] = MAX_ITEMS_IN_STOCK;
						}
					}
					pressed = -1;
				} // End of mouse buttons released with item selected
				env.mouseclock++;
				if (env.mouseclock > 5)
					env.mouseclock = 0;
				lastMouse_b = mouse_b;

				// Sleep a bit if nothing happened
				if (!done && !need_draw)
					LINUX_SLEEP
			} // End of input handling loop

			// Update display if anything happened:
			if (need_draw) {
				need_draw = false;

				// No hardware mouse while drawing
				SHOW_MOUSE(nullptr)

				global.make_update(env.halfWidth - 200, 0,
				                   env.gfxData.stuff_bar[0]->w,
				                   env.gfxData.stuff_bar[0]->h);

				draw_sprite (global.canvas, env.gfxData.stuff_bar[0],
							 env.halfWidth - 200, 0);
				textprintf_ex (global.canvas, font, env.halfWidth - 190, 0, BLACK,
							   -1, "%s %d: %s", env.ingame->Get_Line(10), pl + 1,
							   env.players[pl]->getName ());
				textprintf_ex (global.canvas, font, env.halfWidth - 190, 14, BLACK,
							   -1, "%s: $%s", env.ingame->Get_Line(11),
							   Add_Comma(money));
				snprintf (buf, 49, "%s: %d/%d", env.ingame->Get_Line(12),
						  env.rounds - global.currentround, env.rounds);
				textout_ex (global.canvas, font, buf,
							env.halfWidth + 170 - text_length(font, buf),
							0, BLACK, -1);
				snprintf (buf, 49, "%s: %d", env.ingame->Get_Line(13),
						  env.players[pl]->score);
				textout_ex (global.canvas, font, buf,
							env.halfWidth + 155 - text_length(font, buf),
							14, BLACK, -1);

				draw_weapon_list(env.players[pl], trolley, scroll_old, scroll,
				                 hoverOver_old, hoverOver);

				// Update non-OS mouse movements
				SHOW_MOUSE(global.canvas)

				global.do_updates ();
				hoverOver_old = hoverOver;
				scroll_old    = scroll;
			} // End of drawing
		} // End of player shopping loop

		// Now write back bought/sold items and remaining money
		for (int tItem = 0; tItem < WEAPONS; tItem++)
			env.players[pl]->nm[tItem] += trolley[tItem];
		for (int tItem = WEAPONS; tItem < THINGS; tItem++)
			env.players[pl]->ni[tItem - WEAPONS] += trolley[tItem];
		env.players[pl]->money = money;
	} // End of player handling

	// Eventually give all players interest on their remaining money
	if (!global.isCloseBtnPressed()) {
		for (int32_t z = 0; z < env.numGamePlayers; z++) {
			int32_t money    = env.players[z]->money;
			int32_t interest = 0;
			double  intPerc  = .0;
			int32_t intLevel = 0;
			int32_t intSum   = 0; // The summed up interest
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "======================================================", 0)
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "%2d.: %s enters the bank to get interest:", (z+1),
			              env.players[z]->getName())
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "     Starting Account: %10d", env.players[z]->money)
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "------------------------------------------------------", 0)
			while (money && (intLevel++ < 5)) {
				// Enter next level
				intPerc  = (env.interest - 1.0) / intLevel;
				interest = static_cast<double>(money) * intPerc;

				// The limit is only applicable on the first four levels,
				// in the fifth level interest is fully applied!
				if ((interest > MAX_INTEREST_AMOUNT) && (intLevel < 5))
					interest = MAX_INTEREST_AMOUNT;

				// Now sum the interest up and substract the counted money!
				intSum += interest;
				money  -= static_cast<double>(interest) / intPerc;

				DEBUG_LOG_FIN(env.players[z]->getName(),
				              "     Level %1d:  %8d credits are rated,",
				              intLevel,
				              static_cast<int32_t>(interest / intPerc))
				DEBUG_LOG_FIN(env.players[z]->getName(),
				              "     Interest: %8d credits. (%5.2f%%)",
				              interest, intPerc * 100.)

				// To get rid of (possible) rounding errors, add a security check:
				if ( (money < (4 * intLevel)) || (interest < 1) )
					money = 0; // With less there won't be any more interest anyway!

				DEBUG_LOG_FIN(env.players[z]->getName(),
				              "     Unrated : %8d credits left.", money)
			}

			// Now give them their money:
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "     Sum:      %8d credits.", intSum)
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "------------------------------------------------------", 0)
			env.players[z]->money += intSum;
			DEBUG_LOG_FIN(env.players[z]->getName(),
			              "     Final Account   : %10d", env.players[z]->money)
		} // End of looping players
	} // End of close button not pressed


	// If the close button was pressed, the creator thread must end
	// ASAP so we can get out of here.
	if (global.isCloseBtnPressed())
		lvl_creator->die_now();

	// The LevelCreator, if not finished, can work alone, now:
	if (!lvl_creator->is_finished())
		lvl_creator->work_alone();

	// Wait until the level creator is done
	while (!lvl_creator->is_finished()) {
		MSLEEP(20)
		if ( lvl_creator->has_progress() ) {
			// Hide custom mouse pointer
			SHOW_MOUSE(nullptr)

			lvl_creator->print_state();

			// Draw custom mouse cursor
			SHOW_MOUSE(global.canvas)

			global.do_updates();
		}
	}

	return !global.isCloseBtnPressed();
}


/*
 *  Calculate the potential damage for a given weapon.
 *  Recursively add the damage of sub-munitions.
 */
static int32_t calcPotentialDmg (int32_t weapNum)
{
	WEAPON *weap = &weapon[weapNum];
	int32_t total = 0;

	if ( (weap->submunition >= 0) && (weap->numSubmunitions > 0) )
		total += calcPotentialDmg (weap->submunition)
		       * weap->numSubmunitions;
	else
		total += weap->damage;

	return total;
}


// If configured to do so, this method divides team money to help out team mates
static void divide_team_money()
{
	if (!env.divide_money)
		return;

	int32_t jediMoney = 0;
	int32_t jediCount = 0;
	int32_t sithMoney = 0;
	int32_t sithCount = 0;
	int32_t teamFee   = 0;

	for (int32_t z = 0; z < env.numGamePlayers; ++z) {
		// Sum up team money:
		if (env.players[z]->team == TEAM_JEDI) {
			teamFee = static_cast<double>(env.players[z]->money) / 4.;
			if (teamFee > MAX_TEAM_AMOUNT)
				teamFee = MAX_TEAM_AMOUNT;
			jediMoney += teamFee;
			jediCount++;
		} else  if (env.players[z]->team == TEAM_SITH) {
			teamFee = static_cast<double>(env.players[z]->money) / 4;
			if (teamFee > MAX_TEAM_AMOUNT)
				teamFee = MAX_TEAM_AMOUNT;
			sithMoney += teamFee;
			sithCount++;
		}
		// Note: The team Fee is not docked, yet, as it is not clear
		// whether there is more than one team member.
	}

	DEBUG_LOG_FIN("Overview", "Jedi Count: %d - Sith Count: %d", jediCount, sithCount)

	// Now apply the team money (if any):
	if (jediCount > 1) {
		DEBUG_LOG_FIN("Overview",
					  "The Jedi summed up a pool of %13d credits!",
					  jediMoney)
		jediMoney = static_cast<double>(jediMoney) * .90 / jediCount;
		DEBUG_LOG_FIN("Overview",
					  "Every Jedi will receive %10d credits out of the pool!",
					  jediMoney)
		for (int32_t z = 0; z < env.numGamePlayers; ++z) {
			if (TEAM_JEDI == env.players[z]->team) {
				teamFee = static_cast<double>(env.players[z]->money) / 4;
				if (teamFee > MAX_TEAM_AMOUNT)
					teamFee = MAX_TEAM_AMOUNT;
				env.players[z]->money -= teamFee;
				env.players[z]->money += jediMoney;
			}
		}
	}

	if (sithCount > 1) {
		DEBUG_LOG_FIN("Overview",
					  "The Sith summed up a pool of %13d credits!",
					  sithMoney)
		sithMoney = static_cast<double>(sithMoney) * .90 / sithCount;
		DEBUG_LOG_FIN("Overview",
					  "Every Sith will receive %10d credits out of the pool!",
					  sithMoney)
		for (int32_t z = 0; z < env.numGamePlayers; ++z) {
			if (TEAM_SITH == env.players[z]->team) {
				teamFee = static_cast<double>(env.players[z]->money) / 4;
				if (teamFee > MAX_TEAM_AMOUNT)
					teamFee = MAX_TEAM_AMOUNT;
				env.players[z]->money -= teamFee;
				env.players[z]->money += sithMoney;
			}
		}
	}
}


/// @brief dedicated function for AI shopping.
void do_ai_shopping(PLAYER* player, int32_t maxBoost, int32_t maxScore)
{
	// Print player info and inventory
#ifdef ATANKS_DEBUG_FINANCE
	DEBUG_LOG_FIN(player->getName(),
	              "Starting to buy: (Defensiveness: %4.2lf)",
	              player->defensive)


	DEBUG_LOG_FIN(player->getName(), " --- Inventory --- ", 0)
	DEBUG_LOG_FIN(player->getName(), "-------------------", 0)
	for (int32_t i = 1; i < WEAPONS; ++i) {
		if (player->nm[i])
			DEBUG_LOG_FIN(player->getName(), "% 4d x %s",
			              player->nm[i] / weapon[i].getDelayDiv(),
			              weapon[i].getName())
	}
	DEBUG_LOG_FIN(player->getName(), " - - - - - - - - - ", 0)
	for (int32_t i = 1; i < ITEMS; ++i) {
		if (player->ni[i])
			DEBUG_LOG_FIN(player->getName(), "% 4d x %s",
			              player->ni[i], item[i].getName())
	}
	DEBUG_LOG_FIN(player->getName(), "-------------------", 0)

	int32_t oldMoneyToSave = -1; // So the same message isn't repeated over and over again.
#endif // ATANKS_DEBUG_FINANCE

	player->updatePreferences(maxBoost, maxScore);

	// money saving will be made possible when:
	// 1. It's not the first three rounds
	// 2. It's not the last 5 rounds
	// and, dynamically:
	// 3. We have at least 10 parachutes or no gravity
	// 4. We have at least 2 damage dealing (not small missile) weapon
	//    per AI level.

	// Check for a minimum of damage dealing weapons and parachutes,
	// then buy until 'moneyToSave' is reached.
	int32_t pressed      = -1;
	int32_t ai_level     = static_cast<int32_t>(player->type);
	int32_t buy_count    = 0;
	int32_t last_buy_idx = 0; // Used to "remember" where the AI was in its cart.
	do {
		int32_t moneyToSave = 0; // How much money will the player save?

		// The AI does not save up money in the first three or last five rounds
		if ( (global.currentround > 5)
		  && ((env.rounds - global.currentround) > 3) ) {
			moneyToSave = player->getMoneyToSave(!buy_count);
#ifdef ATANKS_DEBUG_FINANCE
			if (oldMoneyToSave != moneyToSave) {
				DEBUG_LOG_FIN(player->getName(),
				              "Maximum Money to save: %d (I have %d)",
				              moneyToSave, player->money)
				oldMoneyToSave = moneyToSave;
			}
		} else
			DEBUG_LOG_FIN(player->getName(),
			              "No money to save this round!", 0);
#else
		}
#endif // ATANKS_DEBUG_FINANCE

		int32_t numPara     = player->ni[ITEM_PARACHUTE];
		int32_t numDmgWeaps = 0;

		for (int32_t i = 1; i < WEAPONS; ++i) {
			// start from 1, as 0 is the small missile
			if (weapon[i].damage > 0)
				numDmgWeaps += player->nm[i] / weapon[i].getDelayDiv();
		}

		// Try to chose something to buy if enough money is there or either
		// the number of parachutes or damage dealing weapons is too low.
		if ( (player->money > moneyToSave)
		  || ( (numPara   < ai_level) && (env.landSlideType > SLIDE_NONE) )
		  || (numDmgWeaps <  (ai_level * 2)) )
			pressed = player->chooseItemToBuy(maxBoost, last_buy_idx);
		else
			pressed = -1; // Forced to end.

		DEBUG_LOG_FIN(player->getName(),
		              "I have %s%s%s%d credits left%s",
		              pressed > -1 ? "bought: " : "finished, with ",
		              pressed > -1
		              ? pressed < WEAPONS
		                ? weapon[pressed].getName()
		                : item[pressed - WEAPONS].getName()
		              : "",
		              pressed < 0 ? " " : " (",
		              player->money,
		              pressed < 0 ? "" : ")")
		buy_count++;
	} while ( (pressed != -1) && (buy_count < 1000) );

	DEBUG_LOG_FIN(player->getName(),
	              "============================================", 0)
}


static void draw_shop(PLAYER *pl)
{
	global.make_update (0, 0, env.screenWidth, env.screenHeight);
	global.lockLand();
	SHOW_MOUSE(nullptr)
	draw_simple_bg(false);

	if (pl) {
		draw_sprite (global.canvas, env.misc[DONE_IMAGE],
					 env.halfWidth - 100, env.screenHeight - 50);
		draw_sprite (global.canvas, env.misc[FAST_UP_ARROW_IMAGE],
					 env.screenWidth - STUFF_BAR_WIDTH - 30, env.halfHeight - 50);
		draw_sprite (global.canvas, env.misc[UP_ARROW_IMAGE],
					 env.screenWidth - STUFF_BAR_WIDTH - 30, env.halfHeight - 25);
		draw_sprite (global.canvas, env.misc[DOWN_ARROW_IMAGE],
					 env.screenWidth - STUFF_BAR_WIDTH - 30, env.halfHeight);
		draw_sprite (global.canvas, env.misc[FAST_DOWN_ARROW_IMAGE],
					 env.screenWidth - STUFF_BAR_WIDTH - 30, env.halfHeight + 25);
	}

	drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
	global.current_drawing_mode = DRAW_MODE_TRANS;

	if (pl) {
		double  left  = env.halfWidth - 200; // short cut
		int32_t right = env.screenWidth - 1; // another short cut.

		for (int32_t z = 0; z < env.halfWidth - 200; z++) {
			set_trans_blender (0, 0, 0, ROUNDu(static_cast<double>(z) / left * 240) + 15);
			vline (global.canvas, z,         0, SHOP_BAR_HEIGHT, pl->color);
			vline (global.canvas, right - z, 0, SHOP_BAR_HEIGHT, pl->color);
		} // End of drawing player colour blending
	} // End of having a player

	solid_mode ();
	global.current_drawing_mode = DRAW_MODE_SOLID;

	if (pl) {
		textout_ex(global.canvas, font, env.ingame->Get_Line(14), 20, 420, WHITE, -1);
		textout_ex(global.canvas, font, env.ingame->Get_Line(15), 20, 450, WHITE, -1);
		textout_ex(global.canvas, font, env.ingame->Get_Line(16), 20, 465, WHITE, -1);
	}

	global.unlockLand();
	fi = 1;
}


void draw_simple_bg(bool drawImage)
{
	if (!env.drawBackground)
		rectfill(global.canvas, 0, 0, env.screenWidth - 1, env.screenHeight - 1, BLACK);
	else if ( drawImage && env.misc[17] )
		stretch_blit(env.misc[17], global.canvas, 0, 0,
					env.misc[17]->w, env.misc[17]->h, 0, 0,
					env.screenWidth, env.screenHeight);
	else
		rectfill(global.canvas, 0, 0, env.screenWidth - 1, env.screenHeight - 1, DARK_GREEN);
}


static void draw_weapon_list(PLAYER* pl, int32_t* trolley,
                             int32_t scroll_old, int32_t scroll_new,
                             int32_t over_old, int32_t over_new)
{
	// Some pre-calculations and settings.
	int32_t        startX       = env.screenWidth - STUFF_BAR_WIDTH;
	int32_t        halfBar      = STUFF_BAR_HEIGHT / 2;
	static int32_t qtyTxtLen    = 0;
	BITMAP*        imgReleased  = env.gfxData.stuff_bar[0];
	BITMAP*        imgPressed   = env.gfxData.stuff_bar[1];
	int32_t        col_add      = YELLOW;             // Bought items
	int32_t        col_sub      = makecol(176, 0, 0); // Sold items
	static char    buf_cost[50] = { 0 };
	static char    buf_amt[50]  = { 0 };
	bool           full_redraw  = (scroll_new != scroll_old);

	memset(buf_cost, 0, sizeof(char) * 50);
	memset(buf_amt,  0, sizeof(char) * 50);

	if (0 == qtyTxtLen) {
		qtyTxtLen = text_length(font, "Qty. in inventory: ddd");
	}

	// Erase top gap:
	if (full_redraw) {
		global.lockLand();
		rectfill(global.canvas, startX, STUFF_BAR_HEIGHT - 5,
				 startX + env.gfxData.stuff_icon_base->w, STUFF_BAR_HEIGHT,
				 makecol(8, 110, 24));
		global.unlockLand();
		global.make_update(startX, STUFF_BAR_HEIGHT - 5,
						   env.gfxData.stuff_icon_base->w, 5);
	}

	// go through all items and draw them on the screen with
	// the amount of items in the trolley
	for (int32_t slot = 1; slot <= btps; ++slot) {
		int32_t     itemIdx = slot + scroll_new - 1;
		int32_t     itemNum = env.availableItems[itemIdx];
		int32_t     startY  = slot * STUFF_BAR_HEIGHT;
		const char* name    = nullptr;
		int32_t     amt     = 0;
		int32_t     d_div   = 1;

		// Only actually draw the slot, if it has changed:
		if ( !full_redraw
		  && (over_old != itemNum)
		  && (over_new != itemNum) )
			continue;

		// Get text values:
		if (itemNum < WEAPONS) {
			d_div = weapon[itemNum].getDelayDiv();
			name  = weapon[itemNum].getName();
			amt   = pl->nm[itemNum] / d_div;
			snprintf (buf_cost, 49, "$%s", Add_Comma( weapon[itemNum].cost ) );
			snprintf (buf_amt,  49, "for %d", weapon[itemNum].amt / d_div);
		} else {
			name = item[itemNum - WEAPONS].getName();
			amt  = pl->ni[itemNum - WEAPONS];
			snprintf (buf_cost, 49, "$%s",
			          Add_Comma( item[itemNum - WEAPONS].cost ) );
			snprintf (buf_amt,  49, "for %d", item[itemNum - WEAPONS].amt);
		}

		global.lockLand();

		// Draw the background sprites
		draw_sprite(global.canvas,
		            (over_new == itemNum) ? imgPressed : imgReleased,
		            startX, startY);
		draw_sprite(global.canvas, env.gfxData.stuff_icon_base, startX, startY);
		draw_sprite(global.canvas, env.stock[itemNum], startX, startY - 5);
		global.make_update(startX, startY, STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT + 5);

		// Draw the text:
		textout_ex   (global.canvas, font, name,
		              startX + 45, startY - 1, BLACK, -1);
		textprintf_ex(global.canvas, font,
		              startX + 45, startY + halfBar - 4, BLACK, -1,
		              "%s: %d",
		              env.ingame->Get_Line(40), amt);
		if (trolley[itemNum])
			textprintf_ex(global.canvas, font, startX + 45 + qtyTxtLen,
			              startY + halfBar - 4,
			              trolley[itemNum] > 0 ? col_add : col_sub, -1,
			              "%+d", trolley[itemNum] / d_div);
		textout_ex (global.canvas, font, buf_cost,
		            env.screenWidth - 45 - text_length(font, buf_cost),
		            startY - 1, BLACK, -1);
		textout_ex (global.canvas, font, buf_amt,
		            env.screenWidth - 45 - text_length(font, buf_amt),
		            startY + halfBar - 4, BLACK, -1);
		global.unlockLand();

        // Break up if done:
        // (This should not be triggered ever, as scroll is controlled by shop()
		//  to never be more than env.numAvailable-btps.)
		if ( (itemIdx >= (env.numAvailable - 1)) && (slot < btps) )
			slot = btps + 1;
	}

	fi = 1;
}


/** @brief Executes a fast and simple transition from global.canvas to the screen.
**/
void quickChange(bool clearerror)
{
	if (errorMessage) {
		textout_ex (global.canvas, font, errorMessage, errorX, errorY,
		            makecol (255, 0, 0), -1);
		if (clearerror)
			errorMessage = nullptr;
	}

	blit (global.canvas, screen, 0, 0, 0, 0, env.screenWidth, env.screenHeight);
}

