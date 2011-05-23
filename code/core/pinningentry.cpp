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

#include "commands/auth-commands.hpp"

Gobby::PinningEntry::PinningEntry()
{
}

Gobby::PinningEntry::PinningEntry(InfXmppConnection* connection,
                                  Glib::ustring password)
{
	const gchar* host;
	g_object_get(G_OBJECT(connection),
	             "remote_hostname", &host, NULL);
	set_property(HOST, host);

	InfTcpConnection* tcp_connection;
	g_object_get(G_OBJECT(connection),
	             "tcp-connection", &tcp_connection, NULL);
	guint port = inf_tcp_connection_get_remote_port(tcp_connection);
	set_property(SERVICE, g_strdup_printf("%i", port));
	g_object_unref(tcp_connection);

	const gchar* sasl_mechanism;
	g_object_get(G_OBJECT(connection),
	             "sasl-mechanisms", &sasl_mechanism, NULL);

	Glib::ustring mechanism = Glib::ustring(sasl_mechanism);
	set_property(AUTHTYPE, mechanism);
	
	set_property(PASSWORD, password);


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

