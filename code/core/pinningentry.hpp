/* Gobby - GTK-based collaborative text editor
 * Copyright (C) 2008-2011 Armin Burgmeier <armin@arbur.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _GOBBY_PINNING_ENTRY_HPP_
#define _GOBBY_PINNING_ENTRY_HPP_

//#include "commands/auth-commands.hpp"

#include <map>

#include <glibmm/ustring.h>

#include <libinfinity/common/inf-xmpp-connection.h>

namespace Gobby
{


class PinningEntry
{
public:
	PinningEntry();
	PinningEntry(InfXmppConnection* connection,
	             Glib::ustring password);

	enum PinningProperty {
		HOST = 0,
		SERVICE = 1,
		DEVICE = 2,
		/* ALIAS, */
		AUTHTYPE = 4, /* (SASL_MECHANISM) */
		/* USERNAME, */
		PASSWORD = 6
	};

	void set_property(PinningProperty name,
			              Glib::ustring value);

	Glib::ustring get_property(PinningProperty name) const;

	~PinningEntry();

	bool operator==(const PinningEntry& other) const;

protected:


	typedef std::map<PinningProperty, Glib::ustring> PropertyMap;
	typedef std::map<PinningProperty, Glib::ustring>::const_iterator PropertyMapIterator;
	PropertyMap m_properties;
};




} // namespace Gobby

#endif // _GOBBY_PINNING_HPP_
