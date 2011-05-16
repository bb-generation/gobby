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

#ifndef _GOBBY_PINNING_HPP_
#define _GOBBY_PINNING_HPP_

#include "preferences.hpp"
#include "pinningentry.hpp"

#include <map>

#include <glibmm/ustring.h>

#include <libinfinity/common/inf-xmpp-connection.h>


namespace Gobby
{

class Pinning
{
public:
	Pinning(Preferences preferences): m_preferences(preferences) { }
	
	std::list<PinningEntry> load_saved_entries();

	void save_entry(InfXmppConnection* connection);

	void remove_entry(InfXmppConnection* connection);

	PinningEntry* get_entry(InfXmppConnection* connection);

protected:
	typedef std::map<InfXmppConnection*, PinningEntry> PinningEntryMap;
	PinningEntryMap m_pinning_entries;
	Preferences m_preferences;

};


} // namespace Gobby

#endif // _GOBBY_PINNING_HPP_
