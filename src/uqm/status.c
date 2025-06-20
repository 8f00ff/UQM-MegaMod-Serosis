//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "status.h"
#include "colors.h"
#include "globdata.h"
#include "races.h"
#include "ship.h"
#include "setup.h"
#include "options.h"
#include "init.h"
		// for NUM_PLAYERS
#include "menustat.h"
#include "util.h"
#include "intel.h"

#include <stdio.h>
#include <string.h>


COORD status_y_offsets[NUM_PLAYERS];


void
InitStatusOffsets (void)
{
	// XXX: We have to jump through these hoops because GOOD_GUY_YOFFS is
	//   not a constant, contrary to what its name suggests.
	status_y_offsets[0] = GOOD_GUY_YOFFS; // bottom player
	status_y_offsets[1] = BAD_GUY_YOFFS;  // top player
}

bool
animIsInTransition (CAPTAIN_STUFF *CSPtr)
{
	return (CSPtr->thrust_offset & 1) ||
			(CSPtr->weapon_offset & 1) ||
			(CSPtr->special_offset & 1) ||
			(CSPtr->tl_offset & 1) ||
			(CSPtr->tr_offset & 1);
}

static BYTE
CalculateAnimOffsets (CAPTAIN_STUFF* CSPtr, 
		STATUS_FLAGS delta_status_flags, STATUS_FLAGS cur_status_flags)
{
	BYTE redraw_flag = 0;

	if (delta_status_flags & LEFT || CSPtr->tl_offset & 1)
	{
		if (!(delta_status_flags & RIGHT))
		{
			CSPtr->tr_offset = 0;

			if (cur_status_flags & LEFT)
				CSPtr->tl_offset++;
			else
				CSPtr->tl_offset--;

			redraw_flag |= LEFT;
		}
		else if (cur_status_flags & RIGHT)
		{
			CSPtr->tl_offset = 0;
			CSPtr->tr_offset = 1;

			redraw_flag |= RIGHT;
		}
		else
		{
			CSPtr->tr_offset = 0;
			CSPtr->tl_offset = 1;

			redraw_flag |= LEFT;
		}
	}
	else if (delta_status_flags & RIGHT || CSPtr->tr_offset & 1)
	{
		CSPtr->tl_offset = 0;

		if (cur_status_flags & RIGHT)
			CSPtr->tr_offset++;
		else
			CSPtr->tr_offset--;

		redraw_flag |= RIGHT;
	}

	if (delta_status_flags & THRUST || CSPtr->thrust_offset & 1)
	{
		if (cur_status_flags & THRUST)
			CSPtr->thrust_offset++;
		else
			CSPtr->thrust_offset--;

		redraw_flag |= THRUST;
	}

	if (delta_status_flags & WEAPON || CSPtr->weapon_offset & 1)
	{
		if (cur_status_flags & WEAPON)
			CSPtr->weapon_offset++;
		else
			CSPtr->weapon_offset--;

		redraw_flag |= WEAPON;
	}

	if (delta_status_flags & SPECIAL || CSPtr->special_offset & 1)
	{
		if (cur_status_flags & SPECIAL)
			CSPtr->special_offset++;
		else
			CSPtr->special_offset--;

		redraw_flag |= SPECIAL;
	}

	return redraw_flag;
}

static void
DrawCaptainWindowAnimation (CAPTAIN_STUFF* CSPtr, COORD y, 
		BYTE redraw_flag)
{
	STAMP Stamp;

	Stamp.origin.x = CAPTAIN_XOFFS;
	Stamp.origin.y = y + CAPTAIN_YOFFS;

	if (redraw_flag & LEFT)
	{
		BYTE i;

		Stamp.frame = SetRelFrameIndex (CSPtr->turn, 1);
		DrawStamp (&Stamp);
		Stamp.frame = IncFrameIndex (Stamp.frame);
		DrawStamp (&Stamp);

		for (i = 0; i < CSPtr->tl_offset; i++)
		{
			Stamp.frame = IncFrameIndex (Stamp.frame);
			DrawStamp (&Stamp);
		}
	}
	else if (redraw_flag & RIGHT)
	{
		BYTE i;

		Stamp.frame = SetRelFrameIndex (CSPtr->turn, 3);
		DrawStamp (&Stamp);
		Stamp.frame = DecFrameIndex (Stamp.frame);
		DrawStamp (&Stamp);

		for (i = 0; i < CSPtr->tr_offset; i++)
		{
			Stamp.frame = DecFrameIndex (Stamp.frame);
			DrawStamp (&Stamp);
		}
	}

	if (redraw_flag & THRUST)
	{
		Stamp.frame = SetRelFrameIndex (CSPtr->thrust, CSPtr->thrust_offset);
		DrawStamp (&Stamp);
	}

	if (redraw_flag & WEAPON)
	{
		Stamp.frame = SetRelFrameIndex (CSPtr->weapon, CSPtr->weapon_offset);
		DrawStamp (&Stamp);
	}

	if (redraw_flag & SPECIAL)
	{
		Stamp.frame = SetRelFrameIndex (CSPtr->special, CSPtr->special_offset);
		DrawStamp (&Stamp);
	}
}

