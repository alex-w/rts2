/* 
 * BB API access for RTS2.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_BBAPI__
#define __RTS2_BBAPI__

#include "rts2json/asyncapi.h"
#include "rts2json/jsondb.h"
#include "bbtasks.h"

#include <json-glib/json-glib.h>

namespace rts2bb
{
/**
 * Class for BB API requests.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class BBAPI:public rts2json::JSONDBRequest
{
	public:
		BBAPI (const char* prefix, rts2json::HTTPServer *_http_server, XmlRpc::XmlRpcServer* s, BBTasks *_queue);
		virtual ~BBAPI ();

	private:
		void executeJSON (XmlRpc::XmlRpcSource *source, std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length);

		std::map <int, std::pair <double, JsonParser *> > observatoriesJsons;

		BBTasks *queue;
};

class AsyncObsAPI:public rts2json::AsyncAPI
{
	public:
		AsyncObsAPI (rts2json::JSONRequest *_req, rts2core::Connection *_conn, XmlRpc::XmlRpcServerConnection *_source, bool _ext):rts2json::AsyncAPI (_req, _conn, _source, _ext)
		{
			
		}

		virtual ~AsyncObsAPI () {};

};

}

#endif // __RTS2_BBAPI__
