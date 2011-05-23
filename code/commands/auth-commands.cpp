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

#include "commands/auth-commands.hpp"
#include "util/i18n.hpp"

#include <libinfinity/common/inf-xmpp-connection.h>
#include <libinfinity/common/inf-error.h>

#include <gtkmm/stock.h>

namespace
{
	void show_error(const GError* error,
	                Gobby::StatusBar& statusbar,
	                InfXmlConnection* connection)
	{
		gchar* remote;
		g_object_get(connection,
			"remote-hostname", &remote,
			NULL);
		Glib::ustring short_message(Glib::ustring::compose(
			"Authentication failed for \"%1\"", remote));
		g_free(remote);

		if(error->domain ==
		   inf_authentication_detail_error_quark())
		{
			statusbar.add_error_message(
				short_message,
				inf_authentication_detail_strerror(
					InfAuthenticationDetailError(
						error->code)));
		}
		else
		{
			statusbar.add_error_message(
				short_message,
				error->message);
		}
	}
}

Gobby::AuthCommands::AuthCommands(Gtk::Window& parent,
                                  Browser& browser,
                                  StatusBar& statusbar,
                                  const Preferences& preferences):
	m_parent(parent),
	m_browser(browser),
	m_statusbar(statusbar),
	m_preferences(preferences)
{
	GError* error = NULL;
	m_sasl_context = inf_sasl_context_new(&error);

	if(!m_sasl_context)
	{
		std::string error_message =
			std::string("SASL initialization error: ") +
			error->message;
		g_error_free(error);
		throw std::runtime_error(error_message);
	}

	inf_sasl_context_set_callback(
		m_sasl_context, &AuthCommands::sasl_callback_static, this);

	m_browser.set_sasl_context(m_sasl_context, "ANONYMOUS PLAIN");

	g_signal_connect(
		G_OBJECT(m_browser.get_store()),
		"set-browser",
		G_CALLBACK(&AuthCommands::set_browser_callback_static),
		this);
}

Gobby::AuthCommands::~AuthCommands()
{
	m_browser.set_sasl_context(NULL, NULL);
	inf_sasl_context_unref(m_sasl_context);

	for(RetryMap::iterator iter = m_retries.begin();
	    iter != m_retries.end(); ++iter)
	{
		//g_signal_handler_disconnect(iter->first, iter->second.handle);
	}
}

void Gobby::AuthCommands::sasl_callback(InfSaslContextSession* session,
                                        InfXmppConnection* xmpp,
                                        Gsasl_property prop)
{
	Glib::ustring username = m_preferences.user.name;
	switch(prop)
	{
	case GSASL_ANONYMOUS_TOKEN:
		inf_sasl_context_session_set_property(
			session, GSASL_ANONYMOUS_TOKEN, username.c_str());
		inf_sasl_context_session_continue(session, GSASL_OK);
		break;
	case GSASL_AUTHID:
		inf_sasl_context_session_set_property(
			session, GSASL_AUTHID, username.c_str());
		inf_sasl_context_session_continue(session, GSASL_OK);
		break;
	case GSASL_PASSWORD:
		{
			RetryMap::iterator i = m_retries.find(xmpp);
			if(i == m_retries.end())
				i = insert_retry_info(xmpp);
			RetryInfo& info(i->second);
			if(!info.last_password.empty())
			{
				inf_sasl_context_session_set_property(
					session, GSASL_PASSWORD,
					info.last_password.c_str());

				inf_sasl_context_session_continue(session,
				                                  GSASL_OK);
			}
			else
			{
				// Query user for password
				g_assert(info.password_dialog == NULL);

				gchar* remote_id;
				g_object_get(G_OBJECT(xmpp),
				             "remote-hostname", &remote_id,
					     NULL);
				Glib::ustring remote_id_(remote_id);
				g_free(remote_id);

				info.password_dialog = new PasswordDialog(
					m_parent, remote_id_, info.retries);
				info.password_dialog->add_button(
					Gtk::Stock::CANCEL,
					Gtk::RESPONSE_CANCEL);
				info.password_dialog->add_button(
					Gtk::Stock::OK,
					Gtk::RESPONSE_ACCEPT);

				Gtk::Dialog& dialog = *info.password_dialog;
				dialog.signal_response().connect(sigc::bind(
					sigc::mem_fun(
						*this,
						&AuthCommands::on_response),
					session, xmpp));

				info.password_dialog->present();
			}
		}

		break;
	default:
		inf_sasl_context_session_continue(session, GSASL_NO_CALLBACK);
		break;
	}
}

void Gobby::AuthCommands::on_response(int response_id,
                                      InfSaslContextSession* session,
                                      InfXmppConnection* xmpp)
{
	RetryMap::iterator i = m_retries.find(xmpp);
	g_assert(i != m_retries.end());
	RetryInfo& info(i->second);

	if(response_id == Gtk::RESPONSE_ACCEPT)
		info.last_password = info.password_dialog->get_password();
	else
		info.last_password = "";

	delete info.password_dialog;
	info.password_dialog = NULL;

	++info.retries;

	if(info.last_password.empty())
	{
		inf_sasl_context_session_continue(session, GSASL_NO_PASSWORD);
	}
	else
	{
		inf_sasl_context_session_set_property(
			session, GSASL_PASSWORD, info.last_password.c_str());
		inf_sasl_context_session_continue(session, GSASL_OK);
	}
}

