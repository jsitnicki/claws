/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "common/sylpheed.h"
#include "common/version.h"
#include "plugin.h"
#include "utils.h"
#include "hooks.h"
#include "folder.h"
#include "mainwindow.h"
#include "gtkutils.h"
#include "intl.h"
#include "menu.h"
#include "toolbar.h"
#include "prefs_common.h"
#include "main.h"
#include "alertpanel.h"
#include "gtk/manage_window.h"

#include "eggtrayicon.h"
#include "newmarkedmail.xpm"
#include "unreadmarkedmail.xpm"
#include "newmail.xpm"
#include "unreadmail.xpm"
#include "nomail.xpm"

static guint hook_id;

static GdkPixmap *newmail_pixmap;
static GdkPixmap *newmail_bitmap;
static GdkPixmap *unreadmail_pixmap;
static GdkPixmap *unreadmail_bitmap;
static GdkPixmap *newmarkedmail_pixmap;
static GdkPixmap *newmarkedmail_bitmap;
static GdkPixmap *unreadmarkedmail_pixmap;
static GdkPixmap *unreadmarkedmail_bitmap;
static GdkPixmap *nomail_pixmap;
static GdkPixmap *nomail_bitmap;

static EggTrayIcon *trayicon;
static GtkWidget *eventbox;
static GtkWidget *image;
static GtkTooltips *tooltips;
static GtkWidget *traymenu_popup;

guint destroy_signal_id;

typedef enum
{
	TRAYICON_NEW,
	TRAYICON_NEWMARKED,
	TRAYICON_UNREAD,
	TRAYICON_UNREADMARKED,
	TRAYICON_NOTHING,
} TrayIconType;

static void trayicon_get_cb	    (gpointer data, guint action, GtkWidget *widget);
static void trayicon_get_all_cb	    (gpointer data, guint action, GtkWidget *widget);
static void trayicon_compose_cb	    (gpointer data, guint action, GtkWidget *widget);
static void trayicon_addressbook_cb (gpointer data, guint action, GtkWidget *widget);
static void trayicon_exit_cb	    (gpointer data, guint action, GtkWidget *widget);
static void resize_cb		    (GtkWidget *widget, GtkRequisition *req, gpointer user_data);

static GtkItemFactoryEntry trayicon_popup_menu_entries[] =
{
	{N_("/_Get"),			NULL, trayicon_get_cb, 		0, NULL},
	{N_("/Get _All"),		NULL, trayicon_get_all_cb, 	0, NULL},
	{N_("/---"),			NULL, NULL, 			0, "<Separator>"},
	{N_("/_Email"),			NULL, trayicon_compose_cb,   	0, NULL},
	{N_("/Open A_ddressbook"),	NULL, trayicon_addressbook_cb, 	0, NULL},
	{N_("/---"),			NULL, NULL, 			0, "<Separator>"},
	{N_("/E_xit Sylpheed"),		NULL, trayicon_exit_cb,     	0, NULL}
};

static void set_trayicon_pixmap(TrayIconType icontype)
{
	GdkPixmap *pixmap = NULL;
	GdkBitmap *bitmap = NULL;
	static GdkPixmap *last_pixmap = NULL;

	switch(icontype) {
	case TRAYICON_NEW:
		pixmap = newmail_pixmap;
		bitmap = newmail_bitmap;
		break;
	case TRAYICON_NEWMARKED:
		pixmap = newmarkedmail_pixmap;
		bitmap = newmarkedmail_bitmap;
		break;
	case TRAYICON_UNREAD:
		pixmap = unreadmail_pixmap;
		bitmap = unreadmail_bitmap;
		break;
	case TRAYICON_UNREADMARKED:
		pixmap = unreadmarkedmail_pixmap;
		bitmap = unreadmarkedmail_bitmap;
		break;
	default:
		pixmap = nomail_pixmap;
		bitmap = nomail_bitmap;
		break;
	}

	if (pixmap == last_pixmap)
		return;

	gtk_image_set_from_pixmap(GTK_IMAGE(image), pixmap, bitmap);
	gtk_widget_shape_combine_mask(GTK_WIDGET(trayicon), bitmap, GTK_WIDGET(image)->allocation.x, GTK_WIDGET(image)->allocation.y);

	last_pixmap = pixmap;
}

static void update(void)
{
	gint new, unread, unreadmarked, total;
	gchar *buf;
	TrayIconType icontype = TRAYICON_NOTHING;

	folder_count_total_msgs(&new, &unread, &unreadmarked, &total);
	buf = g_strdup_printf("New %d, Unread: %d, Total: %d", new, unread, total);

        gtk_tooltips_set_tip(tooltips, eventbox, buf, "");
	g_free(buf);
	
	if (new > 0 && unreadmarked > 0)
		icontype = TRAYICON_NEWMARKED;
	else if (new > 0)
		icontype = TRAYICON_NEW;
	else if (unreadmarked > 0)
		icontype = TRAYICON_UNREADMARKED;
	else if (unread > 0)
		icontype = TRAYICON_UNREAD;

	set_trayicon_pixmap(icontype);
}

static gboolean folder_item_update_hook(gpointer source, gpointer data)
{
	update();

	return FALSE;
}

static void resize_cb(GtkWidget *widget, GtkRequisition *req,
		      gpointer user_data)
{
	update();
}

