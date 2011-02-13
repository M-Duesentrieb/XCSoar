/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "NMEA/Derived.hpp"
#include "Defines.h"

void
VARIO_INFO::Clear()
{
  CruiseLD = fixed(INVALID_GR);
  AverageLD = fixed(INVALID_GR);
  LD = fixed(INVALID_GR);
  LDvario = fixed(INVALID_GR);
}

void
CLIMB_INFO::ClearPartial()
{
  // If you load persistent values, you need at least these reset:
  LastThermalAverage = fixed_zero;
  LastThermalAverageSmooth = fixed_zero;
  ThermalGain = fixed_zero;
}

void
CLIMB_INFO::Clear()
{
  ClearPartial();

  ThermalAverage = fixed_zero;
}

void
CIRCLING_INFO::ClearPartial()
{
  Circling = false;
}

void
CIRCLING_INFO::Clear()
{
  ClearPartial();

  timeCruising = fixed_zero;
  timeCircling = fixed_zero;
  TotalHeightClimb = fixed_zero;

  CruiseStartTime = fixed_minus_one;
  ClimbStartTime = fixed_minus_one;

  MinAltitude = fixed_zero;
  MaxHeightGain = fixed_zero;
}

void
TERRAIN_ALT_INFO::Clear()
{
  for (int i = 0; i <= TERRAIN_ALT_INFO::NUMTERRAINSWEEPS; i++) {
    GlideFootPrint[i].Longitude = Angle::native(fixed_zero);
    GlideFootPrint[i].Latitude = Angle::native(fixed_zero);
  }

  TerrainWarning = false;

  AltitudeAGL = fixed_zero;
}

void
CLIMB_HISTORY_INFO::Clear()
{
  for (unsigned i = 0; i < 200; i++) {
    AverageClimbRate[i] = fixed_zero;
    AverageClimbRateN[i] = 0;
  }
}

void
DERIVED_INFO::ResetFlight(bool full)
{
  if (full) {
    VARIO_INFO::Clear();
    CLIMB_INFO::Clear();
    CIRCLING_INFO::Clear();
    CLIMB_HISTORY_INFO::Clear();
  } else {
    CLIMB_INFO::ClearPartial();
    CIRCLING_INFO::ClearPartial();
  }

  thermal_band.clear();

  thermal_locator.Clear();

  TERRAIN_ALT_INFO::Clear();
}
