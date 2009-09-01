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
#if !defined(XCSOAR_MAPWINDOW_H)
#define XCSOAR_MAPWINDOW_H

#include "XCSoar.h"
#include "MapWindowProjection.hpp"
#include "Airspace.h"
#include "Trigger.hpp"
#include "Mutex.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/MaskedPaintWindow.hpp"
#include "Screen/LabelBlock.hpp"

class MapWindowBase {
 public:
  MapWindowBase():
    dirtyEvent(TEXT("mapDirty")),
    window_initialised(false),
    hDrawThread(0),
    dwDrawThreadID(0) {};

  Trigger  dirtyEvent;
  void     CloseDrawingThread(void);
  void     SuspendDrawingThread(void);
  void     ResumeDrawingThread(void);
  bool     IsDisplayRunning();
  void     CreateDrawingThread(void);
  Mutex    mutexRun;
 protected:
  DWORD    dwDrawThreadID;
  HANDLE   hDrawThread;
  Mutex    mutexStart;
  bool     window_initialised;
};


class MapWindowBlackboard {
 public:
  NMEA_INFO     DrawInfo;
  DERIVED_INFO  DerivedDrawInfo;
 protected:
  void ExchangeBlackboard(const NMEA_INFO &nmea_info,
			  const DERIVED_INFO &derived_info);
};


