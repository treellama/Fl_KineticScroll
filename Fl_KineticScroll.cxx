//
// Kinetic scrolling widget for the Fast Light Tool Kit (FLTK)
//
// Copyright 2013 by Gregory Smith
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//

#include "Fl_KineticScroll.H"

#include <FL/Fl.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/fl_draw.H>

#include <limits.h>

const double Fl_KineticScroll::DEACCELERATION = 5.0;
const double Fl_KineticScroll::STATIC_FRICTION = 0.25;

Fl_KineticScroll::Fl_KineticScroll(int X, int Y, int W, int H, const char* l) :
  Fl_Group(X, Y, W, H),
  xposition_(0),
  yposition_(0),
  oldx(0),
  oldy(0),
  scrolling_(false)
{
  clip_children(1);
}

extern unsigned long fl_event_time; // X11 only!

void Fl_KineticScroll::deaccelerate() {
  int xnew = xposition() - (1000.0 / UPDATE_RATE * xvelocity_);
  if (xnew < 0) {
    xvelocity_ = 0;
    xnew = 0;
  }
  if (xnew > xmax_) {
    xvelocity_ = 0;
    xnew = xmax_;
  }

  int ynew = yposition() - (1000.0 / UPDATE_RATE * yvelocity_);
  if (ynew < 0) {
    yvelocity_ = 0;
    ynew = 0;
  }
  if (ynew > ymax_) {
    yvelocity_ = 0;
    ynew = ymax_;
  }

  scroll_to(xnew, ynew);

  if (xvelocity_ > 0) {
    xvelocity_ -= DEACCELERATION / UPDATE_RATE;
    if (xvelocity_ < 0) {
      xvelocity_ = 0;
    }
  } else if (xvelocity_ < 0) {
    xvelocity_ += DEACCELERATION / UPDATE_RATE;
    if (xvelocity_ > 0) {
      xvelocity_ = 0;
    }
  }

  if (yvelocity_ > 0) {
    yvelocity_ -= DEACCELERATION / UPDATE_RATE;
    if (yvelocity_ < 0) {
      yvelocity_ = 0;
    }
  } else if (yvelocity_ < 0) {
    yvelocity_ += DEACCELERATION / UPDATE_RATE;
    if (yvelocity_ > 0) {
      yvelocity_ = 0;
    }
  }

  if (xvelocity_ != 0 || yvelocity_ != 0) {
    Fl::repeat_timeout(1.0 / UPDATE_RATE, &Fl_KineticScroll::deaccelerate_callback, this);
  } else {
    scrolling_ = false;
  }
}

int Fl_KineticScroll::handle(int event) {
  switch (event) {
  case FL_PUSH:
    Fl::remove_timeout(&Fl_KineticScroll::deaccelerate_callback, this);

    calculate_bounds();		// assume group doesn't change while scrolling!

    xposition_start_ = xposition();
    event_x_start_ = Fl::event_x();
    event_x_prev_ = event_x_start_;

    yposition_start_ = yposition();
    event_y_start_ = Fl::event_y();
    event_y_prev_ = event_y_start_;

    event_time_prev_ = fl_event_time;

    xvelocity_ = 0;
    yvelocity_ = 0;

    if (!scrolling_) {
      Fl_Group::handle(event);
      pushed_ = Fl::pushed();
      Fl::pushed(this);
    }
        
    scrolling_ = false;

    return 1;
  case FL_DRAG: {
    int ynew = yposition_start_ + event_y_start_ - Fl::event_y();
    if (ynew < 0) {
      ynew = 0;
    }
    if (ynew > ymax_) {
      ynew = ymax_;
    }
    int xnew = xposition_start_ + event_x_start_ - Fl::event_x();
    if (xnew < 0) {
      xnew = 0;
    }
    if (xnew > xmax_) {
      xnew = xmax_;
    }

    if (!scrolling_) {
      if (yposition_start_ - ynew > PRESS_THRESHOLD ||
	  yposition_start_ - ynew < -PRESS_THRESHOLD ||
	  xposition_start_ - xnew > PRESS_THRESHOLD || 
	  xposition_start_ - xnew < -PRESS_THRESHOLD) {
	// send a fake drag event to the pushed widget, so that it
	// unhighlights
	int x = Fl::event_x();
	int y = Fl::event_y();
	// this hack will stop working if FLTK ever makes these private
	Fl::e_x = pushed_->x() - 1;
	Fl::e_y = pushed_->y() - 1;
	pushed_->handle(FL_DRAG);
	Fl::e_x = x;
	Fl::e_y = y;

	scrolling_ = true;
	redraw();
      } else {
	return pushed_->handle(FL_DRAG);
      }
    }

    int dx = Fl::event_x() - event_x_prev_;
    int dy = Fl::event_y() - event_y_prev_;
    unsigned long time = fl_event_time - event_time_prev_;

    if (time) {
      xvelocity_ = static_cast<double>(dx) / time;
      yvelocity_ = static_cast<double>(dy) / time;
    }

    event_x_prev_ = Fl::event_x();
    event_y_prev_ = Fl::event_y();
    event_time_prev_ = fl_event_time;

    scroll_to(xnew, ynew);
    return 1;
  }
  case FL_RELEASE:
    if (!scrolling_) {
      pushed_->handle(FL_RELEASE);
    }
    if (yvelocity_ > STATIC_FRICTION ||
	yvelocity_ < -STATIC_FRICTION || 
	xvelocity_ > STATIC_FRICTION || 
	xvelocity_ < -STATIC_FRICTION) {
      Fl::add_timeout(1.0 / UPDATE_RATE, &Fl_KineticScroll::deaccelerate_callback, this);
    } else {
      scrolling_ = false;
    }
    return 1;
  }

  return Fl_Group::handle(event);
}

void Fl_KineticScroll::calculate_bounds() {
  int xmin = INT_MAX, xmax = INT_MIN;
  int ymin = INT_MAX, ymax = INT_MIN;

  Fl_Widget* const* a = array();
  for (int i = children(); i--;) {
    Fl_Widget* o = *a++;

    if (o->x() < xmin) {
      xmin = o->x();
    }

    if (o->x() + o->w() > xmax) {
      xmax = o->x() + o->w();
    }

    if (o->y() < ymin) {
      ymin = o->y();
    }

    if (o->y() + o->h() > ymax) {
      ymax = o->y() + o->h();
    }
  }
  
  xmax_ = xmax - xmin - w();
  if (xmax_ < 0) {
    xmax_ = 0;
  }

  ymax_ = ymax - ymin - h();
  if (ymax_ < 0) {
    ymax_ = 0;
  }
}

// copied from Fl_Scroll
void Fl_KineticScroll::resize(int X, int Y, int W, int H) {
  int dx = X-x(), dy = Y-y();
  int dw = W-w(), dh = H-h();
  Fl_Widget::resize(X,Y,W,H); // resize _before_ moving children around
  // move all the children:
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    o->position(o->x()+dx, o->y()+dy);
  }	
}

// copied from Fl_Scroll
void Fl_KineticScroll::scroll_to(int X, int Y) {
  int dx = xposition_-X;
  int dy = yposition_-Y;
  if (!dx && !dy) return;
  xposition_ = X;
  yposition_ = Y;
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    o->position(o->x()+dx, o->y()+dy);
  }
  redraw();
}