static gboolean click_cb(GtkWidget * widget,
		         GdkEventButton * event, gpointer user_data)
{
	MainWindow *mainwin;

	if (event == NULL)
		return TRUE;

	mainwin = mainwindow_get_mainwindow();
	
	switch (event->button) {
	case 1:
		if (GTK_WIDGET_VISIBLE(GTK_WIDGET(mainwin->window))) {
			main_window_hide(mainwin);
		} else {
			main_window_show(mainwin);
        	}
		break;
	case 3:
		gtk_menu_popup( GTK_MENU(traymenu_popup), NULL, NULL, NULL, NULL,
		       event->button, event->time );
		break;
	default:
		return TRUE;
	}
	return TRUE;
}

static void create_trayicon(void);

static void destroy_cb(GtkWidget *widget, gpointer *data)
{
	debug_print("Widget destroyed\n");

	create_trayicon();
}

static void create_trayicon()
{
	gint n_entries = 0;
	GtkItemFactory *traymenu_factory;
#if 0
	GtkPacker *packer;
#endif

        trayicon = egg_tray_icon_new("Sylpheed-Claws");
	gtk_widget_realize(GTK_WIDGET(trayicon));
	gtk_window_set_default_size(GTK_WINDOW(trayicon), 16, 16);
        gtk_container_set_border_width(GTK_CONTAINER(trayicon), 0);

        PIXMAP_CREATE(GTK_WIDGET(trayicon), nomail_pixmap, nomail_bitmap, nomail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), unreadmail_pixmap, unreadmail_bitmap, unreadmail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), newmail_pixmap, newmail_bitmap, newmail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), unreadmarkedmail_pixmap, unreadmarkedmail_bitmap, unreadmarkedmail_xpm);
        PIXMAP_CREATE(GTK_WIDGET(trayicon), newmarkedmail_pixmap, newmarkedmail_bitmap, newmarkedmail_xpm);

        eventbox = gtk_event_box_new();
        gtk_container_set_border_width(GTK_CONTAINER(eventbox), 0);
        gtk_container_add(GTK_CONTAINER(trayicon), GTK_WIDGET(eventbox));

        image = gtk_image_new_from_pixmap(nomail_pixmap, nomail_bitmap);
        gtk_container_add(GTK_CONTAINER(eventbox), image);

	destroy_signal_id =
	g_signal_connect(G_OBJECT(trayicon), "destroy",
                     	 G_CALLBACK(destroy_cb), NULL);
	g_signal_connect(GTK_OBJECT(trayicon), "size-request",
		    	 G_CALLBACK(resize_cb), NULL);
	g_signal_connect(G_OBJECT(eventbox), "button-press-event",
		    	 G_CALLBACK(click_cb), NULL);

        tooltips = gtk_tooltips_new();
        gtk_tooltips_set_delay(tooltips, 1000);
        gtk_tooltips_enable(tooltips);

	n_entries = sizeof(trayicon_popup_menu_entries) /
		sizeof(trayicon_popup_menu_entries[0]);
	traymenu_popup = menu_create_items(trayicon_popup_menu_entries,
				       n_entries,
				       "<TrayiconMenu>", &traymenu_factory,
				       NULL);

        gtk_widget_show_all(GTK_WIDGET(trayicon));

	update();
}

int plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 3, 86))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, folder_item_update_hook, NULL);
	if (hook_id == -1) {
		*error = g_strdup("Failed to register folder item update hook");
		return -1;
	}

	create_trayicon();

        return 0;
}

void plugin_done(void)
{
	g_signal_handler_disconnect(G_OBJECT(trayicon), destroy_signal_id);

	gtk_widget_destroy(GTK_WIDGET(trayicon));
	hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_id);

	while (gtk_events_pending())
		gtk_main_iteration();		
}

const gchar *plugin_name(void)
{
	return _("Trayicon");
}

const gchar *plugin_desc(void)
{
	return _("This plugin places a mailbox icon in the system tray that "
	         "indicates if you have new or unread mail.\n"
	         "\n"
	         "The mailbox is empty if you have no unread mail, otherwise "
	         "it contains a letter. A tooltip shows new, unread and total "
	         "number of messages.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

/* popup menu callbacks */
static void trayicon_get_cb( gpointer data, guint action, GtkWidget *widget )
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	inc_mail_cb(mainwin, 0, NULL);
}

static void trayicon_get_all_cb( gpointer data, guint action, GtkWidget *widget )
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	inc_all_account_mail_cb(mainwin, 0, NULL);
}

static void trayicon_compose_cb( gpointer data, guint action, GtkWidget *widget )
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	compose_mail_cb(mainwin, 0, NULL);
}

static void trayicon_addressbook_cb( gpointer data, guint action, GtkWidget *widget )
{
	addressbook_open(NULL);
}

static void app_exit_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
	if (prefs_common.confirm_on_exit) {
		if (alertpanel(_("Exit"), _("Exit this program?"),
			       _("OK"), _("Cancel"), NULL) != G_ALERTDEFAULT)
			return;
		manage_window_focus_in(mainwin->window, NULL, NULL);
	}

	app_will_exit(NULL, mainwin);
}

static void trayicon_exit_cb( gpointer data, guint action, GtkWidget *widget )
{
	MainWindow *mainwin = mainwindow_get_mainwindow();

	if (mainwin->lock_count == 0)
		app_exit_cb(mainwin, 0, NULL);
}
