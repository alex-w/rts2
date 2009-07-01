/* 
 * Utility classes for record values.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include <list>
#include <string>

namespace rts2db
{

/**
 * Class representing a record (value).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Record
{
	private:
		double rectime;
		double val;
	public:
		Record (double _rectime, double _val)
		{
			rectime = _rectime;
			val = _val;
		}

		double getRecTime () { return rectime; };
		double getValue () { return val; };
};

/**
 * Class with value records.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class RecordsSet: public std::list <Record>
{
	private:
		int recval_id;
	public:
		RecordsSet (int _recval_id)
		{
			recval_id = _recval_id;
		}

		/**
		 * @throw SqlError on errror.
		 */
		void load (double t_from, double t_to);
};


}
