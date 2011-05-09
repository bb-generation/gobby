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
#include <gtkmm/treeview.h>

////////////////////// CellRendererPixBuf ////////////////////////////
void Gobby::CellRendererPixbuf::status_icon_data_func(Gtk::CellRenderer* ren, Gtk::TreeModel::iterator iter, InfGtkBrowserModelSort* model)
{
	Gtk::TreeRow row = *iter;

	InfcBrowser* browser;
	InfcBrowserIter* browser_iter;
	
	this->set_visible(true);
	
	if(row.parent()) {
		//parent exists -> no server entry

		gtk_tree_model_get(
				   GTK_TREE_MODEL(model),
				   iter.gobj(),
				   INF_GTK_BROWSER_MODEL_COL_BROWSER, &browser,
				   INF_GTK_BROWSER_MODEL_COL_NODE, &browser_iter,
				   -1
				   );

		if(infc_browser_iter_is_subdirectory(browser, browser_iter))
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() = GTK_STOCK_DIRECTORY;
		else
			((Gtk::CellRendererPixbuf*)this)->property_stock_id() = GTK_STOCK_FILE;

		infc_browser_iter_free(browser_iter);
		g_object_unref(G_OBJECT(browser));
	}
	else { //server entry
	    if(((Gtk::CellRendererPixbuf*)this)->property_stock_id() == GTK_STOCK_YES) {
		((Gtk::CellRendererPixbuf*)this)->property_stock_id() = GTK_STOCK_YES;
	    } else {
		((Gtk::CellRendererPixbuf*)this)->property_stock_id() = GTK_STOCK_NO;
	    }
	    
	}
}

Gobby::CellRendererPixbuf::CellRendererPixbuf()
    : Gtk::CellRendererPixbuf::CellRendererPixbuf()
{
    this->property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
}

bool Gobby::CellRendererPixbuf::activate_vfunc(GdkEvent* event, Gtk::Widget& widget,
		    const Glib::ustring& path,
		    const Gdk::Rectangle& background_area,
		    const Gdk::Rectangle& cell_area,
		    Gtk::CellRendererState flags)
{
    Gtk::TreePath gpath(path);

    if(gpath.size() == 1) {	//root node

	    Gtk::TreeView& view = static_cast<Gtk::TreeView&>(widget);
    
	    GtkTreeModel* model = gtk_tree_view_get_model(view.gobj());

	    GtkTreeIter iter;
	    InfcBrowser* browser;
	    gtk_tree_model_get_iter(model, &iter, gpath.gobj());

	    gtk_tree_model_get(model, &iter, INF_GTK_BROWSER_MODEL_COL_BROWSER, &browser, -1);

	    InfXmlConnection* con = infc_browser_get_connection(browser);
	    
	    g_object_unref(browser);    
    }

    return true;
}

