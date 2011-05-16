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

#include <gtkmm/window.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/widget.h>
#include <gtkmm/cellrendererspin.h>
#include <gtkmm/stock.h>
#include <gtkmm/cellrendererpixbuf.h>
#include <gtkmm/treemodel.h>

#include <libinfinity/common/inf-xmpp-connection.h>
#include <libinfinity/client/infc-browser.h>
#include <libinfinity/inf-config.h>
#include <libinfgtk/inf-gtk-browser-view.h>
#include <libinfgtk/inf-gtk-browser-model.h>
#include <libinfgtk/inf-gtk-browser-model-sort.h>


namespace Gobby
{

class Pinning
{
public:
	Pinning(Preferences& preferences);
	
	std::list<InfXmppConnection*> get_saved_connections();

	void save_entry(InfXmppConnection* connection);

	void remove_entry(InfXmppConnection* connection);

	PinningEntry* get_entry(InfXmppConnection* connection);

protected:
	typedef std::map<InfXmppConnection*, PinningEntry*> PinningEntryMap;
	typedef std::map<InfXmppConnection*, PinningEntry*>::iterator PinningEntryMapIterator;
	PinningEntryMap m_pinning_entries;
  Preferences& m_preferences;
};

class CellRendererPixbuf : public Gtk::CellRendererPixbuf
{
public:

	CellRendererPixbuf(Pinning& pinning);

	bool
	activate_vfunc(GdkEvent* event, Gtk::Widget& widget,
	               const Glib::ustring& path,
	               const Gdk::Rectangle& background_area,
	               const Gdk::Rectangle& cell_area,
	               Gtk::CellRendererState flags);

	void
	status_icon_data_func(Gtk::CellRenderer* ren,
	                      Gtk::TreeModel::iterator iter,
	                      InfGtkBrowserModelSort* model);

private:
	Pinning& m_pinning;
};

} // namespace Gobby

#endif // _GOBBY_PINNING_HPP_