static void
DrawCaptainWindowFrame (FRAME fr, COORD y)
{
	STAMP Stamp;

	Stamp.origin.x = CAPTAIN_XOFFS;
	Stamp.origin.y = y + CAPTAIN_YOFFS;

	Stamp.frame = fr;

	DrawStamp (&Stamp);
}

/*static void
CaptainsWindow (CAPTAIN_STUFF *CSPtr, COORD y,
		STATUS_FLAGS delta_status_flags, STATUS_FLAGS cur_status_flags,
		COUNT Pass)
{// Kruzen: og method. now redundant
	STAMP Stamp;

	Stamp.origin.x = CAPTAIN_XOFFS;
	Stamp.origin.y = y + CAPTAIN_YOFFS;

	if (delta_status_flags & LEFT)
	{
		Stamp.frame = CSPtr->turn;
		if (!(delta_status_flags & RIGHT))
		{
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			if (Pass == 2)
			{
				if (cur_status_flags & LEFT)
					Stamp.frame = IncFrameIndex (Stamp.frame);
				else
					Stamp.frame = DecFrameIndex (Stamp.frame);
			}
		}
		else if (cur_status_flags & RIGHT)
		{
			if (Pass == 1)
				Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			else
				Stamp.frame = IncFrameIndex (Stamp.frame);
			DrawStamp (&Stamp);
			Stamp.frame = DecFrameIndex (Stamp.frame);
		}
		else
		{
			if (Pass == 1)
				Stamp.frame = IncFrameIndex (Stamp.frame);
			else
				Stamp.frame = SetRelFrameIndex (Stamp.frame, 3);
			DrawStamp (&Stamp);
			Stamp.frame = IncFrameIndex (Stamp.frame);
		}
		DrawStamp (&Stamp);
	}
	else if (delta_status_flags & RIGHT)
	{
		Stamp.frame = CSPtr->turn;
		Stamp.frame = IncFrameIndex (Stamp.frame);
		if (Pass == 2)
		{
			if (cur_status_flags & RIGHT)
				Stamp.frame = DecFrameIndex (Stamp.frame);
			else
				Stamp.frame = IncFrameIndex (Stamp.frame);
		}
		DrawStamp (&Stamp);
	}

	if (delta_status_flags & THRUST)
	{
		Stamp.frame = CSPtr->thrust;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & THRUST)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
	if (delta_status_flags & WEAPON)
	{
		Stamp.frame = CSPtr->weapon;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & WEAPON)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
	if (delta_status_flags & SPECIAL)
	{
		Stamp.frame = CSPtr->special;
		if (Pass == 1)
			Stamp.frame = IncFrameIndex (Stamp.frame);
		else if (cur_status_flags & SPECIAL)
			Stamp.frame = SetRelFrameIndex (Stamp.frame, 2);
		DrawStamp (&Stamp);
	}
}*/

void
DrawBattleCrewAmount (SHIP_INFO *ShipInfoPtr, COORD y_offs)
{
#define MAX_CREW_DIGITS 3
	RECT r;
	TEXT t;
	UNICODE buf[40];

	t.baseline.x = BATTLE_CREW_X + RES_SCALE (MENU_BOOL (2, 4));
	t.baseline.y = BATTLE_CREW_Y + y_offs;
	t.align = MENU_BOOL (ALIGN_LEFT, ALIGN_CENTER);
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	r.corner.x = t.baseline.x - RES_SCALE (MENU_BOOL (2, 9));
	r.corner.y = t.baseline.y - RES_SCALE (5);
	r.extent.width = RES_SCALE (6 * MAX_CREW_DIGITS + 6);
	r.extent.height = RES_SCALE (5);

	sprintf (buf, "%u", ShipInfoPtr->crew_level);
	SetContextFont (StarConFont);

	SetContextForeGroundColor (MENU_FOREGROUND_COLOR);
	DrawFilledRectangle (&r);
	SetContextForeGroundColor (BLACK_COLOR);
	font_DrawText (&t);
}

