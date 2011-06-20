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

#include "dialogs/password-dialog.hpp"
#include "core/browser.hpp"
#include "util/gtk-compat.hpp"
#include "util/file.hpp"
#include "util/i18n.hpp"
#include "core/pinningentry.hpp"

#include <libinfinity/inf-config.h>

#include <gtkmm/stock.h>
#include <gtkmm/image.h>
#include <iostream>

#ifndef G_OS_WIN32
# include <sys/socket.h>
# include <net/if.h>
#endif



gint compare_func(GtkTreeModel* model, GtkTreeIter* first, GtkTreeIter* second, gpointer user_data)
{
	gint result;
	InfcBrowser* br_one;
	InfcBrowser* br_two;
	InfcBrowserIter* bri_one;
	InfcBrowserIter* bri_two;
	GtkTreeIter parent;
	

	result = 0;
	if(gtk_tree_model_iter_parent(model, &parent, first))
	{
		g_assert(gtk_tree_model_iter_parent(model, &parent, second));

		gtk_tree_model_get(
			model, first,
			INF_GTK_BROWSER_MODEL_COL_BROWSER, &br_one,
			INF_GTK_BROWSER_MODEL_COL_NODE, &bri_one,
			-1);
		gtk_tree_model_get(
			model, second,
			INF_GTK_BROWSER_MODEL_COL_BROWSER, &br_two,
			INF_GTK_BROWSER_MODEL_COL_NODE, &bri_two,
			-1);

		if(infc_browser_iter_is_subdirectory(br_one, bri_one) &&
		   !infc_browser_iter_is_subdirectory(br_two, bri_two))
		{
			result = -1;
		}
		else if(!infc_browser_iter_is_subdirectory(br_one, bri_one) &&
		        infc_browser_iter_is_subdirectory(br_two, bri_two))
		{
			result = 1;
		}

		g_object_unref(br_one);
		g_object_unref(br_two);
		infc_browser_iter_free(bri_one);
		infc_browser_iter_free(bri_two);
	}

	if(!result)
	{
		gchar* name_one;
		gchar* name_two;

		gtk_tree_model_get(
			model, first,
			INF_GTK_BROWSER_MODEL_COL_NAME, &name_one,
			-1);
		gtk_tree_model_get(
			model, second,
			INF_GTK_BROWSER_MODEL_COL_NAME, &name_two,
			-1);

		gchar* one = g_utf8_casefold(name_one, -1);
		gchar* two = g_utf8_casefold(name_two, -1);

		result = g_utf8_collate(one, two);

		g_free(name_one);
		g_free(name_two);
		g_free(one);
		g_free(two);
	}

	return result;
}

Gobby::Browser::Browser(Gtk::Window& parent,
                        const InfcNotePlugin* text_plugin,
                        StatusBar& status_bar,
                        Preferences& preferences,
                        Pinning& pinning):
	m_parent(parent),
	m_text_plugin(text_plugin),
	m_status_bar(status_bar),
	m_preferences(preferences),
	m_sasl_context(NULL),
	m_expander(_("_Direct Connection"), true),
	m_hbox(false, 6),
	m_label_hostname(_("Host Name:")),
	m_entry_hostname(config_filename("recent_hosts"), 5),
	m_pinning(pinning),
	m_renderer(m_pinning)
{
	m_label_hostname.show();
	m_entry_hostname.get_entry()->signal_activate().connect(
		sigc::mem_fun(*this, &Browser::on_hostname_activate));
	m_entry_hostname.show();


	m_hbox.pack_start(m_label_hostname, Gtk::PACK_SHRINK);
	m_hbox.pack_start(m_entry_hostname, Gtk::PACK_EXPAND_WIDGET);
	m_hbox.show();

	m_expander.add(m_hbox);
	m_expander.show();
	m_expander.property_expanded().signal_changed().connect(
		sigc::mem_fun(*this, &Browser::on_expanded_changed));

	m_io = inf_gtk_io_new();
	InfCommunicationManager* communication_manager =
		inf_communication_manager_new();

	m_browser_store = inf_gtk_browser_store_new(INF_IO(m_io),
	                                            communication_manager);
	g_object_unref(communication_manager);

	m_sort_model = inf_gtk_browser_model_sort_new(INF_GTK_BROWSER_MODEL(m_browser_store));
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(m_sort_model), compare_func, NULL, NULL);

	m_xmpp_manager = inf_xmpp_manager_new();
