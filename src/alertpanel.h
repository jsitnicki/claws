/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999,2000 Hiroyuki Yamamoto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __ALERTPANEL_H__
#define __ALERTPANEL_H__

#include <glib.h>
#include <gtk/gtkwindow.h>

typedef enum
{
	G_ALERTDEFAULT,
	G_ALERTALTERNATE,
	G_ALERTOTHER,
	G_ALERTWAIT
} AlertValue;

AlertValue alertpanel	(const gchar	*title,
			 const gchar	*message,
			 const gchar	*button1_label,
			 const gchar	*button2_label,
			 const gchar	*button3_label);

void alertpanel_message	(const gchar	*title,
			 const gchar	*message);

void alertpanel_notice	(const gchar	*format,
			 ...) G_GNUC_PRINTF(1, 2);
void alertpanel_warning	(const gchar	*format,
			 ...) G_GNUC_PRINTF(1, 2);
void alertpanel_error	(const gchar	*format,
			 ...) G_GNUC_PRINTF(1, 2);

#endif /* __ALERTPANEL_H__ */