void
DrawCaptainsWindow (STARSHIP *StarShipPtr)
{
	COORD y;
	COORD y_offs;
	RECT r;
	STAMP s;
	FRAME Frame;
	RACE_DESC *RDPtr;

	RDPtr = StarShipPtr->RaceDescPtr;
	Frame = RDPtr->ship_data.captain_control.background;
	if (Frame)
	{
		Frame = SetAbsFrameIndex (Frame, 0);
		RDPtr->ship_data.captain_control.background = Frame;
		Frame = SetRelFrameIndex (Frame, 1);
		RDPtr->ship_data.captain_control.turn = Frame;
		Frame = SetRelFrameIndex (Frame, 5);
		RDPtr->ship_data.captain_control.thrust = Frame;
		Frame = SetRelFrameIndex (Frame, 3);
		RDPtr->ship_data.captain_control.weapon = Frame;
		Frame = SetRelFrameIndex (Frame, 3);
		RDPtr->ship_data.captain_control.special = Frame;
	}

	BatchGraphics ();
	
	// Grey area under and around captain's window.
	assert (StarShipPtr->playerNr >= 0);
	y_offs = status_y_offsets[StarShipPtr->playerNr];

	r.corner.x = CAPTAIN_XOFFS - RES_SCALE (2);
	r.corner.y = y_offs + SHIP_INFO_HEIGHT;
	r.extent.width = STATUS_WIDTH - CAPTAIN_XOFFS + IF_HD (4);
	r.extent.height = SHIP_STATUS_HEIGHT - CAPTAIN_YOFFS
			+ RES_SCALE (MENU_BOOL (NDOS_NUM (2), 1));
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	DrawFilledRectangle (&r);

	// Left border of the status panel.
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F));
	r.corner.x = RES_SCALE (1);
	r.corner.y = y_offs + SHIP_INFO_HEIGHT;
	r.extent.width = RES_SCALE (1);
	r.extent.height = (SHIP_STATUS_HEIGHT - SHIP_INFO_HEIGHT - RES_SCALE (2));
	DrawFilledRectangle (&r);
	r.corner.x = 0;
	r.extent.height += RES_SCALE (1);
	DrawFilledRectangle (&r);

	// Lower and right border of the status panel.
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19));
	r.corner.x = STATUS_WIDTH - RES_SCALE (1);
	r.corner.y = y_offs + SHIP_INFO_HEIGHT;
	r.extent.width = RES_SCALE (1);
	r.extent.height = SHIP_STATUS_HEIGHT - SHIP_INFO_HEIGHT;
	DrawFilledRectangle (&r);
	r.corner.x = STATUS_WIDTH - RES_SCALE (2);
	DrawFilledRectangle (&r);
	r.corner.x = RES_SCALE (1);
	r.extent.width = STATUS_WIDTH - RES_SCALE (2);
	r.corner.y = y_offs + (SHIP_STATUS_HEIGHT - RES_SCALE (2));
	r.extent.height = RES_SCALE (1);
	DrawFilledRectangle (&r);
	r.corner.x = 0;
	r.extent.width += RES_SCALE (1);
	r.corner.y += RES_SCALE (1);
	DrawFilledRectangle (&r);

	y = y_offs + CAPTAIN_YOFFS;

	if (!IS_HD)
	{
		// Actual Captain's Window
		// Darker grey rectangle at bottom and right of captain's window
		SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F));
		r.corner.x = CAPTAIN_WIDTH + CAPTAIN_XOFFS;
		r.corner.y = y;
		r.extent.width = RES_SCALE (1);
		r.extent.height = CAPTAIN_HEIGHT;
		DrawFilledRectangle (&r);
		r.corner.x = CAPTAIN_XOFFS - RES_SCALE (1);
		r.corner.y += CAPTAIN_HEIGHT;
		r.extent.width = CAPTAIN_WIDTH + RES_SCALE (2);
		r.extent.height = RES_SCALE (1);
		DrawFilledRectangle (&r);

		// Light grey rectangle at top and left of captains window
		SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19));
		r.corner.x = CAPTAIN_XOFFS - RES_SCALE (1);
		r.extent.width = CAPTAIN_WIDTH + RES_SCALE (2);
		r.corner.y = y - RES_SCALE (1);
		r.extent.height = RES_SCALE (1);
		DrawFilledRectangle (&r);
		r.corner.x = CAPTAIN_XOFFS - RES_SCALE (1);
		r.extent.width = RES_SCALE (1);
		r.corner.y = y;
		r.extent.height = CAPTAIN_HEIGHT;
		DrawFilledRectangle (&r);
	}
	else
	{
		r.corner.x = CAPTAIN_XOFFS - 4;
		r.corner.y = y - 4;
		r.extent.width = CAPTAIN_WIDTH + 8;
		r.extent.height = CAPTAIN_HEIGHT + 8;
		DrawRenderedBox (&r, TRUE, BLACK_COLOR, THIN_INNER_BEVEL, FALSE);
	}

	s.frame = RDPtr->ship_data.captain_control.background;
	s.origin.x = CAPTAIN_XOFFS;
	s.origin.y = y;
	DrawStamp (&s);

	if (StarShipPtr->captains_name_index == 0
			&& StarShipPtr->playerNr == RPG_PLAYER_NUM)
	{	// This is SIS
		TEXT t;

		t.baseline.x = STATUS_WIDTH >> 1;
		t.baseline.y = y + RES_SCALE (6);
		t.align = ALIGN_CENTER;
		t.pStr = GLOBAL_SIS (CommanderName);
		t.CharCount = (COUNT)~0;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02));
		SetContextFont (MENU_BOOL (TinyFontBold, TinyFont));
		font_DrawText (&t);
	}
	if (RDPtr->ship_info.max_crew > MAX_CREW_SIZE ||
			RDPtr->ship_info.ship_flags & PLAYER_CAPTAIN)
	{
		// All crew doesn't fit in the graphics; print a number.
		// Always print a number for the SIS in the full game.
		y_offs += SAFE_X;
		DrawBattleCrewAmount (&RDPtr->ship_info, y_offs);
	}

	UnbatchGraphics ();
}