class MapWindow
: public MaskedPaintWindow, public MapWindowBase,
  public MapWindowProjection,
  public MapWindowBlackboard {
 public:
  MapWindow();

  bool register_class(HINSTANCE hInstance, const TCHAR* szWindowClass);

  // inter-process, used only on file change
  void ForceVisibilityScan() {
    askVisibilityScan = true;
  }

  // used by dlgTarget
  bool TargetDragged(double *longitude, double *latitude);
  bool SetTargetPan(bool dopan, int task_index);

  // use at startup
  void SetMapRect(RECT rc) {
    MapRect = rc;
  }

  bool isMapFullScreen(); // gui feedback

  void RequestToggleFullScreen();
  void RequestFullScreen(bool full);

  // used by topology store
  void ScanVisibility(rectObj *bounds_active);
  bool RenderTimeAvailable(); // used only by TopologyStore.cpp

  // input events or reused code
  void Event_SetZoom(double value);
  void Event_ScaleZoom(int vswitch);
  void Event_Pan(int vswitch);
  void Event_TerrainTopology(int vswitch);
  void Event_AutoZoom(int vswitch);
  void Event_PanCursor(int dx, int dy);

  // used by MapWindowBase
  static DWORD DrawThread (LPVOID);
  DWORD _DrawThread ();

  ////////////////////////////////////////////////////////////////////
 private:

  void DrawThreadLoop (const bool first);
  void DrawThreadInitialise (void);

  // state
  BOOL     Initialised;
  bool     user_asked_redraw;

  void     ExchangeBlackboard(const NMEA_INFO &nmea_info,
			      const DERIVED_INFO &derived_info);

  // display management
  void          RefreshMap();
  void          SwitchZoomClimb(void);

  // state/localcopy/local data
  double        TargetDrag_Latitude;
  double        TargetDrag_Longitude;
  int           TargetDrag_State;
  POINT         Groundline[NUMTERRAINSWEEPS+1];
  bool          LandableReachable;

  // projection
  bool      BigZoom;
  bool      askFullScreen;
  bool      MapFullScreen;
  bool      askVisibilityScan; // called only by XCSoar.cpp on
			              // settings reload
  void      StoreRestoreFullscreen(bool);
  void      ToggleFullScreenStart();

  double    findMapScaleBarSize(const RECT rc);

  // other
  DWORD     fpsTime0;
  DWORD     timestamp_newdata;
  void      UpdateTimeStats(bool start);

  // interface handlers
  int ProcessVirtualKey(int X, int Y, long keytime, short vkmode);

  // display element functions

  void ScanVisibilityWaypoints(rectObj *bounds_active);
  void ScanVisibilityAirspace(rectObj *bounds_active);

  void CalculateScreenPositions(POINT Orig, RECT rc,
                                       POINT *Orig_Aircraft);
  void CalculateScreenPositionsTask();
  void CalculateScreenPositionsWaypoints();
  void CalculateScreenPositionsGroundline();
  void CalculateScreenPositionsAirspace();
  void CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE& circ);
  void CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA& area);
  void CalculateScreenPositionsThermalSources();
  void CalculateWaypointReachable(void);
  bool WaypointInTask(int ind);
  void MapWaypointLabelSortAndRender(Canvas &canvas);

  // display renderers
  void DrawAircraft(Canvas &canvas);
  void DrawCrossHairs(Canvas &canvas, const RECT rc);
  void DrawGlideCircle(Canvas &canvas, const POINT Orig, const RECT rc); // VENTA3
  void DrawBestCruiseTrack(Canvas &canvas);
  void DrawCompass(Canvas &canvas, const RECT rc);
  void DrawHorizon(Canvas &canvas, const RECT rc);
  void DrawWindAtAircraft2(Canvas &canvas, POINT Orig, RECT rc);
  void DrawAirSpace(Canvas &canvas, const RECT rc, Canvas &buffer);
  void DrawWaypoints(Canvas &canvas, const RECT rc);
  void DrawWaypointsNew(Canvas &canvas, const RECT rc); // VENTA5
  void DrawLook8000(Canvas &canvas, const RECT rc); // VENTA5
  void DrawFlightMode(Canvas &canvas, const RECT rc);
  void DrawGPSStatus(Canvas &canvas, const RECT rc);
  double DrawTrail(Canvas &canvas, const RECT rc);
  void DrawTeammate(Canvas &canvas, const RECT rc);
  void DrawTrailFromTask(Canvas &canvas, const RECT rc, const double);
  void DrawOffTrackIndicator(Canvas &canvas, const RECT rc);
  void DrawProjectedTrack(Canvas &canvas, const RECT rc);
  void DrawStartSector(Canvas &canvas, const RECT rc, POINT &Start,
                              POINT &End, int Index);
  void DrawTask(Canvas &canvas, RECT rc);
  void DrawThermalEstimate(Canvas &canvas, const RECT rc);
  void DrawTaskAAT(Canvas &canvas, const RECT rc, Canvas &buffer);
  void DrawAbortedTask(Canvas &canvas, const RECT rc);

  void DrawBearing(Canvas &canvas, const RECT rc, int bBearingValid);
  void DrawMapScale(Canvas &canvas, const RECT rc,
			   const bool ScaleChangeFeedback);
  void DrawMapScale2(Canvas &canvas, const RECT rc);
  void DrawFinalGlide(Canvas &canvas, const RECT rc);
  void DrawThermalBand(Canvas &canvas, const RECT rc);
  void DrawGlideThroughTerrain(Canvas &canvas, const RECT rc);
  void DrawTerrainAbove(Canvas &hDC, const RECT rc, Canvas &buffer);
  void DrawCDI();
  //  void DrawSpeedToFly(HDC hDC, RECT rc);
  void DrawFLARMTraffic(Canvas &canvas, RECT rc);

  void ClearAirSpace(Canvas &dc, bool fill);

  // thread, main functions
  void RenderMapWindow(Canvas &canvas, const RECT rc);
  void RenderMapWindowBg(Canvas &canvas, const RECT rc);
  void UpdateCaches(const bool force=false);

  // graphics vars

  BufferCanvas draw_canvas;
  BufferCanvas buffer_canvas;

  LabelBlock label_block;
 public:
  bool checkLabelBlock(RECT rc);
  LabelBlock *getLabelBlock() {
    return &label_block;
  }
  bool draw_masked_bitmap_if_visible(Canvas &canvas,
				     Bitmap &bitmap,
				     const double &lon,
				     const double &lat,
				     unsigned width,
				     unsigned height,
				     POINT *sc=NULL);
 protected:
  virtual bool on_create();
  virtual bool on_destroy();
  virtual bool on_resize(unsigned width, unsigned height);
  virtual bool on_mouse_double(int x, int y);
  virtual bool on_mouse_move(int x, int y);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_key_down(unsigned key_code);
  virtual void on_paint(Canvas& canvas);

};

#endif