void Gobby::AuthCommands::set_browser_callback(InfcBrowser* browser)
{
	g_signal_connect(
		G_OBJECT(browser),
		"error",
		G_CALLBACK(browser_error_callback_static),
		this);
}

void Gobby::AuthCommands::browser_error_callback(InfcBrowser* browser,
                                                 GError* error)
{
	// The Browser already displays errors inline, but we want
	// auth-related error messages to show up in the status bar.

	InfXmlConnection* connection = infc_browser_get_connection(browser);
	g_assert(INF_IS_XMPP_CONNECTION(connection));

	InfXmppConnection* xmpp = INF_XMPP_CONNECTION(connection);
	RetryMap::iterator iter = m_retries.find(xmpp);
	if(iter == m_retries.end())
		iter = insert_retry_info(xmpp);
	Glib::ustring& last_password(iter->second.last_password);
	Glib::ustring old_password;

	old_password.swap(last_password);

	if(error->domain ==
	     g_quark_from_static_string("INF_XMPP_CONNECTION_AUTH_ERROR"))
	{
		// Authentication failed for some reason, maybe because the
		// server aborted authentication. If we were querying a
		// password then close the dialog now.
		delete iter->second.password_dialog;
		iter->second.password_dialog = NULL;

		const GError* sasl_error =
			inf_xmpp_connection_get_sasl_error(xmpp);
		if(sasl_error != NULL &&
		   sasl_error->domain ==
		     inf_authentication_detail_error_quark())
		{
			handle_error_detail(xmpp, sasl_error,
			                    old_password,
			                    last_password);
		}
		else if(sasl_error != NULL)
		{
			show_error(sasl_error, m_statusbar, connection);
		}
		else
		{
			show_error(error, m_statusbar, connection);
		}
	}
	else if(error->domain == inf_gsasl_error_quark())
	{
		show_error(error, m_statusbar, connection);
	}
}

void Gobby::AuthCommands::handle_error_detail(InfXmppConnection* xmpp,
                                              const GError* detail_error,
                                              Glib::ustring& old_password,
                                              Glib::ustring& last_password)
{
	GError* error = NULL;
	switch(detail_error->code)
	{
	case INF_AUTHENTICATION_DETAIL_ERROR_AUTHENTICATION_FAILED:
		inf_xmpp_connection_retry_sasl_authentication(xmpp, &error);
		break;
	case INF_AUTHENTICATION_DETAIL_ERROR_TRY_AGAIN:
		old_password.swap(last_password);
		inf_xmpp_connection_retry_sasl_authentication(xmpp, &error);

		break;
	default:
		show_error(detail_error, m_statusbar,
		           INF_XML_CONNECTION(xmpp));
		break;
	}

	if(error)
	{
		show_error(error, m_statusbar,
		           INF_XML_CONNECTION(xmpp));
		g_error_free(error);
	}
}

Gobby::AuthCommands::RetryMap::iterator
Gobby::AuthCommands::insert_retry_info(InfXmppConnection* xmpp)
{
	RetryMap::iterator iter = m_retries.insert(
		std::make_pair(xmpp,
		               RetryInfo())).first;
	iter->second.retries = 0;
	// iter->second.handle = g_signal_connect(
	// 	G_OBJECT(xmpp),
	// 	"notify::status",
	// 	G_CALLBACK(on_notify_status_static),
	// 	this);
	iter->second.password_dialog = NULL;

	return iter;
}

void Gobby::AuthCommands::on_notify_status(InfXmppConnection* connection)
{
	InfXmlConnectionStatus status;
	g_object_get(G_OBJECT(connection), "status", &status, NULL);

	if(status != INF_XML_CONNECTION_OPENING)
	{
		RetryMap::iterator iter = m_retries.find(connection);
		g_signal_handler_disconnect(connection, iter->second.handle);
		delete iter->second.password_dialog;
		m_retries.erase(iter); // TODO: does this cause troubles?!
	}
}

Glib::ustring
Gobby::AuthCommands::get_saved_password(InfXmppConnection* connection)
{
	RetryMap::iterator i = m_retries.find(connection);
	if(i == m_retries.end())
	{
		insert_retry_info(connection);
		return "";
	}
	else
	{
		RetryInfo& info(i->second);
		return info.last_password;
	}
}

void
Gobby::AuthCommands::set_saved_password(InfXmppConnection* connection,
                                        Glib::ustring password)
{
	RetryMap::iterator i = m_retries.find(connection);
	if(i == m_retries.end())
	{
		i = insert_retry_info(connection);
	}

	RetryInfo& info(i->second);
	info.last_password = password;
}

