#pragma once
#ifndef ATANKS_SRC_OPTIONCONTENT_H_INCLUDED
#define ATANKS_SRC_OPTIONCONTENT_H_INCLUDED

/*
 * atanks - obliterate each other with oversize weapons
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "optiontypes.h"
#include "globaltypes.h"

/** @file optioncontent.h
  *
  * This file defines static string arrays with the text content of the
  * player, play and option menu and all sub menus.
  *
  * It is not necessary to include this file anywhere else but in menu.cpp.
  *
**/


// Maximum number of entries including Title and 0x0 termination per menu
const uint32_t maxEntriesPerMenu = 18;


// Maximum text entries per text class including 0x0 termination
const uint32_t maxEntriesPerClass = 11;


/** @brief string array for the menu content
  *
  * The ordering, although it looks a bit overwhelming here, is quite simple.
  * The first index is the menu class, the second is the language.
  *
  * With this both translation and adding new content is fairly easy. Just
  * copy a block (after adding new enum entries at the proper places in
  * optiontypes.h) and edit to the new content.
  *
  * All text arrays end with a zero 0x0 entry. It is therefore not needed to
  * hard code any menu list sizes.
  *
  * As a rule of thumb, the title is the first line, every other texts are
  * listed with two entries per line. Unless a possible third entry is the
  * finalizing 0x0 entry, it does not need its own line.
**/
const char* const
MenuTitleText[MC_MENUCLASS_COUNT][EL_LANGUAGE_COUNT][maxEntriesPerMenu] = {
	{
	/* -------------------- *
	 * --- AREYOUSURE   --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Sind Sie sicher?",
			"Ja", "Nein", 0x0
		}, {
		/* ===	EL_SLOVAK === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Are you sure?",
			"Yes", "No", 0x0
		}
	}, {
	/* -------------------- *
	 * --- FINANCE      --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Money",
			"Starting Money", "Interest Rate",
			"Round Win Bonus", "Damage Bounty",
			"Self-Damage Penalty", "Team-Damage Penalty", "Tank Destruction Bonus",
			"Tank Self-Destruction Penalty", "Item Sell Multiplier",
			"Teams Share" , "Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Dinheiro",
			"Dinheiro inicial", "Taxa de Juros",
			"Bônus por Vitória", "Bônus por Estrago",
			"Penalidade por Auto-Estrago", "Team-Damage Penalty", "Bônus por Tanque Destruído",
			"Penalidade por Auto-Destruição", "Multiplicador de Item Vendido",
			"Parte das equipes", "Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Finances",
			"Somme de départ", "Taux d'intérêt",
			"Gains par victoire", "Bonus dommages",
			"Pénalité auto-dommages", "Team-Damage Penalty", "Bonus destruction tank",
			"Pénalité autodestruction tank", "Coeff. vente item",
			"Part d'equipes", "Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Geld",
			"Startgeld", "Zinssatz",
			"Rundenbonus", "Schadensbonus",
			"Strafe für Selbstschaden", "Strafe für Teamschaden", "Zerstörungsbonus",
			"Selbstzerstörungsstrafe", "Verkaufsmultiplikator",
			"Mannschaftanteil", "Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Peniaze",
			"Peniaze na začiatku",  "Úroková miera",
			"Bonus pri skončení kola",  "Odmena za poškodenie",
			"Pokuta za vlastné poškodenie",  "Team-Damage Penalty", "Bonus za zničenie tanku",
			"Pokuta za vlastné zničenie tanku",  "Násobiteľ pri predaji položiek",
			"Teamy zdieľajú peniaze", "Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Экономика",
			"Начальные деньги", "Банковский процент",
			"Бонус за победу", "Бонус за попадание",
			"Штраф за попадание в себя", "Team-Damage Penalty", "Бонус за уничтожение",
			"Штраф за самоуничтожение", "Коэфф. продажи снаряжения",
			"Командные боеприпасы", "Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Money",
			"Starting Money", "Interest Rate",
			"Round Win Bonus", "Damage Bounty",
			"Self-Damage Penalty", "Team-Damage Penalty", "Tank Destruction Bonus",
			"Tank Self-Destruction Penalty", "Item Sell Multiplier",
			"Teams Share" , "Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Money",
			"Starting Money", "Interest Rate",
			"Round Win Bonus", "Damage Bounty",
			"Self-Damage Penalty", "Team-Damage Penalty", "Tank Destruction Bonus",
			"Tank Self-Destruction Penalty", "Item Sell Multiplier",
			"Teams Share", "Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- GRAPHICS     --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Graphics",
			"Full Screen", "Dithering",
			"Detailed Land", "Detailed Sky",
			"Fading Text", "Shadowed Text",
			"Swaying Text",
			"Colour Theme", "Screen Width",
			"Screen Height", "Mouse Pointer",
			"Game Speed", "Custom Background",
			"Show AI Feedback", "Dynamic Menu Background",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Gráficos",
			"Tela Cheia", "Pontilhamento",
			"Detalhes do Terreno", "Detalhes do Céu",
			"texto sombreado", "texto de desvanecimento",
			"Swaying Text",
			"tema da cor", "Largura da Tela",
			"Altura da Tela", "Ponteiro do Rato",
			"Velocidade do jogo", "Fundo personalizado",
			"Show AI Feedback", "Dynamic Menu Background",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Graphismes",
			"Full Screen", "Tramage",
			"Détails du terrain", "Ciel détaillé",
			"texte ombragé", "texte de effacement",
			"Swaying Text",
			"Thème de couleurs", "Largeur d'écran",
			"Hauteur d'écran", "Curseur de souris",
			"Vitesse du jeu", "Fond fait sur commande",
			"Show AI Feedback", "Dynamic Menu Background",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Grafik",
			"Vollbild", "Dithering",
			"Landdetails", "Himmeldetails",
			"Ausblendender Text", "Schattierter Text",
			"Schwingender Text",
			"Farbschema", "Bildschirmbreite",
			"Bildschirmhöhe", "Mauszeiger",
			"Spielgeschwindigket", "Eigener Hintergrund",
			"Zeige AI Feedback", "Dynamischer Menühintergrund",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Grafika",
			"Na celú obrazovku", "Rozptyl",
			"Detaily krajiny", "Detaily oblohy",
			"Slabnúci text", "Text s tieňom",
			"Swaying Text",
			"Farebná téma", "Šírka obrazovky",
			"Výška obrazovky", "Ukazovateľ myši",
			"Rýchlosť hry", "Vlastné pozadie",
			"Show AI Feedback", "Dynamic Menu Background",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Графика",
			"Full Screen", "Сглаживание",
			"Детализированный ландшафт", "Детализированное небо",
			"Исчезающий текст", "Оттененный текст",
			"Swaying Text",
			"Цветовая тема", "Ширина окна игры",
			"Высота окна игры", "Курсор в игре",
			"Скорость игры", "Собственный фон",
			"Show AI Feedback", "Dynamic Menu Background",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Graphics",
			"Full Screen", "Dithering",
			"Detailed Land", "Detailed Sky",
			"Fading Text", "Shadowed Text",
			"Swaying Text",
			"Colour Theme", "Screen Width",
			"Screen Height", "Mouse Pointer",
			"Game Speed", "Custom Background",
			"Show AI Feedback", "Dynamic Menu Background",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Graphics",
			"Full Screen", "Dithering",
			"Detailed Land", "Detailed Sky",
			"Fading Text", "Shadowed Text",
			"Swaying Text",
			"Colour Theme", "Screen Width",
			"Screen Height", "Mouse Pointer",
			"Game Speed", "Custom Background",
			"Show AI Feedback", "Dynamic Menu Background",
			"Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- MAIN         --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Main Menu",
			"Reset All", "Physics",
			"Weather", "Graphics",
			"Money", "Network",
			"Sound", "Weapon Tech Level",
			"Item Tech Level", "Landscape",
			"Turn Order", "Skip AI-only play",
			"Show FPS", "Language",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Menu Principal",
			"Reset All", "Física",
			"Condições Meteorológicas", "Gráficos",
			"Finanças", "Rede",
			"Som", "Arma Nível Tecnológico",
			"Artigo Nível Tecnológico", "Cenário",
			"Ordem de Jogadas", "Continuar o Jogo Só com Robôs",
			"Show FPS", "Língua",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Menu principal",
			"Reset All", "Physique",
			"Météo", "Graphismes",
			"Finances", "Réseau",
			"Sound", "Niveau technique armes",
			"Niveau technique équipement", "Paysage",
			"Ordre de passage", "Continuer le Jeu Robots seuls",
			"Show FPS", "Langue",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Hauptmenü",
			"Alles zurücksetzen", "Physik",
			"Wetter", "Grafik",
			"Geld", "Netzwerk",
			"Sounds", "Technologiestufe Waffen",
			"Technologiestufe Gegenstände", "Landschaft",
			"Reihenfolge", "Überspringe Nur-KI",
			"FPS anzeigen", "Sprache",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Hlavné menu",
			"Reset All", "Fyzika",
			"Počasie", "Grafika",
			"Peniaze", "Sieť",
			"Zvuk", "Tech úroveň zbraní",
			"Tech úroveň vecí", "Krajina",
			"Poradie","Preskočiť hru samotného PC",
			"Show FPS", "Jazyk",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Главное меню",
			"Reset All", "Физика",
			"Погода", "Графика",
			"Экономика", "Настройки сети",
			"Звук", "Уровень оружия",
			"Уровень снаряжения", "Тип ландшафта",
			"Порядок хода", "Пропускать игру компьютеров",
			"Show FPS", "Язык (Language)",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Main Menu",
			"Reset All", "Physics",
			"Weather", "Graphics",
			"Money", "Network",
			"Sound", "Weapon Tech Level",
			"Item Tech Level", "Landscape",
			"Turn Order", "Skip AI-only play",
			"Show FPS", "Language",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Main Menu",
			"Reset All", "Physics",
			"Weather", "Graphics",
			"Money", "Network",
			"Sound", "Weapon Tech Level",
			"Item Tech Level", "Landscape",
			"Turn Order", "Skip AI-only play",
			"Show FPS", "Language",
			"Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- NETWORK      --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Network",
			"Check Updates",  "Networking",
			"Listen Port", "Server Address",
			"Server Port", "Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Network",
			"Procurar actualizações", "Activar Rede",
			"Número de Porta", "Server Address",
			"Server Port", "Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"Network",
			"Check Updates",  "Networking",
			"Listen Port", "Server Address",
			"Server Port", "Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Netzwerk",
			"Auf Aktualisierungen prüfen", "Netzwerk",
			"offener Port", "Serveraddresse",
			"Server Port", "Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Sieť",
			"Kontrola aktualizácii", "Sieťová hra",
			"Port pre načúvanie", "Server Address",
			"Server Port", "Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Настройки сети",
			"Проверять обновления", "Networking",
			"Listen Port", "Server Address",
			"Server Port", "Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Network",
			"Check Updates",  "Networking",
			"Listen Port", "Server Address",
			"Server Port", "Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Network",
			"Check Updates",  "Networking",
			"Listen Port", "Server Address",
			"Server Port", "Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- PHYSICS      --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Physics",
			"Gravity", "Viscosity",
			"Land Slide", "Land Slide Delay",
			"Wall Type", "Boxed Mode",
			"Boxed Ceiling Wrapping",
			"Violent Death", "Timed Shots",
			"Volley Delay", "Explosion Debris",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Física",
			"Gravidade", "Viscosidade",
			"Deslizamento de Terra", "Corrediça da terra atrasa",
			"Tipo de Parede", "Modalidade encaixotada",
			"Boxed Ceiling Wrapping",
			"Morte violenta", "Tiro programado",
			"Volley Delay", "Explosion Debris",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Physique",
			"Gravité", "Viscosité",
			"Glissements de terrain", "Délai glissements de terrain",
			"Murs", "Enfermé dans boîte",
			"Boxed Ceiling Wrapping",
			"Mort violente", "Projectile synchronisé",
			"Volley Delay", "Explosion Debris",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Physik",
			"Gravitation", "Reibung",
			"Erdrutsch", "Erdrutsch Verzögerung",
			"Wand Art", "Höhlenmodus",
			"Höhlendeckenwarp",
			"Gewalttätiger Tod", "Zeitlimit",
			"Mehrfachschussverzögerung", "Explosionsschrott",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Fyzika",
			"Gravitácia", "Viskozita",
			"Zosun zeme", "Zdržanie zosunu zeme",
			"Typ steny", "Režim krabíc",
			"Boxed Ceiling Wrapping",
			"Krutá smrť", "Časované strely",
			"Volley Delay", "Explosion Debris",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Физика",
			"Гравитация", "Сила трения",
			"Падение земли", "Задержка падения земли",
			"Тип стен", "Потолок",
			"Boxed Ceiling Wrapping",
			"Мощные взрывы танков", "Задержка выстрела",
			"Volley Delay", "Explosion Debris",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Physics",
			"Gravity", "Viscosity",
			"Land Slide", "Land Slide Delay",
			"Wall Type", "Boxed Mode",
			"Boxed Ceiling Wrapping",
			"Violent Death", "Timed Shots",
			"Volley Delay", "Explosion Debris",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Physics",
			"Gravity", "Viscosity",
			"Land Slide", "Land Slide Delay",
			"Wall Type", "Boxed Mode",
			"Boxed Ceiling Wrapping",
			"Violent Death", "Timed Shots",
			"Volley Delay", "Explosion Debris",
			"Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- PLAY         --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Select Players",
			"Rounds", "New Game Name",
			"or Load Game", "Load Game",
			"Campaign", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
			"Select Players",
			"Rounds", "New Game Name",
			"or Load Game", "Load Game",
			"Campaign", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"Select Players",
			"Rounds", "New Game Name",
			"or Load Game", "Load Game",
			"Campaign", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Spieler auswählen",
			"Rundenanzahl", "Neues Spiel",
			"oder Spiel laden", "Spiel laden",
			"Kampagne", "Starten",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Výber hráčov",
			"Kolá", "Názov novej hry",
			"alebo načítať hru", "Načítať hru",
			"Kampaň", "OK",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Выберите игроков",
			"Кол-во раундов", "Имя для игры",
			"или имя прошлой игры", "Загрузить игру",
			"Кампания", "OK",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Select Players",
			"Rounds", "New Game Name",
			"or Load Game", "Load Game",
			"Campaign", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Select Players",
			"Rounds", "New Game Name",
			"or Load Game", "Load Game",
			"Campaign", "Okay",
			"Back", 0x0
		}
	}, {
		/* -------------------- *
		 * --- PLAYER       --- *
		 * -------------------- *
		 * Note: The title says "New Player", but this class is used for the
		 * player editing, too. There the title is substituted by the player
		 * name.
		 * Further the "New Player" screen itself does not display the
		 * "Delete This Player" option.
		 */
		{
		/* === EL_ENGLISH === */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Neuer Spieler",
			"Name", "Farbe",
			"Typ", "Team",
			"Erzeuge Konfig", "Gespielt",
			"Gewonnen", "Panzertyp",
			"Diesen Spieler Löschen", "Anlegen",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "OK",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "OK",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "Okay",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"New Player",
			"Name", "Colour",
			"Type", "Team",
			"Generate Pref", "Played",
			"Won", "Tank Type",
			"Delete This Player", "Okay",
			"Back", 0x0
		}
	}, {
		/* -------------------- *
		 * --- PLAYERS      --- *
		 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Players",
			"Create New", "Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Spieler",
			"Neuer Spieler", "Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Players",
			"Create New", "Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- RESET        --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
			"Optionen zurücksetzen?",
			"Zurücksetzen", "Abbruch", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_SLOVAK === */
		/* ===== Needs to be translated ===== */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Reset Options?",
			"Reset", "Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Reset Options?",
			"Reset", "Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- SOUND        --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Sound",
			"All Sound", "Sound Driver",
			"Music", "Volume Factor",
			"Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Som",
			"Efeitos de Som", "Sistema de Som",
			"Música", "Volume Factor",
			"Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Sound",
			"Effets Sonores", "Système de Son",
			"Musique", "Volume Factor",
			"Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Sounds",
			"Alle Sounds", "Sound Treiber",
			"Musik", "Lautstärkefaktor",
			"Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Zvuk",
			"Všetky zvuky", "Ovládač zvuku",
			"Hudba", "Volume Factor",
			"Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
			"Sound",
			"All Sound", "Sound Driver",
			"Music", "Volume Factor",
			"Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Sound",
			"All Sound", "Sound Driver",
			"Music", "Volume Factor",
			"Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Sound",
			"All Sound", "Sound Driver",
			"Music", "Volume Factor",
			"Back", 0x0
		}
	}, {
	/* -------------------- *
	 * --- WEATHER      --- *
	 * -------------------- */
		{
		/* === EL_ENGLISH === */
			"Weather",
			"Meteor Showers", "Lightning",
			"Falling Dirt", "Laser Satellite",
			"Fog", "Max Wind Strength",
			"Wind Variation", "Back", 0x0
		}, {
		/* ===	EL_PORTUGUESE === */
			"Condições Meteorológicas",
			"Chuvas de Meteoro", "Relâmpagos",
			"Sujeira de queda", "Satélite do Laser",
			"Neblina", "Velocidade Max do Vento",
			"Variação do Vento", "Back", 0x0
		}, {
		/* ===	EL_FRENCH === */
			"Météo",
			"Orages de météorites", "Éclairs",
			"Saleté en chute", "Satellites Laser",
			"Brouillard", "Force maxi du vent",
			"Variation du vent", "Back", 0x0
		}, {
		/* ===	EL_GERMAN === */
			"Wetter",
			"Meteoritenregen", "Gewitter",
			"Schmutzregen", "Lasersatellit",
			"Nebel", "Max Windstärke",
			"Windveränderung", "Zurück", 0x0
		}, {
		/* ===	EL_SLOVAK === */
			"Počasie",
			"Dážď meteorov", "Blesky",
			"Padajúca zem", "Laserový satelit",
			"Hmla", "Maximálna sila vetra",
			"Zmena vetra", "Späť", 0x0
		}, {
		/* ===	EL_RUSSIAN === */
			"Погода",
			"Метеоритный дождь", "Молнии",
			"Падающая грязь", "Удары со спутника",
			"Туман", "Макс. сила ветра",
			"Изменения силы ветра", "Назад", 0x0
		}, {
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
			"Weather",
			"Meteor Showers", "Lightning",
			"Falling Dirt", "Laser Satellite",
			"Fog", "Max Wind Strength",
			"Wind Variation", "Back", 0x0
		}, {
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
			"Weather",
			"Meteor Showers", "Lightning",
			"Falling Dirt", "Laser Satellite",
			"Fog", "Max Wind Strength",
			"Wind Variation", "Back", 0x0
		}
	}
};


