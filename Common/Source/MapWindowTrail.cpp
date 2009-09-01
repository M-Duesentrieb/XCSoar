/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "MapWindow.h"
#include "XCSoar.h"
#include "SnailTrail.hpp"
#include "InfoBoxLayout.h"
#include <math.h>
#include <stdlib.h>
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Screen/Util.hpp"
#include "McReady.h"
#include "OnLineContest.h"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Screen/Graphics.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"

#define fSnailColour(cv) max(0,min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

// This function is slow...
double MapWindow::DrawTrail(Canvas &canvas, const RECT rc)
{
  int i, snail_index;
  SNAIL_POINT P1;
  static BOOL last_circling = FALSE;
  static float vario_max= 5.0;
  static float vario_min= -5.0;
  static bool need_colour = true;

  double TrailFirstTime = -1;

  if(!TrailActive)
    return -1;

  mutexGlideComputer.Lock();

  if ((DisplayMode == dmCircling) != last_circling) {
    need_colour = true;
  }
  last_circling = (DisplayMode == dmCircling);

  //////////// Trail drift calculations

  double traildrift_lat = 0.0;
  double traildrift_lon = 0.0;

  if (EnableTrailDrift && (DisplayMode == dmCircling)) {
    double tlat1, tlon1;

    FindLatitudeLongitude(DrawInfo.Latitude,
                          DrawInfo.Longitude,
                          DerivedDrawInfo.WindBearing,
                          DerivedDrawInfo.WindSpeed,
                          &tlat1, &tlon1);
    traildrift_lat = (DrawInfo.Latitude-tlat1);
    traildrift_lon = (DrawInfo.Longitude-tlon1);
  } else {
    traildrift_lat = 0.0;
    traildrift_lon = 0.0;
  }

  // JMW don't draw first bit from home airport

  /////////////  Trail size

  int num_trail_max;
  if (TrailActive!=2) {
    num_trail_max = TRAILSIZE;
  } else {
    num_trail_max = TRAILSIZE/TRAILSHRINK;
  }
  if ((DisplayMode == dmCircling)) {
    num_trail_max /= TRAILSHRINK;
  }

  ///////////// Vario colour scaling

  double MACCREADY = GlidePolar::GetMacCready();
  float this_vario_max = (float)(1.5*min(5.0, max(MACCREADY,0.5)));
  float this_vario_min = (float)(-1.5*min(5.0, max(MACCREADY,2.0)));
  vario_max = this_vario_max;
  vario_min = this_vario_min;

  ///////////// Snail skipping

  const int skip_divisor = num_trail_max/5;
  int skip_border = skip_divisor;
  int skip_level= 3; // TODO code: try lower level?

  int snail_offset = TRAILSIZE+GlideComputer::snail_trail.getIndex()-num_trail_max;
  while (snail_offset>= TRAILSIZE) {
    snail_offset -= TRAILSIZE;
  }
  while (snail_offset< 0) {
    snail_offset += TRAILSIZE;
  }
  const int zero_offset = (TRAILSIZE-snail_offset);
  skip_border += zero_offset % skip_level;

  int index_skip = ((int)DrawInfo.Time)%skip_level;

  // TODO code: Divide by time step cruise/circling for zero_offset

  ///////////// Keep track of what's drawn

  bool this_visible = true;
  bool last_visible = false;
  POINT point_lastdrawn;
  point_lastdrawn.x = 0;
  point_lastdrawn.y = 0;

  ///////////// Average colour display for skipped points
  float vario_av = 0;
  int vario_av_num = 0;

  ///////////// Constants for speedups

  const bool display_circling = DisplayMode == dmCircling;
  const double display_time = DrawInfo.Time;

  // expand bounds so in strong winds the appropriate snail points are
  // still visible (since they are being tested before drift is applied)
  // this expands them by one minute

  rectObj bounds_thermal = screenbounds_latlon;
  screenbounds_latlon.minx -= fabs(60.0*traildrift_lon);
  screenbounds_latlon.maxx += fabs(60.0*traildrift_lon);
  screenbounds_latlon.miny -= fabs(60.0*traildrift_lat);
  screenbounds_latlon.maxy += fabs(60.0*traildrift_lat);

  const rectObj bounds = bounds_thermal;

  const int deg = DEG_TO_INT(AngleLimit360(DisplayAngle));
  const int cost = ICOSTABLE[deg];
  const int sint = ISINETABLE[deg];
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = GetLonLatToScreenScale();
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;

  ////////////// Main loop

  for(i=1;i< num_trail_max; ++i)
  {
    ///// Handle skipping

    if (i>=skip_border) {
      skip_level= max(1,skip_level-1);
      skip_border= i+2*(zero_offset % skip_level)+skip_divisor;
      index_skip = skip_level;
    }

    index_skip++;
    if ((i<num_trail_max-10) && (index_skip < skip_level)) {
      continue;
    } else {
      index_skip=0;
    }

    ////// Find the snail point

    snail_index = snail_offset+i;
    while (snail_index>=TRAILSIZE) {
      snail_index-= TRAILSIZE;
    }

    P1 = GlideComputer::snail_trail.getPoint(snail_index);

    /////// Mark first time of display point

    if (((TrailFirstTime<0) || (P1.Time<TrailFirstTime)) && (P1.Time>=0)) {
      TrailFirstTime = P1.Time;
    }

    //////// Ignoring display elements for modes

    if (display_circling) {
      if ((!P1.Circling)&&( i<num_trail_max-60 )) {
        // ignore cruise mode lines unless very recent
	last_visible = false;
        continue;
      }
    } else {
      //  if ((P1.Circling)&&( snail_index % 5 != 0 )) {
        // JMW TODO code: This won't work properly!
        // draw only every 5 points from circling when in cruise mode
	//        continue;
      //      }
    }

    ///////// Filter if far visible

    if (!P1.FarVisible) {
      last_visible = false;
      continue;
    }

    ///////// Determine if this is visible

    this_visible =   ((P1.Longitude> bounds.minx) &&
		     (P1.Longitude< bounds.maxx) &&
		     (P1.Latitude> bounds.miny) &&
		     (P1.Latitude< bounds.maxy)) ;

    if (!this_visible && !last_visible) {
      last_visible = false;
      continue;
    }

    ////////// Find coordinates on screen after applying trail drift

    // now we know either point is visible, better get screen coords
    // if we don't already.

    double dt = max(0,(display_time-P1.Time)*P1.DriftFactor);
    double this_lon = P1.Longitude+traildrift_lon*dt;
    double this_lat = P1.Latitude+traildrift_lat*dt;

#if 1
    // this is faster since many parameters are const
    int Y = Real2Int((mPanLatitude-this_lat)*mDrawScale);
    int X = Real2Int((mPanLongitude-this_lon)*fastcosine(this_lat)*mDrawScale);
    P1.Screen.x = (xxs-X*cost + Y*sint)/1024;
    P1.Screen.y = (Y*cost + X*sint + yys)/1024;
#else
    LonLat2Screen(this_lon,
		  this_lat,
		  P1.Screen);
#endif

    ////////// Determine if we should skip if close to previous point

    if (last_visible && this_visible) {
      // only average what's visible

      if (abs(P1.Screen.y-point_lastdrawn.y)
	  +abs(P1.Screen.x-point_lastdrawn.x)<IBLSCALE(4)) {
	vario_av += P1.Vario;
	vario_av_num ++;
	continue;
	// don't draw if very short line
      }
    }

    ////////// Lookup the colour if it's not already set

    if ((P1.Colour<0)||(P1.Colour>=NUMSNAILCOLORS)) {
      float colour_vario = P1.Vario;
      if (vario_av_num) {
	// set color to average if skipped
	colour_vario = (colour_vario+vario_av)/(vario_av_num+1);
	vario_av_num= 0;
	vario_av= 0;
      }
      if (colour_vario<0) {
	colour_vario /= (-vario_min); // JMW fixed bug here
      } else {
	colour_vario /= vario_max;
      }
      P1.Colour = fSnailColour(colour_vario);
    }
    canvas.select(MapGfx.hSnailPens[P1.Colour]);

    if (!last_visible) { // draw set cursor at P1
#ifndef NOLINETO
      canvas.move_to(P1.Screen.x, P1.Screen.y);
#endif
    } else {
#ifndef NOLINETO
      canvas.line_to(P1.Screen.x, P1.Screen.y);
#else
      canvas.clipped_line(P1.Screen, point_lastdrawn, rc);
#endif
    }
    point_lastdrawn = P1.Screen;
    last_visible = this_visible;
  }

  // draw final point to glider
  if (last_visible) {
#ifndef NOLINETO
    canvas.line_to(Orig_Aircraft.x, Orig_Aircraft.y);
#else
    canvas.clipped_line(Orig_Aircraft, point_lastdrawn, rc);
#endif
  }

  mutexGlideComputer.Unlock();

  return TrailFirstTime;
}


void MapWindow::DrawTrailFromTask(Canvas &canvas, const RECT rc,
				  const double TrailFirstTime) {
  static POINT ptin[MAXCLIPPOLYGON];

  if((TrailActive!=3) || (DisplayMode == dmCircling) || (TrailFirstTime<0))
    return;

  const double mTrailFirstTime = TrailFirstTime - DerivedDrawInfo.TakeOffTime;
  // since olc keeps track of time wrt takeoff

  mutexGlideComputer.Lock();
  GlideComputer::olc.SetLine();
  int n = min(MAXCLIPPOLYGON, GlideComputer::olc.getN());
  int i, j=0;
  for (i=0; i<n; i++) {
    if (GlideComputer::olc.getTime(i)>= mTrailFirstTime)
      break;
    LonLat2Screen(GlideComputer::olc.getLongitude(i),
                  GlideComputer::olc.getLatitude(i),
                  ptin[j]);
    j++;
  }
  mutexGlideComputer.Unlock();
  if (j>=2) {
    canvas.select(MapGfx.hSnailPens[NUMSNAILCOLORS / 2]);
    canvas.clipped_polygon(ptin, j, rc, false);
  }
}


