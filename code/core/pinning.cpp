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

#include "pinning.hpp"
#include "iconmanager.hpp"
#include <gtkmm/treeview.h>

////////////////////// CellRendererPixBuf ////////////////////////////
void Gobby::CellRendererPixbuf::status_icon_data_func(
						Gtk::CellRenderer* ren,
						Gtk::TreeModel::iterator iter,
						InfGtkBrowserModelSort* model)
{
	Gtk::TreeRow row = *iter;

	InfcBrowser* browser;
	InfcBrowserIter* browser_iter;

	this->set_visible(true);

	gtk_tree_model_get(
				  GTK_TREE_MODEL(model),
				  iter.gobj(),
				  INF_GTK_BROWSER_MODEL_COL_BROWSER, &browser,
				  INF_GTK_BROWSER_MODEL_COL_NODE, &browser_iter,
				  -1
				   );


	if(row.parent())
	{
		//parent exists -> no server entry

		if(infc_browser_iter_is_subdirectory(browser, browser_iter))
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
				GTK_STOCK_DIRECTORY;
		else
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
				GTK_STOCK_FILE;

	}
	else
	{ //server entry
		InfXmlConnection* con = infc_browser_get_connection(browser);
		if(m_pinning.get_entry(INF_XMPP_CONNECTION(con)) == 0)
		{
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
			((Gtk::StockID)IconManager::STOCK_SERVER_UNSTARRED)
			.get_string();
		}
		else
		{
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
			((Gtk::StockID)IconManager::STOCK_SERVER_STARRED)
			.get_string();
		}
	}
	infc_browser_iter_free(browser_iter);
	g_object_unref(G_OBJECT(browser));

}



Gobby::Pinning::Pinning(Preferences& preferences) :
	m_preferences(preferences)
{

}

std::list<InfXmppConnection*>
Gobby::Pinning::get_saved_connections()
{
	std::list<InfXmppConnection*> rlist;
	PinningEntryMapIterator it = m_pinning_entries.begin();
	for(;it != m_pinning_entries.end(); ++it)
	{
		rlist.push_back(it->first);
	}

	return rlist;
}

void
Gobby::Pinning::save_entry(InfXmppConnection* connection)
{
	PinningEntry* entry = new PinningEntry(connection);

	m_pinning_entries.insert(std::make_pair(connection, entry));
}

void Gobby::Pinning::remove_entry(InfXmppConnection* connection)
{
	m_pinning_entries.erase(connection);
}

Gobby::PinningEntry*
Gobby::Pinning::get_entry(InfXmppConnection* connection)
{
	PinningEntryMapIterator it = m_pinning_entries.find(connection);
	if(it == m_pinning_entries.end())
		return 0;
	else
		return it->second;
}


Gobby::CellRendererPixbuf::CellRendererPixbuf(Pinning& pinning)
	: Gtk::CellRendererPixbuf::CellRendererPixbuf(),
	  m_pinning(pinning)
{
	this->property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
}

bool Gobby::CellRendererPixbuf::activate_vfunc(
		    GdkEvent* event,
		    Gtk::Widget& widget,
		    const Glib::ustring& path,
		    const Gdk::Rectangle& background_area,
		    const Gdk::Rectangle& cell_area,
		    Gtk::CellRendererState flags)
{
	Gtk::TreePath gpath(path);
	int x, y, width, height;
	int click_x = ((GdkEventButton*)event)->x;
	int click_y = ((GdkEventButton*)event)->y;

	this->get_size(widget, x, y, width, height);

	x += cell_area.get_x();	//get absolute coordinates from tree view
	y += cell_area.get_y();

	if(gpath.size() == 1 &&
	   click_x > x && click_x < (x+width) &&
	   click_y > y && click_y < (y+height)) {
		//root node & click on renderer

		Gtk::TreeView& view = static_cast<Gtk::TreeView&>(widget);

		GtkTreeModel* model = gtk_tree_view_get_model(view.gobj());

		GtkTreeIter iter;
		InfcBrowser* browser;
		gtk_tree_model_get_iter(model, &iter, gpath.gobj());

		gtk_tree_model_get(model, &iter,
					   INF_GTK_BROWSER_MODEL_COL_BROWSER,
					   &browser, -1);

		InfXmlConnection* con = infc_browser_get_connection(browser);
		if(m_pinning.get_entry(INF_XMPP_CONNECTION(con)) == 0)
		{
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
				((Gtk::StockID)IconManager::STOCK_SERVER_UNSTARRED)
				.get_string();
			m_pinning.save_entry(INF_XMPP_CONNECTION(con));
		}
		else
		{
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() =
				((Gtk::StockID)IconManager::STOCK_SERVER_STARRED)
				.get_string();
			m_pinning.remove_entry(INF_XMPP_CONNECTION(con));
		}


		g_object_unref(browser);

	}

	return false;
}