#ifdef LIBINFINITY_HAVE_AVAHI
	m_discovery = inf_discovery_avahi_new(INF_IO(m_io), m_xmpp_manager,
	                                      NULL, NULL, NULL);
	inf_discovery_avahi_set_security_policy(
		m_discovery, m_preferences.security.policy);
	inf_gtk_browser_store_add_discovery(m_browser_store,
	                                    INF_DISCOVERY(m_discovery));
#endif

	Glib::ustring known_hosts_file = config_filename("known_hosts");

	const std::string trust_file = m_preferences.security.trust_file;
	m_cert_manager = inf_gtk_certificate_manager_new(
		parent.gobj(), m_xmpp_manager,
		trust_file.empty() ? NULL : trust_file.c_str(),
		known_hosts_file.c_str());

	m_browser_view =
		INF_GTK_BROWSER_VIEW(
			inf_gtk_browser_view_new_with_model(
				INF_GTK_BROWSER_MODEL(m_sort_model)));

	gtk_widget_show(GTK_WIDGET(m_browser_view));
	gtk_container_add(GTK_CONTAINER(m_scroll.gobj()),
	                  GTK_WIDGET(m_browser_view));
	m_scroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_scroll.set_shadow_type(Gtk::SHADOW_IN);
	m_scroll.show();

	g_signal_connect(
		m_browser_store,
		"set-browser",
		G_CALLBACK(&on_set_browser_static),
		this
	);

	g_signal_connect(
		m_browser_view,
		"activate",
		G_CALLBACK(&on_activate_static),
		this
	);

	m_preferences.security.policy.signal_changed().connect(
		sigc::mem_fun(*this, &Browser::on_security_policy_changed));
	m_preferences.security.trust_file.signal_changed().connect(
		sigc::mem_fun(*this, &Browser::on_trust_file_changed));

	set_spacing(6);
	pack_start(m_scroll, Gtk::PACK_EXPAND_WIDGET);
	pack_start(m_expander, Gtk::PACK_SHRINK);

	set_focus_child(m_expander);

	inf_gtk_browser_view_set_status_cell_renderer(m_browser_view, (GtkCellRenderer*)m_renderer.gobj());
	
	Gtk::TreeViewColumn *col = Glib::wrap(inf_gtk_browser_view_get_column(m_browser_view));
	
	col->set_cell_data_func(m_renderer, sigc::bind(sigc::mem_fun(&m_renderer, &Gobby::CellRendererPixbuf::status_icon_data_func), m_sort_model));
	
}

Gobby::Browser::~Browser()
{
	if(m_sasl_context)
		inf_sasl_context_unref(m_sasl_context);


	g_object_unref(m_browser_store);
	g_object_unref(m_sort_model);
	g_object_unref(m_cert_manager);
	g_object_unref(m_xmpp_manager);
#ifdef LIBINFINITY_HAVE_AVAHI
	g_object_unref(m_discovery);
#endif
	g_object_unref(m_io);
}

void Gobby::Browser::load_pinning_entries()
{
	// TODO: load pinning entries
	m_pinning.set_sasl_context(m_sasl_context);
	m_pinning.load_saved_connections();

	std::list<InfXmppConnection*> saved_connections
	  = m_pinning.get_saved_connections();
	for(std::list<InfXmppConnection*>::iterator it = saved_connections.begin();
	    it != saved_connections.end();
			++it)
	{
		PinningEntry* pentry = m_pinning.get_entry(*it);
		g_assert(pentry != NULL);
		inf_xmpp_manager_add_connection(m_xmpp_manager,INF_XMPP_CONNECTION(*it));
		inf_gtk_browser_store_add_connection(
			m_browser_store,
			INF_XML_CONNECTION(*it),
			pentry->get_property(PinningEntry::HOST).c_str());
	}

}

void Gobby::Browser::on_expanded_changed()
{
	if(m_expander.get_expanded())
	{
		if(GtkCompat::is_realized(m_entry_hostname))
		{
			m_entry_hostname.grab_focus();
		}
		else
		{
			m_entry_hostname.signal_realize().connect(
				sigc::mem_fun(m_entry_hostname,
					&Gtk::Entry::grab_focus));
		}
	}
}