BOOLEAN
DeltaEnergy (ELEMENT *ElementPtr, SIZE energy_delta)
{
	BOOLEAN retval;
	STARSHIP *StarShipPtr;
	SHIP_INFO *ShipInfoPtr;

	retval = TRUE;
	
	if (antiCheat (ElementPtr, FALSE, OPTVAL_INF_ENERGY)
			|| antiCheat (ElementPtr, FALSE, OPTVAL_FULL_GOD))
		return retval;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	ShipInfoPtr = &StarShipPtr->RaceDescPtr->ship_info;
	if (energy_delta >= 0)
	{
		if ((BYTE)(ShipInfoPtr->energy_level + (BYTE)energy_delta) >
				ShipInfoPtr->max_energy)
			energy_delta = ShipInfoPtr->max_energy
					- ShipInfoPtr->energy_level;
	}
	else
	{
		if ((BYTE)-energy_delta > ShipInfoPtr->energy_level)
		{
			retval = FALSE;
		}
	}

	if (!retval)
		StarShipPtr->cur_status_flags |= LOW_ON_ENERGY;
	else
	{
		StarShipPtr->cur_status_flags &= ~LOW_ON_ENERGY;
		StarShipPtr->energy_counter =
				StarShipPtr->RaceDescPtr->characteristics.energy_wait;

		DeltaStatistics (ShipInfoPtr, status_y_offsets[StarShipPtr->playerNr],
				0, energy_delta);
	}

	return (retval);
}

