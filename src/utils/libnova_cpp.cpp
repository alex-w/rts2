/* 
 * libnova C++ extension.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek,net>
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
 */

#include "libnova_cpp.h"

// (this is in libnova_cpp.h) #include <math.h>
#include <iomanip>
#include <sstream>

double
timetFromJD (double JD)
{
  time_t t;
  ln_get_timet_from_julian (JD, &t);
  return t;
}

void
LibnovaRa::toHms (struct ln_hms *ra_hms)
{
  ln_deg_to_hms (ra, ra_hms);
}

void
LibnovaRa::fromHms (struct ln_hms *ra_hms)
{
  ra = ln_hms_to_deg (ra_hms);
}

void
LibnovaRa::flip ()
{
  ra = ln_range_degrees (ra + 180);
}

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra)
{
  if (isnan (l_ra.ra))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_hms ra_hms;
  l_ra.toHms (&ra_hms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (3);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << std::setw (2) << ra_hms.hours << " "
    << std::setw (2) << ra_hms.minutes << " "
    << std::setw (6) << ra_hms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::istream & operator >> (std::istream & _is, LibnovaRa & l_ra)
{
  struct ln_hms hms;
  _is >> hms.hours >> hms.minutes >> hms.seconds;
  l_ra.fromHms (&hms);
  return _is;
}

std::ostream & operator << (std::ostream & _os, LibnovaRaJ2000 l_ra)
{
  _os << l_ra.getRa () << " (" << LibnovaRa (l_ra) << ")";
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaHaM l_haM)
{
  struct ln_hms hms;
  l_haM.toHms (&hms);
  int old_precison = _os.precision (3);
  char old_fill = _os.fill ('0');
  _os << std::setw (2) << hms.hours << " "
    << std::setw (6) << (hms.minutes + hms.seconds / 60.0);
  _os.fill (old_fill);
  _os.precision (old_precison);
  return _os;
}

std::istream & operator >> (std::istream & _is, LibnovaHaM & l_haM)
{
  struct ln_hms hms;
  _is >> hms.hours >> hms.seconds;
  hms.minutes = (unsigned short int) floor (hms.seconds);
  hms.seconds -= hms.minutes;
  hms.seconds *= 60.0;
  l_haM.fromHms (&hms);
  return _is;
}

std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra)
{
  if (isnan (l_ra.ra))
    {
      _os << std::setw (6) << "nan";
      return _os;
    }
  struct ln_hms ra_hms;
  l_ra.toHms (&ra_hms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (3);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << std::setw (2) << ra_hms.hours
    << std::setw (2) << ra_hms.minutes << std::setw (6) << ra_hms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}


void
LibnovaDeg::toDms (struct ln_dms *deg_dms)
{
  ln_deg_to_dms (deg, deg_dms);
}

void
LibnovaDeg::fromDms (struct ln_dms *deg_dms)
{
  deg = ln_dms_to_deg (deg_dms);
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (13) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (3) << deg_dms.degrees << " "
    << std::setw (2) << deg_dms.minutes << " "
    << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}


/*
 * operator >>
 *
 * Behaves sensibly if given deg >= 360, min >= 60, sec >= 60
 */
std::istream & operator >> (std::istream & _is, LibnovaDeg & l_deg)
{
  double res = 0;
  bool neg = false;

  // divider for degrees conversion
  int unit = 1;
  // unfortuantelly we have to parse one by one
  while (_is.good ())
    {
      int next = _is.peek ();
      // ignore spaces..
      if (isspace (next) || next == ':')
	{
	  _is.get ();
	  continue;
	}
      if (next == '-' || next == '+')
	{
	  if (unit == 1)
	    {
	      neg = (next == '-') ? true : false;
	      _is.get ();
	    }
	  else
	    {
	      // it's beggingin of next deg..
	      l_deg.fromNegDouble (neg, res);
	      return _is;
	    }
	}
      // otherwise, try to get number..
      double t_deg;
      _is >> t_deg;
      if (_is.fail ())
	{
	  if (unit != 1 && (_is.eof ()))
	    {
	      l_deg.fromNegDouble (neg, res);
	      return _is;
	    }
	  else
	    {
	      return _is;
	    }
	}
      res += t_deg / unit;
      // seconds..
      if (unit == 3600)
	{
	  l_deg.fromNegDouble (neg, res);
	  return _is;
	}
      unit *= 60;
    }
  l_deg.fromNegDouble (neg, res);
  return _is;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (12) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << std::setw (1) << (deg_dms.neg ? '-' : '+')
    << std::setw (2) << deg_dms.degrees << " "
    << std::setw (2) << deg_dms.minutes << " "
    << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg360 l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os
    << std::setw (3) << deg_dms.degrees << " "
    << std::setw (2) << deg_dms.minutes << " "
    << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg180 l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  if (l_deg.deg > 180.0)
    l_deg.deg = l_deg.deg - 360.0;
  l_deg.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os
    << std::setw (1) << (deg_dms.neg ? '-' : '+')
    << std::setw (3) << deg_dms.degrees << " "
    << std::setw (2) << deg_dms.minutes << " "
    << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

void
LibnovaDec::flip ()
{
  if (deg > 0)
    deg = 180 - deg;
  else
    deg = -180 - deg;
}

std::ostream & operator << (std::ostream & _os, LibnovaDec l_dec)
{
  if (isnan (l_dec.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_dec.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (2) << deg_dms.degrees << " "
    << std::setw (2) << deg_dms.minutes << " "
    << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDecJ2000 l_dec)
{
  _os << l_dec.getDec () << " (" << LibnovaDec (l_dec) << ")";
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDeg90Comp l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (7) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (0);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os << (deg_dms.neg ? '-' : '+')
    << std::setw (2) << deg_dms.degrees
    << std::setw (2) << deg_dms.minutes << std::setw (2) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}


std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  std::ios_base::fmtflags old_settings =
    _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  int old_precison = _os.precision (2);
  if (deg_dms.degrees == 0 && deg_dms.minutes == 0)
    {
      _os << "   " << (deg_dms.neg ? '-' : '+') << "0'";
    }
  else
    {
      _os << std::setw (5)
	<< std::showpos << ((deg_dms.neg ? -1 : 1) *
			    (deg_dms.degrees * 60 +
			     deg_dms.minutes)) << std::noshowpos << "'";
    }
  char old_fill = _os.fill ('0');
  _os << std::setw (5) << deg_dms.seconds;
  _os.fill (old_fill);
  _os.precision (old_precison);
  _os.setf (old_settings);
  return _os;
}


std::ostream & operator << (std::ostream & _os, LibnovaDegDist l_deg)
{
  if (isnan (l_deg.deg))
    {
      _os << std::setw (11) << "nan";
      return _os;
    }
  struct ln_dms deg_dms;
  l_deg.toDms (&deg_dms);
  int old_precison = _os.precision (2);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  if (deg_dms.degrees == 0 && deg_dms.minutes == 0)
    {
      _os << "   0'";
    }
  else
    {
      _os << std::setw (5) << (deg_dms.degrees * 60 + deg_dms.minutes) << "'";
    }
  char old_fill = _os.fill ('0');
  _os << std::setw (5) << deg_dms.seconds;
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::istream & operator >> (std::istream & _is, LibnovaDegDist & l_deg)
{
  double val;
  double out = 0;
  double step = 1;
  bool get_something = false;
  std::ostringstream * os = NULL;
  std::istringstream * is = NULL;
  while (1)
    {
      char peek_val = _is.peek ();
      if (isdigit (peek_val)
	  || peek_val == '.' || peek_val == '-' || peek_val == '+')
	{
	  if (!os)
	    os = new std::ostringstream ();
	  (*os) << peek_val;
	  _is.get ();
	  continue;
	}
      switch (_is.peek ())
	{
	case 'h':
	  step = 15;
	case ' ':
	case ':':
	case 'o':
	case 'd':
	case 'm':
	case 's':
	  delete is;
	  // eat character
	  _is.get ();
	  if (os)
	    {
	      is = new std::istringstream (os->str ());
	      (*is) >> val;
	    }
	  else
	    {
	      continue;
	    }
	  get_something = true;
	  delete os;
	  os = NULL;
	  break;
	default:
	  if (get_something)
	    l_deg.deg = out;
	  return _is;
	}
      if (out == 0 && val < 0)
	step *= -1;
      out += val / step;
      step *= 60.0;
    }
}

void
LibnovaRaDec::flip ()
{
  if (ra)
    ra->flip ();
  if (dec)
    dec->flip ();
}

std::ostream & operator << (std::ostream & _os, LibnovaRaDec l_radec)
{
  if (l_radec.ra && l_radec.dec)
    _os << *(l_radec.ra) << " " << *(l_radec.dec);
  else
    _os << "nan nan";
  return _os;
}

std::istream & operator >> (std::istream & _is, LibnovaRaDec & l_radec)
{
  delete l_radec.ra;
  delete l_radec.dec;
  l_radec.ra = new LibnovaRa ();
  l_radec.dec = new LibnovaDec ();
  _is >> (*(l_radec.ra)) >> (*(l_radec.dec));
  return _is;
}

std::ostream & operator << (std::ostream & _os, LibnovaHrz l_hrz)
{
  if (l_hrz.alt && l_hrz.az)
    _os << *(l_hrz.alt) << " " << *(l_hrz.az);
  else
    _os << "nan nan";
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaDate l_date)
{
  char old_fill = _os.fill ('0');
  int old_precison = _os.precision (3);
  std::ios_base::fmtflags old_settings = _os.flags ();
  _os.setf (std::ios_base::fixed, std::ios_base::floatfield);
  _os << std::setw (4) << l_date.date.years << "/"
    << std::setw (2) << l_date.date.months << "/"
    << std::setw (2) << l_date.date.days << " "
    << std::setw (2) << l_date.date.hours << ":"
    << std::setw (2) << l_date.date.minutes << ":"
    << std::setw (6) << l_date.date.seconds << " UT";
  _os.setf (old_settings);
  _os.precision (old_precison);
  _os.fill (old_fill);
  return _os;
}

std::istream & operator >> (std::istream & _is, LibnovaDate & l_date)
{
  char ch;
  _is >> l_date.date.years >> ch >> l_date.date.months >> ch >> l_date.date.
    days >> l_date.date.hours >> ch >> l_date.date.
    minutes >> ch >> l_date.date.seconds;
  return _is;
}

Rts2Night::Rts2Night (struct ln_date * ln_night, struct ln_lnlat_posn * obs)
{
  struct ln_date ln_tmp;
  // let's calculate time from..t_from will contains start of night
  // local 12:00 will be at ~ give time..
  ln_tmp = *ln_night;
  ln_tmp.hours = (int) ln_range_degrees (180.0 - obs->lng) / 15;
  ln_tmp.minutes = 0;
  ln_tmp.seconds = 0;
  ln_get_timet_from_julian (ln_get_julian_day (&ln_tmp), &from);
  to = from + 86400;
}

Rts2Night::Rts2Night (double JD, struct ln_lnlat_posn *obs)
{
  struct ln_date l_date;
  // let's calculate time from..t_from will contains start of night
  // local 12:00 will be at ~ give time..
  ln_get_date (JD, &l_date);
  l_date.hours = (int) ln_range_degrees (180.0 - obs->lng) / 15;
  l_date.minutes = 0;
  l_date.seconds = 0;
  JD = ln_get_julian_day (&l_date);
  ln_get_timet_from_julian (JD, &from);
  to = from + 86400;
}

std::ostream & operator << (std::ostream & _os, Rts2Night night)
{
  _os << LibnovaDate (night.getFrom ())
    << " - " << LibnovaDate (night.getTo ());
  return _os;
}

std::ostream & operator << (std::ostream & _os, LibnovaPos l_pos)
{
  _os
    << LibnovaDeg (l_pos.getLongitude ())
    << (l_pos.getLongitude () > 0 ? " E," : " W,")
    << LibnovaDeg90 (l_pos.getLatitude ())
    << (l_pos.getLatitude () > 0 ? " N" : " S");
  return _os;
}