/** @brief string array for the option text class content
  *
  * The ordering, although it looks a bit overwhelming here, is quite simple.
  * The first index is the text class, the second is the language.
  *
  * With this both translation and adding new content is fairly easy. Just
  * copy a block (after adding new enum entries at the proper places in
  * optiontypes.h) and edit to the new content.
  *
  * All text arrays end with a zero 0x0 entry. It is therefore not needed to
  * hard code any option value sizes.
**/
const char* const
OptionClassText[TC_TEXTCLASS_COUNT][EL_LANGUAGE_COUNT][maxEntriesPerClass] = {
	{
	/* -------------------- *
	 * --- TC_COLOUR   --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Regular", "Crispy", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Regular", "Crispy", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Régulier", "Croustillant", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Normal", "Kontrastreich", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Normálna", "Svieža", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Обычная", "Четкая", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Regular", "Crispy", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Regular", "Crispy", 0x0 }

	}, {
	/* --------------------- *
	 * --- TC_LANDSLIDE --- *
	 * --------------------- */
		/* === EL_ENGLISH === */
		{ "None", "Tank Only", "Instant", "Gravity", "Cartoon", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Nenhum", "Tanque Somente", "Instantâneo", "Gravidade", "Cartoon", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Aucun", "Réservoir Seulement", "Instantané", "Gravité", "Dessin animé", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Keine", "Nur Panzer", "Sofort", "Schwerkraft", "Cartoon", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Žiaden", "Iba tank", "Okamžitý", "Gravitácia", "Kresl.film", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Выкл.", "Только танки", "Сразу же", "По умолчанию", "Как в мультиках", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "None", "Tank Only", "Instant", "Gravity", "Cartoon", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "None", "Tank Only", "Instant", "Gravity", "Cartoon", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_LANDTYPE --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Aleatório", "Canyons", "Montanhas", "Vales", "Colinas", "Morros", "Planos", "Nenhum", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Aléatoire", "Canyons", "Montagnes", "Vallées", "Collines", "Contreforts", "Plaines", "Aucun", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Zufällig", "Canyons", "Berge", "Täler", "Hügel", "Flache Hügel", "Ebene", "Nichts", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Náhodná", "Kaňony", "Hory", "Údolia", "Kopce", "Úpätia", "Nížiny", "Žiadna", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Случайный", "Каньоны", "Горы", "Возвышенность", "Холмы", "Предгорья", "Равнины", "Выкл.", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Random", "Canyons", "Mountains", "Valleys", "Hills", "Foothills", "Plains", "None", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_LANGUAGE --- *
	 * -------------------- */

		/* === EL_ENGLISH === */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 },
		/* ===	EL_FRENCH === */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 },
		/* ===	EL_GERMAN === */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Anglicky", "Portugalsky", "Francúzsky", "Nemecky", "Slovensky", "Rusky", "Spanish", "Italian", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Русский", "Spanish", "Italian", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "English", "Português", "Français", "Deutsch", "Slovak", "Russian", "Spanish", "Italian", 0x0 }

	}, {
	/* --------------------- *
	 * --- TC_LIGHTNING --- *
	 * --------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "Weak", "Energetic", "Violent", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Desligado", "Fraco", "Energético", "Violento", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Aucun", "Faible", "Energique", "Violent", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "Schwach", "Energetisch", "Brutal", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnuté", "Slabé", "Energetické", "Kruté", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Нет", "Слабые", "Сильные", "Мощные", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Weak", "Energetic", "Violent", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Weak", "Energetic", "Violent", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_METEOR   --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "Light", "Heavy", "Lethal", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Desligado", "Fraco", "Forte", "Letal", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Light", "Heavy", "Lethal", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "Leicht", "Schwer", "Tödlich", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnuté", "Ľahké", "Ťažké", "Smrteľné", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Нет", "Слабый", "Сильный", "Смертельный", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Light", "Heavy", "Lethal", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Light", "Heavy", "Lethal", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_MOUSE    --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Custom", "Default", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Personalizado", "Padrão", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Pesonnel", "Défaut", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Angepasst", "Standard", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vlastné", "Východzie", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Собственный", "По умолчанию", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Custom", "Default", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Custom", "Default", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_OFFON    --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "On", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Desligado", "Ligado", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Non", "Oui", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "An", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnuté", "Zapnuté", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Выкл.", "Вкл.", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "On", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "On", 0x0 }

	}, {
	/* ----------------------- *
	 * --- TC_OFFONRANDOM --- *
	 * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "On", "Random", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Desligado", "Ligado", "Aleatório", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Non", "Oui", "Hasard", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "An", "Zufällig", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnuté", "Zapnuté", "Náhodný", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Выкл.", "Вкл.", "Случайно", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "On", "Random", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "On", "Random", 0x0 }

	}, {
	/* ----------------------- *
	 * --- TC_PLAYERPREF  --- *
	 * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Per Game", "Only Once", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Per Game", "Only Once", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Per Game", "Only Once", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Pro Spiel", "Nur einmal", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Na hru", "Iba raz", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Каждую игру заново", "Только один раз", 0x0 },
		/* ===	EL_SPANISH === */
		{ "Por Juego", "Solo una vez", 0x0 },
		/* ===	EL_ITALIAN === */
		{ "Per Gioco", "Only Once", 0x0 },

	} , {
	/* ----------------------- *
	 * --- TC_PLAYERTEAM  --- *
	 * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Sith", "Neutral", "Jedi", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Sith", "Neutral", "Jedi", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Sith", "Neutral", "Jedi", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Sith", "Neutral", "Jedi", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Sith", "Neutrálny", "Jedi", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Ситх", "Нейтральный", "Джедай", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Sith", "Neutral", "Jedi", 0x0 },
		/* ===	EL_ITALIAN === */
		{ "Sith", "Neutrale", "Jedi", 0x0 },

	} , {
	/* ----------------------- *
	 * --- TC_PLAYERTYPE  --- *
	 * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Human", "Useless", "Guesser", "Range", "Targetter", "Deadly", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Mensch", "Nutzlos", "Ratlos", "Schütze", "Scharfschütze", "Tödlich", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Človek", "Nepoužiteľný", "Ten, čo háda", "Ten, čo hľadá správnu silu", "Ten, čo mieri", "Ten, čo prináša smrť", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Человек", "Ноль", "Слабый ИИ", "Средний ИИ", "Сильный ИИ", "Терминатор", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Humano", "Inservible", "Guesser", "Rango", "Targetter", "Mortal", 0x0 },
		/* ===	EL_ITALIAN === */
		{ "Umano", "Sottodotato", "Mediocre", "Medio", "Elevato", "Mortale", 0x0 },

	}, {
	/* --------------------- *
	 * --- TC_SATELLITE --- *
	 * --------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "Weak", "Strong", "Super", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Desligado", "Fraco", "Forte", "Super", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Aucun", "Faible", "Fort", "Super", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "Schwach", "Stark", "Super", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnutý", "Slabý", "Silný", "Super", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Нет", "Слабые", "Сильные", "Супер!!", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Weak", "Strong", "Super", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Weak", "Strong", "Super", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_SKIPTYPE --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Off", "Humans Dead", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Wrong translation ? ===== */
		{ "Desligado", "Ligado", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Wrong translation ? ===== */
		{ "Non", "Oui", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Aus", "Menschen Tot", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vypnuté", "Smrť ľudí", 0x0 },
		/* ===	EL_RUSSIAN === */
		/* ===== Wrong translation ? ===== */
		{ "Выкл.", "Вкл.", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Humans Dead", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Off", "Humans Dead", 0x0 }

	}, {
	/* ----------------------- *
	 * --- TC_SOUNDDRIVER --- *
	 * ----------------------- */
		/* === EL_ENGLISH === */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Automatisch", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_SLOVAK === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_RUSSIAN === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Auto Detect", "OSS", "ESD", "ARTS", "ALSA", "JACK", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_TANKTYPE --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", 0x0 },
		/* ===	EL_PORTUGUESE === */
		/* ===== Needs to be translated ===== */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", 0x0 },
		/* ===	EL_FRENCH === */
		/* ===== Needs to be translated ===== */
		{ "Normal", "Classic", "Big Grey", "T34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Normal", "Klassisch", "Der Große Graue", "T34", "Schwergewicht", "Futuristisch", "UFO", "Spinne", "Big Foot", "Mini", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Bežný", "Klasický", "Veľký šedý", "T34", "Ťažký", "Futuristický", "UFO", "Spider", "Big Foot", "Mini", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Обычный", "В старом стиле", "Большой Серый Танк", "Т-34", "Heavy", "Future", "UFO", "Spider", "Big Foot", "Mini", 0x0 },
		/* ===	EL_SPANISH === */
		{ "Normal", "Clasico", "Big Grey", "T34", "Pesado", "Futuro", "UFO", "Araña", "Big Foot", "Mini", 0x0 },
		/* ===	EL_ITALIAN === */
		{ "Normale", "Classico", "Big Grey", "T34", "Pesante", "Futuro", "UFO", "Spider", "Big Foot", "Mini", 0x0 },

	}, {
	/* -------------------- *
	 * --- TC_TURNTYPE --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "High+", "Low+", "Random", "Simul", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Melhores+", "Piores+", "Aleatório", "Simular", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Haut", "Bas", "Aléatoire", "Similaire", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Hoch+", "Niedrig+", "Zufällig", "Simul", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Vysoký+", "Nízky+", "Náhodný", "Simul", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Сильные +", "Слабые +", "Случайно", "Все сразу", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "High+", "Low+", "Random", "Simul", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "High+", "Low+", "Random", "Simul", 0x0 }

	}, {
	/* -------------------- *
	 * --- TC_WALLTYPE --- *
	 * -------------------- */
		/* === EL_ENGLISH === */
		{ "Rubber", "Steel", "Spring", "Wrap", "Random", 0x0 },
		/* ===	EL_PORTUGUESE === */
		{ "Elástico", "Aço", "Mola", "Envoltório", "Aleatório", 0x0 },
		/* ===	EL_FRENCH === */
		{ "Elastique", "Acier", "Mou", "Enveloppe", "Aléatoire", 0x0 },
		/* ===	EL_GERMAN === */
		{ "Gummi", "Stahl", "Federnd", "Verbunden", "Zufällig", 0x0 },
		/* ===	EL_SLOVAK === */
		{ "Guma", "Oceľ", "Pružina", "Prikrývka", "Náhodný", 0x0 },
		/* ===	EL_RUSSIAN === */
		{ "Резиновые", "Непробиваемые", "Пружинящие", "Бесконечность", "Случайные", 0x0 },
		/* ===	EL_SPANISH === */
		/* ===== Needs to be translated ===== */
		{ "Rubber", "Steel", "Spring", "Wrap", "Random", 0x0 },
		/* ===	EL_ITALIAN === */
		/* ===== Needs to be translated ===== */
		{ "Rubber", "Steel", "Spring", "Wrap", "Random", 0x0 }

	}
}; // End of MenuClassText


#endif // ATANKS_SRC_OPTIONCONTENT_H_INCLUDED

