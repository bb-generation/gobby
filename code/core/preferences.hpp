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

#ifndef _GOBBY_PREFERENCES_HPP_
#define _GOBBY_PREFERENCES_HPP_

#include "util/config.hpp"

#include "features.hpp"

#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcestyleschememanager.h>

#include <gtkmm/toolbar.h>

#include <libinfinity/common/inf-xmpp-connection.h>

namespace Gobby
{

class Preferences
{
public:
	template<typename Type>
	class Option
	{
	public:
		typedef sigc::signal<void> signal_changed_type;

		Option(const Type& initial_value):
			m_value(initial_value) {}

		const Option<Type>& operator=(const Type& new_value)
		{
			m_value = new_value;
			notify();
			return *this;
		}

		void set(const Type& new_value)
		{
			*this = new_value;
		}

		const Type& get() const
		{
			return m_value;
		}

		operator const Type&() const
		{
			return m_value;
		}

		/*operator Type&()
		{
			return m_value;
		}*/

		signal_changed_type signal_changed() const
		{
			return m_signal_changed;
		}

		void notify() const
		{
			m_signal_changed.emit();
		}

	protected:
		Type m_value;
		signal_changed_type m_signal_changed;
	};

	/** Reads preferences values out of a config, using default values
	 * for values that do not exist in the config.
	 */
	Preferences(Config& m_config);

	/** Serialises preferences back to config.
	 */
	void serialize(Config& config) const;

	class User
	{
	public:
		User(Config::ParentEntry& entry);
		void serialize(Config::ParentEntry& entry) const;

		Option<Glib::ustring> name;
		Option<double> hue;
		Option<std::string> host_directory;

		Option<bool> show_remote_cursors;
		Option<bool> show_remote_selections;
		Option<bool> show_remote_current_lines;
		Option<bool> show_remote_cursor_positions;
	};

	class Editor
	{
	public:
		Editor(Config::ParentEntry& entry);
		void serialize(Config::ParentEntry& entry) const;

		Option<unsigned int> tab_width;
		Option<bool> tab_spaces;
		Option<bool> indentation_auto;
		Option<bool> homeend_smart;
		Option<bool> autosave_enabled;
		Option<unsigned int> autosave_interval;
	};

	class View
	{
	public:
		View(Config::ParentEntry& entry);
		void serialize(Config::ParentEntry& entry) const;

		Option<Gtk::WrapMode> wrap_mode;
		Option<bool> linenum_display;
		Option<bool> curline_highlight;
		Option<bool> margin_display;
		Option<unsigned int> margin_pos;
		Option<bool> bracket_highlight;
		Option<GtkSourceDrawSpacesFlags> whitespace_display;
	};

	class Appearance
	{
	public:
		Appearance(Config::ParentEntry& entry);
		void serialize(Config::ParentEntry& entry) const;

		// TODO: Option<bool> use_system_default_toolbar_style
		// (sets toolbar_style by gconf). At least WITH_GNOME.
		Option<Gtk::ToolbarStyle> toolbar_style;
		Option<Pango::FontDescription> font;

		Option<Glib::ustring> scheme_id;

		Option<unsigned int> document_userlist_width;
		Option<unsigned int> chat_userlist_width;

		Option<bool> show_toolbar;
		Option<bool> show_statusbar;
		Option<bool> show_browser;
		Option<bool> show_chat;
		Option<bool> show_document_userlist;
		Option<bool> show_chat_userlist;
	};

	class Security
	{
	public:
		Security(Config::ParentEntry& entry);
		void serialize(Config::ParentEntry& entry) const;

		Option<std::string> trust_file;
		Option<InfXmppConnectionSecurityPolicy> policy;
	};

	User user;
	Editor editor;
	View view;
	Appearance appearance;
	Security security;
};

template<typename Type>
std::ostream& operator<<(std::ostream& stream,
                         const Preferences::Option<Type>& option)
{
	stream << static_cast<const Type&>(option);
	return stream;
}

}

#endif // _GOBBY_PREFERENCES_HPP_