void Gobby::Browser::on_set_browser(GtkTreeIter* iter,
                                    InfcBrowser* browser)
{
	if(browser)
		infc_browser_add_plugin(browser, m_text_plugin);
}

void Gobby::Browser::on_activate(GtkTreeIter* iter)
{
	InfcBrowser* browser;
	InfcBrowserIter* browser_iter;

	gtk_tree_model_get(GTK_TREE_MODEL(m_sort_model), iter,
	                   INF_GTK_BROWSER_MODEL_COL_BROWSER, &browser,
	                   INF_GTK_BROWSER_MODEL_COL_NODE, &browser_iter,
	                   -1);

	m_signal_activate.emit(browser, browser_iter);

	infc_browser_iter_free(browser_iter);
	g_object_unref(browser);
}

void Gobby::Browser::on_hostname_activate()
{
	Glib::ustring str = m_entry_hostname.get_entry()->get_text();
	if(str.empty()) return;

	connect_to_host(str);

	m_entry_hostname.commit();
	m_entry_hostname.get_entry()->set_text("");
}

bool Gobby::Browser::get_selected(InfcBrowser** browser,
                                  InfcBrowserIter* iter)
{
	GtkTreeIter tree_iter;
	if(!inf_gtk_browser_view_get_selected(m_browser_view, &tree_iter))
		return false;

	InfcBrowser* tmp_browser;
	InfcBrowserIter* tmp_iter;

	gtk_tree_model_get(
		GTK_TREE_MODEL(m_sort_model), &tree_iter,
		INF_GTK_BROWSER_MODEL_COL_BROWSER, &tmp_browser,
		-1);

	if(tmp_browser == NULL)
		return false;

	gtk_tree_model_get(
		GTK_TREE_MODEL(m_sort_model), &tree_iter,
		INF_GTK_BROWSER_MODEL_COL_NODE, &tmp_iter,
		-1);

	*browser = tmp_browser;
	*iter = *tmp_iter;

	infc_browser_iter_free(tmp_iter);
	g_object_unref(tmp_browser);

	return true;
}

void Gobby::Browser::set_selected(InfcBrowser* browser, InfcBrowserIter* iter)
{
	GtkTreeIter tree_iter;

	gboolean has_iter = inf_gtk_browser_model_browser_iter_to_tree_iter(
		INF_GTK_BROWSER_MODEL(m_sort_model),
		browser, iter, &tree_iter);
	g_assert(has_iter == TRUE);

	inf_gtk_browser_view_set_selected(m_browser_view, &tree_iter);
}

void Gobby::Browser::connect_to_host(Glib::ustring str)
{
	Glib::ustring host;
	Glib::ustring service = "6523"; // Default
	unsigned int device_index = 0; // Default
	gint iservice;
	std::stringstream sstr;

	// Strip away device name
	Glib::ustring::size_type pos;
	if( (pos = str.rfind('%')) != Glib::ustring::npos)
	{
		Glib::ustring device_name = str.substr(pos + 1);
		str.erase(pos);

#ifdef G_OS_WIN32
		// TODO: Implement
		device_index = 0;
#else
		device_index = if_nametoindex(device_name.c_str());
		if(device_index == 0)
		{
			m_status_bar.add_error_message(
				Glib::ustring::compose(
					_("Connection to \"%1\" failed"),
					host),
				Glib::ustring::compose(
					_("Device \"%1\" does not exist"),
					device_name));
		}
#endif
	}

	if(str[0] == '[' && ((pos = str.find(']', 1)) != Glib::ustring::npos))
	{
		// Hostname surrounded by '[...]'
		host = str.substr(1, pos-1);
		++ pos;
		if(pos < str.length() && str[pos] == ':')
			service = str.substr(pos + 1);
	}
	else
	{
		pos = str.find(':');
		if(pos != Glib::ustring::npos)
		{
			host = str.substr(0, pos);
			service = str.substr(pos + 1);
		}
		else
			host = str;
	}

	sstr << service;
	sstr >> iservice;

	if(iservice == 0){
		iservice=6523;
		m_status_bar.add_info_message(
				_("Substituted port to default port: 6523"), 5
		);
	}

	InfTcpConnection* connection = inf_tcp_connection_new_from_hostname(
		INF_IO(m_io),
		host.c_str(),
		iservice
	);

	g_object_set(G_OBJECT(connection),
		     "device-index", device_index,
		     NULL);

	InfXmppConnection* xmpp = inf_xmpp_connection_new(
		connection, INF_XMPP_CONNECTION_CLIENT,
		NULL, host.c_str(),
		m_preferences.security.policy,
		NULL,
		m_sasl_context,
		m_sasl_mechanisms.empty()
			? ""
			: m_sasl_mechanisms.c_str());

	/* Check if the connection is already managed
	 * and suppress it if that is the case. */
	if(!inf_xmpp_manager_contains_connection(m_xmpp_manager, xmpp))
	{
		open_connection(connection, xmpp, &host);
	}
	else
	{
		m_status_bar.add_error_message(
			Glib::ustring::compose(
				_("Already connected to \"%1\""),
				host),
			Glib::ustring::compose(
				_("Connection to \"%1\" suppressed"),
				host)
		);
	}
	g_object_unref(connection);
}

