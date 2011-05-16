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

#include "pinningentry.hpp"

Gobby::PinningEntry::PinningEntry()
{
}

Gobby::PinningEntry::PinningEntry(InfXmppConnection* connection)
{
	// TODO
}

Gobby::PinningEntry::~PinningEntry()
{
}

void
Gobby::PinningEntry::set_property(PinningProperty name, Glib::ustring value)
{
	m_properties[name] = value;
}

Glib::ustring
Gobby::PinningEntry::get_property(PinningProperty name) const
{
	PropertyMapIterator it = m_properties.find(name);
	if(it != m_properties.end())
		return it->second;
	else
		return "";
}