BOOLEAN
DeltaCrew (ELEMENT *ElementPtr, SIZE crew_delta)
{
	BOOLEAN retval;
	STARSHIP *StarShipPtr;
	SHIP_INFO *ShipInfoPtr;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE
			&& ElementPtr->playerNr == NPC_PLAYER_NUM)
		return (TRUE); /* Samatra can't be crew-modified */

	retval = TRUE;
	GetElementStarShip (ElementPtr, &StarShipPtr);
	ShipInfoPtr = &StarShipPtr->RaceDescPtr->ship_info;
	if (crew_delta > 0)
	{
		ElementPtr->crew_level += crew_delta;
		if (ElementPtr->crew_level > ShipInfoPtr->max_crew)
		{
			crew_delta = ShipInfoPtr->max_crew - ShipInfoPtr->crew_level;
			ElementPtr->crew_level = ShipInfoPtr->max_crew;
		}
	}
	else if (crew_delta < 0)
	{
		if (ElementPtr->crew_level > (COUNT)-crew_delta)
			ElementPtr->crew_level += crew_delta;
		else
		{
			crew_delta = -(SIZE)ElementPtr->crew_level;
			ElementPtr->crew_level = 0;
			retval = FALSE;
		}
	}

	DeltaStatistics (ShipInfoPtr, status_y_offsets[StarShipPtr->playerNr],
			crew_delta, 0);

	return (retval);
}

void
PreProcessStatus (ELEMENT *ShipPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->captains_name_index
			|| StarShipPtr->playerNr == RPG_PLAYER_NUM)
	{	// All except Sa-Matra, no captain's window there
		STATUS_FLAGS old_status_flags, cur_status_flags;
		CAPTAIN_STUFF *CSPtr;
		BYTE redraw_flags = 0;

		cur_status_flags = StarShipPtr->cur_status_flags;
		old_status_flags = StarShipPtr->old_status_flags;
		old_status_flags ^= cur_status_flags;

		CSPtr = &StarShipPtr->RaceDescPtr->ship_data.captain_control;
		old_status_flags &= (LEFT | RIGHT | THRUST | WEAPON | SPECIAL);
		if (old_status_flags || animIsInTransition (CSPtr))
		{
			assert (StarShipPtr->playerNr >= 0);
			redraw_flags = CalculateAnimOffsets (CSPtr, old_status_flags, cur_status_flags);

			if (redraw_flags)
				DrawCaptainWindowAnimation (CSPtr, status_y_offsets[StarShipPtr->playerNr], redraw_flags);
		}
	}
}

