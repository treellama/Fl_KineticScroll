//
// Header file for kinetic scrolling widget for the Fast Light Tool Kit (FLTK)
//
// Copyright 2013 by Gregory Smith
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//

#ifndef Fl_KineticScroll_H
#define Fl_KineticScroll_H

#include <FL/Fl_Scroll.H>

class Fl_KineticScroll : public Fl_Group {
public:
  Fl_KineticScroll(int X, int Y, int W, int H, const char* l = 0);
  
  int handle(int event);
  void resize(int, int, int, int);

  void scroll_to(int X, int Y);
  int yposition() { return yposition_; }
  int xposition() { return xposition_; }

private:
  int xposition_, yposition_;
  int oldx, oldy;

  // maximum scroll limits; calculated on FL_PUSH and cached
  void calculate_bounds();
  int xmax_;
  int ymax_;

  int xposition_start_;
  int event_x_start_;

  int yposition_start_;
  int event_y_start_;

  int event_y_prev_;
  int event_x_prev_;
  unsigned long event_time_prev_;

  double yvelocity_;
  double xvelocity_;

  Fl_Widget* pushed_;
  bool scrolling_;

  static void deaccelerate_callback(void* pv) {
    reinterpret_cast<Fl_KineticScroll*>(pv)->deaccelerate(); 
  }

  void deaccelerate();

  // velocity (pixels / ms) higher than this starts scrolling
  static const double STATIC_FRICTION;

  // pixels/ms per second
  static const double DEACCELERATION;

  // press vs drag detection threshold
  static const int PRESS_THRESHOLD = 2;

  // updates per second
  static const int UPDATE_RATE = 60;
};

#endif