void Gobby::Browser::set_sasl_context(InfSaslContext* sasl_context,
                                      const char* mechanisms)
{
	if(m_sasl_context) inf_sasl_context_unref(m_sasl_context);
	m_sasl_context = sasl_context;
	if(m_sasl_context) inf_sasl_context_ref(m_sasl_context);
	m_sasl_mechanisms = mechanisms ? mechanisms : "";

#ifdef LIBINFINITY_HAVE_AVAHI
	g_object_set(G_OBJECT(m_discovery),
		"sasl-context", m_sasl_context,
		"sasl-mechanisms", mechanisms,
		NULL);
#endif
}

void Gobby::Browser::open_connection(InfTcpConnection* connection,
                                     InfXmppConnection* xmpp,
                                     const Glib::ustring* host)
{
	InfTcpConnectionStatus tcp_status;
	InfIpAddress* addr;
	g_return_if_fail(INF_IS_TCP_CONNECTION(connection));
	g_return_if_fail(INF_IS_XMPP_CONNECTION(xmpp));
	g_assert(connection != NULL);
	g_assert(xmpp != NULL);
	/* The connection must be added to the manager at this point,
	 * because otherwise a check for multiple connections to the
	 * same host isn't possible when the connection is opened. */
	inf_xmpp_manager_add_connection(m_xmpp_manager, xmpp);
	GError* error = NULL;
	//try to open the connection (the hostname gets resolved now!)
	if(!inf_tcp_connection_open(connection, &error))
	{
		//the connection itself could not be established
		inf_xmpp_manager_remove_connection(
			m_xmpp_manager, xmpp
		);
		m_status_bar.add_error_message(
			Glib::ustring::compose(
				_("Connection to \"%1\" failed"),
				*host
			),
			error->message
		);
		g_error_free(error);
		return;
	}

	/* Check if the connection was opened or suppressed.
	 * It is suppression if and only if the _resolved_
	 * connection is already managed.
	 * The suppression check cannot happen before the
	 * connection is resolved.
	 * Therefore the connection status must be checked
	 * for verification. */
	g_object_get(G_OBJECT(connection),
		     "status", &tcp_status, NULL);
	if(tcp_status == INF_TCP_CONNECTION_CONNECTING ||
	   tcp_status == INF_TCP_CONNECTION_CONNECTED)
	{
		/* The connection was successfully established
		 * and was not already managed. */
		inf_gtk_browser_store_add_connection(
			m_browser_store,
			INF_XML_CONNECTION(xmpp),
			host->c_str()
		);
	}
	else
	{
		//the connection was suppressed
		inf_xmpp_manager_remove_connection(
			m_xmpp_manager, xmpp
		);
		addr = inf_tcp_connection_get_remote_address(connection);
		m_status_bar.add_error_message(
			Glib::ustring::compose(
				_("Already connected to %1"),
				inf_ip_address_to_string(addr)),
			Glib::ustring::compose(
				_("Connection to \"%1\" suppressed"),
				*host)
		);
	}
	g_object_unref(xmpp);
}

void Gobby::Browser::on_security_policy_changed()
{
#ifdef LIBINFINITY_HAVE_AVAHI
	inf_discovery_avahi_set_security_policy(
		m_discovery, m_preferences.security.policy);
#endif
}

void Gobby::Browser::on_trust_file_changed()
{
	const std::string trust_file = m_preferences.security.trust_file;

	g_object_set(G_OBJECT(m_cert_manager), "trust-file",
		     trust_file.empty() ? NULL : trust_file.c_str(), NULL);
}