void
PostProcessStatus (ELEMENT *ShipPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->captains_name_index
			|| StarShipPtr->playerNr == RPG_PLAYER_NUM)
	{	// All except Sa-Matra, no captain's window there
		COORD y;
		STATUS_FLAGS cur_status_flags, old_status_flags;
		

		cur_status_flags = StarShipPtr->cur_status_flags;

		assert (StarShipPtr->playerNr >= 0);
		y = status_y_offsets[StarShipPtr->playerNr];

		if (ShipPtr->crew_level == 0)
		{
			StarShipPtr->cur_status_flags &=
					~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);

			if (StarShipPtr->RaceDescPtr->ship_info.crew_level == 0 &&
					ShipPtr->state_flags & FINITE_LIFE)
			{// Kruzen: FINITE_LIFE check was added in case if crew-level == 0
			 // Is triggered by the ship itself (shofixti glory device) and
			 // avoiding drawing black area
				BYTE i, j;
				Color c;
				RECT r;

				i = (BYTE)(NUM_EXPLOSION_FRAMES * 3 - 1) - ShipPtr->life_span;

				if (cur_status_flags & SHOFIXTI_EXPLOSION && i == 0)
				{// Special case: Instead of first orange rectangle we will draw the last
				 // frame from captain avatars (used for shofixti glory device)
					CAPTAIN_STUFF *CSPtr;
					CSPtr = &StarShipPtr->RaceDescPtr->ship_data.captain_control;

					cur_status_flags &= ~SHOFIXTI_EXPLOSION;
					DrawCaptainWindowFrame (DecFrameIndex (CSPtr->background), y);
				}
				else
				{
					if (i <= 4)
					{
						static const Color flash_tab0[] =
						{
							BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),
							BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x19, 0x19), 0x24),
							BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7B),
							BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1C, 0x00), 0x78),
							BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x1F), 0x0F),
						};

						c = flash_tab0[i];
						r.corner.x = CAPTAIN_XOFFS;
						r.corner.y = y + CAPTAIN_YOFFS;
						r.extent.width = CAPTAIN_WIDTH;
						r.extent.height = CAPTAIN_HEIGHT;
					}
					else
					{
						SetContextForeGroundColor (BLACK_COLOR);
						i -= 5;
						if (i <= 14)
						{
							static const Color flash_tab1[] =
							{
								BUILD_COLOR (MAKE_RGB15_INIT (0x1E, 0x1F, 0x12), 0x70),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x0A), 0x0E),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x00), 0x71),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1C, 0x00), 0x78),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x18, 0x00), 0x79),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7A),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7B),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7C),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x00), 0x7D),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7E),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7F),
								BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2A),
								BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
								BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2C),
								BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),
							};

							c = flash_tab1[i];
							r.corner.x = CAPTAIN_XOFFS + RES_SCALE (i);
							r.corner.y = y + CAPTAIN_YOFFS + RES_SCALE (i);
							r.extent.width = CAPTAIN_WIDTH - RES_SCALE ((i << 1));
							r.extent.height = CAPTAIN_HEIGHT - RES_SCALE ((i << 1));

							if (r.extent.height == RES_SCALE (2))
								r.extent.height += RES_SCALE (1);

							for (j = 0; j < RES_SCALE (1); j++)
							{
								DrawRectangle (&r, FALSE);
								++r.corner.x;
								++r.corner.y;
								r.extent.width -= 2;
								r.extent.height -= 2;
							}
						}
						else if ((i -= 15) <= 4)
						{
							r.corner.y = y + (CAPTAIN_YOFFS + RES_SCALE (15));
							r.extent.width = RES_SCALE (i + 1);
							r.extent.height = RES_SCALE (1);
							switch (i)
							{
							case 0:
								r.corner.x = CAPTAIN_XOFFS + RES_SCALE (15);
								i = CAPTAIN_WIDTH - RES_SCALE ((15 + 1) << 1);
								c = BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C);
								break;
							case 1:
								r.corner.x = CAPTAIN_XOFFS + RES_SCALE (16);
								i = CAPTAIN_WIDTH - RES_SCALE ((17 + 1) << 1);
								c = BUILD_COLOR (MAKE_RGB15 (0x07, 0x00, 0x00), 0x2F);
								break;
							case 2:
								r.corner.x = CAPTAIN_XOFFS + RES_SCALE (18);
								i = CAPTAIN_WIDTH - RES_SCALE ((20 + 1) << 1);
								c = BUILD_COLOR (MAKE_RGB15 (0x1B, 0x00, 0x00), 0x2A);
								break;
							case 3:
								r.corner.x = CAPTAIN_XOFFS + RES_SCALE (21);
								i = CAPTAIN_WIDTH - RES_SCALE ((24 + 1) << 1);
								c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x29);
								break;
							case 4:
								r.corner.x = CAPTAIN_XOFFS + RES_SCALE (25);
								i = RES_SCALE (1);
								r.extent.width = RES_SCALE (2);
								c = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x50, 0x05), 0x28);
								break;
							default:
								// Should not happen.
								c = UNDEFINED_COLOR;  // Keeping compiler quiet.
								break;
							}
							DrawFilledRectangle (&r);
							r.corner.x += i + r.extent.width;
							DrawFilledRectangle (&r);
							r.corner.x -= i;
							r.extent.width = i;
						}
						else
						{
							if ((i -= 5) > 2)
								c = BLACK_COLOR;
							else
							{
								static const Color flash_tab2[] =
								{
									BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
									BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),
									BUILD_COLOR (MAKE_RGB15_INIT (0x0B, 0x00, 0x00), 0x2E),
								};

								c = flash_tab2[i];
							}
							r.corner.x = CAPTAIN_XOFFS
								+ (CAPTAIN_WIDTH >> 1) - IF_HD (2);
							r .corner.y = y + CAPTAIN_YOFFS
								+ ((CAPTAIN_HEIGHT + RES_SCALE (1)) >> 1) - IF_HD (2);
							r.extent.width = RES_SCALE (1);
							r.extent.height = RES_SCALE (1);
						}
					}
					SetContextForeGroundColor (c);
					DrawFilledRectangle (&r);
				}
			}
		}

		old_status_flags = StarShipPtr->old_status_flags;
		old_status_flags = (old_status_flags ^ cur_status_flags) &
				(LEFT | RIGHT | THRUST | WEAPON | SPECIAL | LOW_ON_ENERGY);

		if (old_status_flags)
		{
			if (old_status_flags & LOW_ON_ENERGY)
			{
				if (!(cur_status_flags & LOW_ON_ENERGY))
					DrawCrewFuelString (y, 1);
				else
					DrawCrewFuelString (y, -1);
			}
		}

		StarShipPtr->old_status_flags = cur_status_flags;
	}
}

