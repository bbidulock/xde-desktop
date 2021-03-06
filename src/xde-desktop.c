/*****************************************************************************

 Copyright (c) 2008-2020  Monavacon Limited <http://www.monavacon.com/>
 Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, version 3 of the license.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 *****************************************************************************/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <sys/utsname.h>

#include <assert.h>
#include <locale.h>
#include <stdarg.h>
#include <strings.h>
#include <regex.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#ifdef XRANDR
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#endif
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif
#include <X11/SM/SMlib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <cairo.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <libgnomevfs/gnome-vfs.h>

#include <pwd.h>

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#define XPRINTF(args...) do { } while (0)
#define OPRINTF(args...) do { if (options.output > 1) { \
	fprintf(stdout, "I: "); \
	fprintf(stdout, args); \
	fflush(stdout); } } while (0)
#define DPRINTF(args...) do { if (options.debug) { \
	fprintf(stderr, "D: %s +%d %s(): ", __FILE__, __LINE__, __func__); \
	fprintf(stderr, args); \
	fflush(stderr); } } while (0)
#define EPRINTF(args...) do { \
	fprintf(stderr, "E: %s +%d %s(): ", __FILE__, __LINE__, __func__); \
	fprintf(stderr, args); \
	fflush(stderr);   } while (0)
#define DPRINT() do { if (options.debug) { \
	fprintf(stderr, "D: %s +%d %s()\n", __FILE__, __LINE__, __func__); \
	fflush(stderr); } } while (0)

#undef EXIT_SUCCESS
#undef EXIT_FAILURE
#undef EXIT_SYNTAXERR

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define EXIT_SYNTAXERR	2

#define XA_SELECTION_NAME	"_XDE_DESKTOP_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"

static int saveArgc;
static char **saveArgv;

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
static Atom _XA_NET_WM_ICON_GEOMETRY;
static Atom _XA_NET_DESKTOP_LAYOUT;
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WM_DESKTOP;
static Atom _XA_XROOTPMAP_ID;
static Atom _XA_ESETROOT_PMAP_ID;

// static Atom _XA_WIN_AREA;
// static Atom _XA_WIN_AREA_COUNT;

static Atom _XA_NET_WORKAREA;
static Atom _XA_WIN_WORKAREA;

typedef enum {
	UseScreenDefault,               /* default screen by button */
	UseScreenActive,                /* screen with active window */
	UseScreenFocused,               /* screen with focused window */
	UseScreenPointer,               /* screen with pointer */
	UseScreenSpecified,             /* specified screen */
} UseScreen;

typedef enum {
	PositionDefault,                /* default position */
	PositionPointer,                /* position at pointer */
	PositionCenter,                 /* center of monitor */
	PositionTopLeft,                /* top left of work area */
	PositionSpecified,		/* specified position (X geometry) */
} MenuPosition;

typedef enum {
	CommandDefault,
	CommandRun,
	CommandQuit,
	CommandReplace,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

typedef enum {
	WindowOrderDefault,
	WindowOrderClient,
	WindowOrderStacking,
} WindowOrder;

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	Bool proxy;
	int button;
	Time timestamp;
	UseScreen which;
	MenuPosition where;
	struct {
		int value;
		int sign;
	} x, y;
	unsigned int w, h;
	Bool cycle;
	Bool hidden;
	Bool minimized;
	Bool monitors;
	Bool workspaces;
	WindowOrder order;
	Bool activate;
	Bool raise;
	Bool restore;
	char *keys;
	Command command;
	char *clientId;
	char *saveFile;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.proxy = False,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseScreenDefault,
	.where = PositionDefault,
	.x = {
	      .value = 0,
	      .sign = 1,
	      }
	,
	.y = {
	      .value = 0,
	      .sign = 1,
	      }
	,
	.w = 0,
	.h = 0,
	.cycle = False,
	.hidden = False,
	.minimized = False,
	.monitors = False,
	.workspaces = False,
	.order = WindowOrderDefault,
	.activate = True,
	.raise = False,
	.restore = True,
	.keys = NULL,
	.command = CommandDefault,
	.clientId = NULL,
	.saveFile = NULL,
};

typedef struct {
	int refs;			/* number of references */
	Pixmap pmap;			/* original pixmap */
	GdkPixmap *pixmap;		/* copy of original pixmap */
} XdePixmap;

typedef struct {
	int index;			/* monitor number */
	int current;			/* current desktop for this monitor */
	GdkRectangle geom;		/* geometry of the monitor */
} XdeMonitor;

#define ICON_WIDE 80
#define ICON_HIGH 80

typedef enum {
	IconTypeShortcut = 1,
	IconTypeDirectory = 2,
	IconTypeFile = 3,
} XdeIconType;

typedef struct {
	XdeIconType type;
	GtkWidget *widget;
	struct {
		int col;
		int row;
	} attached;
} XdeIcon;

struct XdeScreen;
typedef struct XdeScreen XdeScreen;

typedef struct {
	XdeScreen *xscr;
	XdePixmap *pixmap;		/* current pixmap for this desktop */
	int index;
	int rows;			/* number of rows in table */
	int cols;			/* number of cols in table */
	unsigned int xoff, yoff;
	GList *paths;
	GList *links;
	GList *dires;
	GList *files;
	struct {
		GList *links;
		GList *dires;
		GList *files;
		GList *detop;
		GHashTable *paths;
		GHashTable *winds;
	} icons;
	GFile *directory;
	GFileMonitor *monitor;
	GdkRectangle workarea;
} XdeDesktop;

struct XdeScreen {
	int index;			/* index */
	GdkDisplay *disp;
	GdkScreen *scrn;		/* screen */
	GdkWindow *root;
	WnckScreen *wnck;
	gint nmon;			/* number of monitors */
	XdeMonitor *mons;		/* monitors for this screen */
	Bool mhaware;			/* multi-head aware NetWM */
	char *theme;			/* XDE theme name */
	GKeyFile *entry;		/* XDE theme file entry */
	int nimg;			/* number of images */
	Window selwin;			/* selection owner window */
	Atom atom;			/* selection atom for this screen */
	Window laywin;			/* desktop layout selection owner */
	Atom prop;			/* dekstop layout selection atom */
	int width, height;
	guint timer;			/* timer source of running timer */
	int rows;			/* number of rows in layout */
	int cols;			/* number of cols in layout */
	int desks;			/* number of desks in layout */
	int current;			/* current desktop for this screen */
	char *wmname;			/* window manager name (adjusted) */
	Bool goodwm;			/* is the window manager usable? */
	GdkWindow *proxy;
	guint deferred_refresh_layout;
	guint deferred_refresh_desktop;
	GtkWindow *desktop;
	GtkWidget *popup;
	Bool inside;			/* pointer inside popup */
	Bool keyboard;			/* have a keyboard grab */
	Bool pointer;			/* have a pointer grab */
	GdkModifierType mask;
	GtkWidget *table;
	GPtrArray *desktops;		/* array of pointers to XdeDesktops */
	XdeDesktop *desk;		/* current desktop */
	XContext pixmaps;		/* pixmap hash */
};

XdeScreen *screens;			/* array of screens */

typedef enum  {
	TARGET_URI_LIST = 1,
	TARGET_MOZ_URL = 2,
	TARGET_XDS = 3,
	TARGET_RAW = 4,
} TargetType;

static GHashTable *MIME_GENERIC_ICONS = NULL;
static GHashTable *MIME_ALIASES = NULL;
static GHashTable *MIME_SUBCLASSES = NULL;
static GHashTable *MIME_APPLICATIONS = NULL;
static GHashTable *XDG_DESKTOPS = NULL;
static GHashTable *XDG_CATEGORIES = NULL;

XdePixmap *
get_pixmap(XdeScreen *xscr, Pixmap pmap)
{
	XdePixmap *pixmap = NULL;
	Display *dpy;

	dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	if (XFindContext(dpy, pmap, xscr->pixmaps, (XPointer *) &pixmap) == Success) {
		GdkPixmap *orig;
		GdkColormap *cmap;
		gint w, h, d;
		cairo_t *cr;

		orig = gdk_pixmap_foreign_new_for_display(xscr->disp, pmap);
		cmap = gdk_screen_get_default_colormap(xscr->scrn);
		gdk_drawable_set_colormap(orig, cmap);
		gdk_pixmap_get_size(orig, &w, &h);
		d = gdk_drawable_get_depth(orig);
		pixmap = calloc(1, sizeof(*pixmap));
		pixmap->pmap = pmap;
		pixmap->pixmap = gdk_pixmap_new(xscr->root, w, h, d);
		cr = gdk_cairo_create(pixmap->pixmap);
		gdk_cairo_set_source_pixmap(cr, orig, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);
		g_object_unref(orig);
		XSaveContext(dpy, pmap, xscr->pixmaps, (XPointer) pixmap);
	}
	return (pixmap);
}

/** @brief set desktop style
  *
  * Adjusts the style of the desktop window to use the pixmap specified by
  * _XROOTPMAP_ID as the background.  Uses GTK2 styles to do this.  This must
  * be called from update_root_pixmap() for it to work correctly.
  */
static void
set_style(XdeScreen *xscr)
{
	GtkStyle *style;
	GdkPixmap *pixmap = xscr->desk->pixmap->pixmap;

	style = gtk_style_copy(gtk_widget_get_default_style());
	g_object_ref(pixmap);	/* ??? */
	style->bg_pixmap[GTK_STATE_NORMAL] = pixmap;
	g_object_ref(pixmap);	/* ??? */
	style->bg_pixmap[GTK_STATE_PRELIGHT] = pixmap;
	gtk_widget_set_style(GTK_WIDGET(xscr->desktop), style);
}

void update_desktop(XdeDesktop *desk);

static void
xde_desktop_changed(GFileMonitor *monitor, GFile *file, GFile *other_file,
		    GFileMonitorEvent event_type, gpointer user_data)
{
	(void) monitor;
	(void) file;
	(void) other_file;
	(void) event_type;
	XdeDesktop *desk = user_data;
	update_desktop(desk);
}

static void
xde_desktop_watch_directory(XdeDesktop *desk, const char *label, const char *path)
{
	(void) label;
	if (desk->directory)
		g_object_unref(desk->directory);
	desk->directory = g_file_new_for_path(path);
	if (desk->monitor)
		g_file_monitor_cancel(desk->monitor);
	desk->monitor = g_file_monitor_directory(desk->directory,
						 G_FILE_MONITOR_NONE |
						 G_FILE_MONITOR_WATCH_MOUNTS |
						 G_FILE_MONITOR_WATCH_MOVES, NULL, NULL);
	g_signal_connect(G_OBJECT(desk->monitor), "changed", G_CALLBACK(xde_desktop_changed), desk);
}

void
xde_list_free(gpointer data)
{
	g_free(data);
}

/** @brief read the desktop
  *
  * Perform a read of the $HOME/Desktop directory.  Must follow xdg spec to
  * find directory, just use $HOME/Desktop for now.
  */
static void
xde_desktop_read_desktop(XdeDesktop *desk)
{
	char *path;
	DIR *dir;

	g_list_free_full(desk->paths, &xde_list_free);
	g_list_free(desk->links);
	g_list_free(desk->dires);
	g_list_free(desk->files);

	desk->paths = NULL;
	desk->links = NULL;
	desk->dires = NULL;
	desk->files = NULL;

	path = g_strdup_printf("%s/Desktop", getenv("HOME") ? : "~");
	xde_desktop_watch_directory(desk, "Desktop", path);
	if ((dir = opendir(path))) {
		struct dirent *d;

		while ((d = readdir(dir))) {
			char *name;

			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
			/* TODO: provide option for hidden files. */
			if (*d->d_name == '.')
				continue;
			name = g_strdup_printf("%s/%s", path, d->d_name);
			if (d->d_type == DT_DIR) {
				desk->dires = g_list_append(desk->dires, g_strdup_printf("%s/%s", path, d->d_name));
				desk->paths = g_list_append(desk->paths, name);
			} else if (d->d_type == DT_LNK || d->d_type == DT_REG) {
				if (strstr(d->d_name, ".desktop"))
					desk->links = g_list_append(desk->links, name);
				else
					desk->files = g_list_append(desk->files, name);
				desk->paths = g_list_append(desk->paths, name);
			} else
				g_free(name);
		}
		closedir(dir);
	}
	g_free(path);
}

static void
xde_icon_free(gpointer data)
{
	XdeIcon *icon = data;

	gtk_widget_destroy(icon->widget);
	g_free(icon);
}

static XdeIcon *
xde_icon_shortcut_new(XdeDesktop *desk, const char *path)
{
	XdeIcon *icon = NULL;

	(void) desk;
	(void) path;
	/* FIXME: create one */
	return (icon);
}

static XdeIcon *
xde_icon_directory_new(XdeDesktop *desk, const char *path)
{
	XdeIcon *icon = NULL;

	(void) desk;
	(void) path;
	/* FIXME: create one */
	return (icon);
}

static XdeIcon *
xde_icon_file_new(XdeDesktop *desk, const char *path)
{
	XdeIcon *icon = NULL;

	(void) desk;
	(void) path;
	/* FIXME: create one */
	return (icon);
}

static XID
xde_icon_create(XdeIcon *icon)
{
	XID xid = None;

	(void) icon;
	/* FIXME: create icon */
	return (xid);
}

/** @brief show the icon on the corresponding desktop
  */
static void
xde_icon_show(XdeIcon *icon)
{
	gtk_widget_show(icon->widget);
}

/** @brief hide the icon from the corresponding desktop
  */
static void
xde_icon_hide(XdeIcon *icon)
{
	gtk_widget_hide(icon->widget);
}

/** @brief create objects
  *
  * Creates the desktop icons objects for each of the shortcuts, directories
  * and documents found in the Desktop directory.  Desktop icon objects are
  * only created if they have not already been created.  Desktop icons objects
  * that are no longer used are released to be freed.
  */
static void
xde_desktop_create_objects(XdeDesktop *desk)
{
	GList *links, *dires, *files;
	XdeIcon *icon;

	if (!desk->icons.paths) {
		desk->icons.paths =
		    g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, xde_icon_free);
	}

	for (links = desk->links; links; links = links->next) {
		if (!(icon = g_hash_table_lookup(desk->icons.paths, links->data)))
			icon = xde_icon_shortcut_new(desk, links->data);
		if (icon->type == IconTypeShortcut)
			desk->icons.links = g_list_append(desk->icons.links, icon);
		else
			desk->icons.files = g_list_append(desk->icons.files, icon);
		desk->icons.detop = g_list_append(desk->icons.detop, icon);
		g_hash_table_replace(desk->icons.paths, links->data, icon);
	}
	for (dires = desk->dires; dires; dires = dires->next) {
		if (!(icon = g_hash_table_lookup(desk->icons.paths, dires->data)))
			icon = xde_icon_directory_new(desk, dires->data);
		if (icon) {
			desk->icons.dires = g_list_append(desk->icons.dires, icon);
			desk->icons.detop = g_list_append(desk->icons.detop, icon);
			g_hash_table_replace(desk->icons.paths, dires->data, icon);
		}
	}
	for (files = desk->files; files; files = files->next) {
		if (!(icon = g_hash_table_lookup(desk->icons.paths, files->data)))
			icon = xde_icon_file_new(desk, files->data);
		if (icon) {
			desk->icons.files = g_list_append(desk->icons.files, icon);
			desk->icons.detop = g_list_append(desk->icons.detop, icon);
			g_hash_table_replace(desk->icons.paths, files->data, icon);
		}
	}
}

/** @brief create windows
  *
  * Creates windows for all desktop icons.  This method simply requests that
  * each icon create a window and return the XID of the window.  Desktop
  * icons are indexed by XID so that we can find them in event handlers.
  * Note that if a window has already been created for a desktop icon, it
  * still returns its XID.  If desktop icons have been deleted, hide them
  * now so that they do not persist until finalized.
  */
static void
xde_desktop_create_windows(XdeDesktop *desk)
{
	GList *detop;
	GHashTable *winds;
	GHashTableIter iter;
	gpointer key, value;

	winds = g_hash_table_new_full(&g_int_hash, &g_int_equal, NULL, NULL);

	for (detop = desk->icons.detop; detop; detop = detop->next) {
		XdeIcon *icon = detop->data;
		XID xid;

		xid = xde_icon_create(icon);
		g_hash_table_replace(winds, (gpointer) xid, icon);
	}

	g_hash_table_iter_init(&iter, desk->icons.winds);
	while (g_hash_table_iter_next(&iter, &key, &value))
		if (!g_hash_table_lookup(winds, key))
			xde_icon_hide(value);
	g_hash_table_destroy(desk->icons.winds);
	desk->icons.winds = winds;
}

/** @brief calculate cells
  *
  * Creates an array of ICON_WIDEx72 cells on the desktop in columns and rows.
  * This uses the available area of the desktop as indicated by the
  * _NET_WORKAREA or _WIN_WORKAREA properties on the root window.
  * Returns a boolean indicating whether the calculation changed.
  */
static int
xde_desktop_calculate_cells(XdeDesktop *desk)
{
#if 0
	int x, y, w, h;
	int cols, rows, xoff, yoff;

	(void) cols;
	(void) rows;
	(void) xoff;
	(void) yoff;

	/* FIXME */
	/* get _NET_WORKAREA */
	/* get _WIN_WORKAREA */
	/* just use screen clipped */
	if ((desk->workarea.width || desk->workarea.height) &&
	    desk->workarea.x == x &&
	    desk->workarea.y == y && desk->workarea.width == w && desk->workarea.height == h) {
		return (0);
	}
	desk->workarea.x = x;
	desk->workarea.y = y;
	desk->workarea.width = w;
	desk->workarea.height = h;
	/* leave at least 1/2 a cell ((36,42) pixels) around the desktop area to
	   accomodate window managers that do not account for panels. */
	cols = desk->cols = (int) (w / ICON_WIDE);
	rows = desk->rows = (int) (h / ICON_HIGH);
	xoff = desk->xoff = (int) ((w - cols * ICON_WIDE) / 2);
	yoff = desk->yoff = (int) ((h - rows * ICON_HIGH) / 2);
#else
	(void) desk;
#endif
	return (1);
}
static void
next_cell(XdeDesktop *desk, int *col, int *row, int *x, int *y)
{
	*row += 1;
	*y += ICON_HIGH;
	if (*row >= desk->rows) {
		*row = 0;
		*y = desk->rows;
		*col += 1;
		*x += ICON_WIDE;
	}
}

/** @brief remove icon from corresponding desktop
  */
static void
xde_icon_remove(XdeIcon *icon, GtkWidget *table)
{
	if (icon->attached.row != -1 && icon->attached.col != -1) {
		gtk_container_remove(GTK_CONTAINER(table), icon->widget);
		icon->attached.col = -1;
		icon->attached.row = -1;
	}
}

/** @brief place the icon on the corresponding desktop at column and row
  */
static void
xde_icon_place(XdeIcon *icon, GtkWidget *table, int col, int row)
{
	xde_icon_remove(icon, table);
	gtk_table_attach_defaults(GTK_TABLE(table), icon->widget, col, col + 1, row, row + 1);
	icon->attached.col = col;
	icon->attached.row = row;
}

/** @brief arrange icons
  *
  * Arranges (places) all of the desktop icons.  The placement is performed
  * by arranging each icon and asking it to place itself, and update its
  * contents.
  */
static void
xde_desktop_arrange_icons(XdeDesktop *desk)
{
	int col = 0, row = 0;
	int x = desk->xoff, y = desk->yoff;
	XdeScreen *xscr = desk->xscr;

	if (desk->icons.links && col < desk->cols) {
		GList *links;

		for (links = desk->icons.links; links; links = links->next) {
			xde_icon_place(links->data, xscr->table, col, row);
			desk->icons.detop = g_list_append(desk->icons.detop, links->data);
			next_cell(desk, &col, &row, &x, &y);
			if (col < desk->cols)
				continue;
			break;
		}
	}
	if (desk->icons.dires && col < desk->cols) {
		GList *dires;

		for (dires = desk->icons.dires; dires; dires = dires->next) {
			xde_icon_place(dires->data, xscr->table, col, row);
			desk->icons.detop = g_list_append(desk->icons.detop, dires->data);
			next_cell(desk, &col, &row, &x, &y);
			if (col < desk->cols)
				continue;
			break;
		}
	}
	if (desk->icons.files && col < desk->cols) {
		GList *files;

		for (files = desk->icons.files; files; files = files->next) {
			xde_icon_place(files->data, xscr->table, col, row);
			desk->icons.detop = g_list_append(desk->icons.detop, files->data);
			next_cell(desk, &col, &row, &x, &y);
			if (col < desk->cols)
				continue;
			break;
		}
	}
}

/** @brief hide icons
  *
  * Shows all of the desktop icons.  The method simply requests that each
  * icon hide itself.
  */
void
xde_desktop_hide_icons(XdeDesktop *desk)
{
	GList *detop;

	for (detop = desk->icons.detop; detop; detop = detop->next)
		xde_icon_hide(detop->data);
}


/** @brief show icons
  *
  * Shows all of the desktop icons.  The method simply requests that each
  * icon show itself.
  */
static void
xde_desktop_show_icons(XdeDesktop *desk)
{
	GList *detop;

	for (detop = desk->icons.detop; detop; detop = detop->next)
		xde_icon_show(detop->data);
}

/** @brief update the desktop
  *
  * Create or updates the complete desktop arrangement, including reading
  * or rereading the $HOME/Desktop directory.
  */
void
update_desktop(XdeDesktop *desk)
{
	OPRINTF("==> Reading desktop...\n");
	xde_desktop_read_desktop(desk);
	OPRINTF("==> Creating objects...\n");
	xde_desktop_create_objects(desk);
	OPRINTF("==> Creating windows...\n");
	xde_desktop_create_windows(desk);
	OPRINTF("==> Calculating cells...\n");
	xde_desktop_calculate_cells(desk);
	OPRINTF("==> Arranging icons...\n");
	xde_desktop_arrange_icons(desk);
	OPRINTF("==> Showing icons...\n");
	xde_desktop_show_icons(desk);
}

static void refresh_layout(XdeScreen *xscr);
static void refresh_desktop(XdeScreen *xscr);

static gboolean
on_deferred_refresh_layout(gpointer data)
{
	XdeScreen *xscr = data;

	xscr->deferred_refresh_layout = 0;
	refresh_layout(xscr);
	return G_SOURCE_REMOVE;
}

static gboolean
on_deferred_refresh_desktop(gpointer data)
{
	XdeScreen *xscr = data;

	xscr->deferred_refresh_desktop = 0;
	refresh_desktop(xscr);
	return G_SOURCE_REMOVE;
}

static void
add_deferred_refresh_layout(XdeScreen *xscr)
{
	if (!xscr->deferred_refresh_layout)
		xscr->deferred_refresh_layout =
			g_idle_add(on_deferred_refresh_layout, xscr);
	if (xscr->deferred_refresh_desktop) {
		g_source_remove(xscr->deferred_refresh_desktop);
		xscr->deferred_refresh_desktop = 0;
	}
}

static void
add_deferred_refresh_desktop(XdeScreen *xscr)
{
	if (xscr->deferred_refresh_layout)
		return;
	if (xscr->deferred_refresh_desktop)
		return;
	xscr->deferred_refresh_desktop =
		g_idle_add(on_deferred_refresh_desktop, xscr);
}

void
xde_pixmap_ref(XdePixmap *pixmap)
{
	if (pixmap)
		pixmap->refs++;
}

void
xde_pixmap_delete(XdeScreen *xscr, XdePixmap *pixmap)
{
	if (pixmap) {
		if (pixmap->pmap)
			XDeleteContext(GDK_DISPLAY_XDISPLAY(xscr->disp), pixmap->pmap,
				       xscr->pixmaps);
		if (pixmap->pixmap)
			g_object_unref(pixmap->pixmap);
		free(pixmap);
	}
}

void
xde_pixmap_unref(XdeScreen *xscr, XdePixmap **pixmapp)
{
	if (pixmapp && *pixmapp) {
		if (--(*pixmapp)->refs <= 0) {
			xde_pixmap_delete(xscr, *pixmapp);
			*pixmapp = NULL;
		}
	}
}

static Window
get_selection(Bool replace, Window selwin)
{
	char selection[64] = { 0, };
	GdkDisplay *disp;
	Display *dpy;
	int s, nscr;
	Window owner;
	Atom atom;
	Window gotone = None;

	DPRINT();
	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	for (s = 0; s < nscr; s++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF("No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF("Setting owner of %s to 0x%08lx from 0x%08lx\n", selection,
				selwin, owner);
			XSetSelectionOwner(dpy, atom, selwin, CurrentTime);
			XSync(dpy, False);
		}
		if (!gotone && owner)
			gotone = owner;
	}
	if (replace) {
		if (gotone) {
			if (selwin)
				DPRINTF("%s: replacing running instance\n", NAME);
			else
				DPRINTF("%s: quitting running instance\n", NAME);
		} else {
			if (selwin)
				DPRINTF("%s: no running instance to replace\n", NAME);
			else
				DPRINTF("%s: no running instance to quit\n", NAME);
		}
		if (selwin) {
			XEvent ev;
			Atom manager = XInternAtom(dpy, "MANAGER", False);
			GdkScreen *scrn;
			Window root;

			for (s = 0; s < nscr; s++) {
				scrn = gdk_display_get_screen(disp, s);
				root = GDK_WINDOW_XID(gdk_screen_get_root_window(scrn));
				snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
				atom = XInternAtom(dpy, selection, False);

				ev.xclient.type = ClientMessage;
				ev.xclient.serial = 0;
				ev.xclient.send_event = False;
				ev.xclient.display = dpy;
				ev.xclient.window = root;
				ev.xclient.message_type = manager;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = CurrentTime;	/* FIXME:
									   mimestamp */
				ev.xclient.data.l[1] = atom;
				ev.xclient.data.l[2] = selwin;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;

				XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
				XFlush(dpy);
			}
		}
	} else if (gotone)
		DPRINTF("%s: not replacing running instance\n", NAME);
	return (gotone);
}

#if 0
static GdkFilterReturn laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static Window
get_desktop_layout_selection(XdeScreen *xscr)
{
	GdkDisplay *disp = gdk_screen_get_display(xscr->scrn);
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	Window root = RootWindow(dpy, xscr->index);
	char selection[64] = { 0, };
	GdkWindow *lay;
	Window owner;
	Atom atom;

	if (xscr->laywin)
		return None;
	
	xscr->laywin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XSelectInput(dpy, xscr->laywin,
		StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);
	lay = gdk_x11_window_foreign_new_for_display(disp, xscr->laywin);
	gdk_window_add_filter(lay, laywin_handler, xscr);
	snprintf(selection, sizeof(selection), XA_NET_DESKTOP_LAYOUT, xscr->index);
	atom = XInternAtom(dpy, selection, False);
	if (!(owner = XGetSelectionOwner(dpy, atom)))
		DPRINTF("No owner for %s\n", selection);
	XSetSelectionOwner(dpy, atom, xscr->laywin, CurrentTime);
	XSync(dpy, False);
	
	if (xscr->laywin) {
		XEvent ev;

		ev.xclient.type = ClientMessage;
		ev.xclient.serial = 0;
		ev.xclient.send_event = False;
		ev.xclient.display = dpy;
		ev.xclient.window = root;
		ev.xclient.message_type = XInternAtom(dpy, "MANAGER", False);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = CurrentTime;
		ev.xclient.data.l[1] = atom;
		ev.xclient.data.l[2] = xscr->laywin;
		ev.xclient.data.l[3] = 0;
		ev.xclient.data.l[4] = 0;

		XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
		XFlush(dpy);
	}
	return (owner);
}
#endif

static void
workspace_destroyed(WnckScreen *wnck, WnckWorkspace *space, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	(void) wnck;
	(void) space;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
workspace_created(WnckScreen *wnck, WnckWorkspace *space, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	(void) wnck;
	(void) space;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static Bool
good_window_manager(XdeScreen *xscr)
{
	DPRINT();
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	/* XXX: 2bwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "2bwm"))
		return True;
	/* XXX: adwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "adwm"))
		return True;
	/* XXX: awewm(1) does not really support desktops and is, therefore, not
	   supported.  (Welllll, it does.) */
	if (!strcasecmp(xscr->wmname, "aewm"))
		return True;
	/* XXX: afterstep(1) provides both workspaces and viewports (large desktops).
	   libwnck+ does not support these well, so when xde-pager detects that it is
	   running under afterstep(1), it does nothing.  (It has a desktop button proxy,
	   but it does not relay scroll wheel events by default.) */
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return False;
	/* XXX: awesome(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "awesome"))
		return True;
	/* XXX: blackbox(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return True;
	/* XXX: bspwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "baspwm"))
		return True;
	/* XXX: ctwm(1) is only GNOME/WinWM compliant and is not yet supported by
	   libwnck+.  Use etwm(1) instead.  xde-pager mitigates this somewhat, so it is
	   still listed as supported. */
	if (!strcasecmp(xscr->wmname, "ctwm"))
		return True;
	/* XXX: cwm(1) is supported, but it doesn't work that well because cwm(1) is not
	   placing _NET_WM_STATE on client windows, so libwnck+ cannot locate them and
	   will not provide contents in the pager. */
	if (!strcasecmp(xscr->wmname, "cwm"))
		return True;
	/* XXX: dtwm(1) is only OSF/Motif compliant and does support multiple desktops;
	   however, libwnck+ does not yet support OSF/Motif/CDE.  This is not mitigated
	   by xde-pager. */
	if (!strcasecmp(xscr->wmname, "dtwm"))
		return False;
	/* XXX: dwm(1) is barely ICCCM compliant.  It is not supported. */
	if (!strcasecmp(xscr->wmname, "dwm"))
		return False;
	/* XXX: echinus(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "echinus"))
		return True;
	/* XXX: etwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "etwm"))
		return True;
	/* XXX: failsafewm(1) has no desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "failsafewm"))
		return False;
	/* XXX: fluxbox(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "fluxbox"))
		return True;
	/* XXX: flwm(1) supports GNOME/WinWM but not EWMH/NetWM and is not currently
	   supported by libwnck+.  xde-pager mitigates this to some extent. */
	if (!strcasecmp(xscr->wmname, "flwm"))
		return True;
	/* XXX: fvwm(1) is supported and works well.  fvwm(1) provides a desktop button
	   proxy, but it needs the --noproxy option.  Viewports work better than on
	   afterstep(1) and behaviour is predictable. */
	if (!strcasecmp(xscr->wmname, "fvwm"))
		return True;
	/* XXX: herbstluftwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "herbstluftwm"))
		return True;
	/* XXX: i3(1) is supported; however, i3(1) does not support
	   _NET_NUMBER_OF_DESKTOPS, so only the current desktop is shown at any given
	   time, which makes it less effective. */
	if (!strcasecmp(xscr->wmname, "i3"))
		return True;
	/* XXX: icewm(1) provides its own pager on the panel, but does respect
	   _NET_DESKTOP_LAYOUT in some versions.  Although a desktop button proxy is
	   provided, older versions of icewm(1) will not proxy butt events sent by the
	   pager.  Use the version at https://github.com/bbidulock/icewm for best
	   results. */
	if (!strcasecmp(xscr->wmname, "icewm"))
		return True;
	/* XXX: jwm(1) provides its own pager on the panel, but does not respect or set
	   _NET_DESKTOP_LAYOUT, and key bindings are confused.  When xde-pager detects
	   that it is running under jwm(1) it will simply do nothing. */
	if (!strcasecmp(xscr->wmname, "jwm"))
		return False;
	/* XXX: matwm2(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "matwm2"))
		return True;
	/* XXX: metacity(1) provides its own competent desktop switching feedback pop-up. 
	   When xde-pager detects that it is running under metacity(1), it will simply do 
	   nothing. */
	if (!strcasecmp(xscr->wmname, "metacity"))
		return False;
	/* XXX: mwm(1) only supports OSF/Motif and does not support multiple desktops. It 
	   is not supported. */
	if (!strcasecmp(xscr->wmname, "mwm"))
		return False;
	/* XXX: mutter(1) has not been tested. */
	if (!strcasecmp(xscr->wmname, "mutter"))
		return True;
	/* XXX: openbox(1) provides its own meager desktop switching feedback pop-up.  It 
	   does respect _NET_DESKTOP_LAYOUT but does not provide any of the contents of
	   the desktop. When both are running it is a little confusing, so when xde-pager 
	   detects that it is running under openbox(1), it will simply do nothing. */
	if (!strcasecmp(xscr->wmname, "openbox"))
		return False;
	/* XXX: pekwm(1) provides its own broken desktop switching feedback pop-up;
	   however, it does not respect _NET_DESKTOP_LAYOUT and key bindings are
	   confused.  When xde-pager detects that it is running under pekwm(1), it will
	   simply do nothing. */
	if (!strcasecmp(xscr->wmname, "pekwm"))
		return False;
	/* XXX: spectrwm(1) is supported, but it doesn't work that well because, like
	   cwm(1), spectrwm(1) is not placing _NET_WM_STATE on client windows, so
	   libwnck+ cannot locate them and will not provide contents in the pager. */
	if (!strcasecmp(xscr->wmname, "spectrwm"))
		return True;
	/* XXX: twm(1) does not support multiple desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "twm"))
		return False;
	/* XXX: uwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "uwm"))
		return True;
	/* XXX: vtwm(1) is barely ICCCM compliant and currently unsupported: use etwm
	   instead. */
	if (!strcasecmp(xscr->wmname, "vtwm"))
		return False;
	/* XXX: waimea(1) is supported; however, waimea(1) defaults to triple-sized large 
	   desktops in a 2x2 arrangement.  With large virtual desktops, libwnck+ gets
	   confused just as with afterstep(1).  fvwm(1) must be doing something right. It 
	   appears to be _NET_DESKTOP_VIEWPORT, which is supposed to be set to the
	   viewport position of each desktop (and isn't).  Use the waimea at
	   https://github.com/bbidulock/waimea for a corrected version. */
	if (!strcasecmp(xscr->wmname, "waimea"))
		return True;
	/* XXX: wind(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wind"))
		return True;
	/* XXX: wmaker(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wmaker"))
		return True;
	/* XXX: wmii(1) is supported and works well.  wmii(1) was stealing the focus back 
	   from the pop-up, but this was fixed. */
	if (!strcasecmp(xscr->wmname, "wmii"))
		return True;
	/* XXX: wmx(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wmx"))
		return True;
	/* XXX: xdwm(1) does not support EWMH/NetWM for desktops. */
	if (!strcasecmp(xscr->wmname, "xdwm"))	/* XXX */
		return False;
	/* XXX: yeahwm(1) does not support EWMH/NetWM and is currently unsupported.  The
	   pager will simply not do anything while this window manager is running. */
	if (!strcasecmp(xscr->wmname, "yeahwm"))
		return True;
	return True;
}

static void setup_button_proxy(XdeScreen *xscr);

static void
window_manager_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	const char *name;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	wnck_screen_force_update(wnck);
	if (options.proxy)
		setup_button_proxy(xscr);
	free(xscr->wmname);
	xscr->wmname = NULL;
	xscr->goodwm = False;
	if ((name = wnck_screen_get_window_manager_name(wnck))) {
		xscr->wmname = strdup(name);
		*strchrnul(xscr->wmname, ' ') = '\0';
		/* Some versions of wmx have an error in that they only set the
		   _NET_WM_NAME to the first letter of wmx. */
		if (!strcmp(xscr->wmname, "w")) {
			free(xscr->wmname);
			xscr->wmname = strdup("wmx");
		}
		/* Ahhhh, the strange naming of μwm...  Unfortunately there are several
		   ways to make a μ in utf-8!!! */
		if (!strcmp(xscr->wmname, "\xce\xbcwm") || !strcmp(xscr->wmname, "\xc2\xb5wm")) {
			free(xscr->wmname);
			xscr->wmname = strdup("uwm");
		}
		xscr->goodwm = good_window_manager(xscr);
	}
	DPRINTF("window manager is '%s'\n", xscr->wmname);
	DPRINTF("window manager is %s\n", xscr->goodwm ? "usable" : "unusable");
}

static void
something_changed(WnckScreen *wnck, XdeScreen *xscr)
{
	(void) wnck;
	(void) xscr;
}

static void
viewports_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(wnck, xscr);
}

static void
background_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	(void) wnck;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
active_workspace_changed(WnckScreen *wnck, WnckWorkspace *prev, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	(void) prev;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(wnck, xscr);
}

static GdkFilterReturn proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
setup_button_proxy(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index), proxy;
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;

	DPRINT();
	xscr->proxy = NULL;
	if (XGetWindowProperty(dpy, root, _XA_WIN_DESKTOP_BUTTON_PROXY,
			       0, 1, False, XA_CARDINAL, &actual, &format,
			       &nitems, &after, (unsigned char **) &data) == Success &&
	    format == 32 && nitems >= 1 && data) {
		proxy = data[0];
		if ((xscr->proxy = gdk_x11_window_foreign_new_for_display(xscr->disp, proxy))) {
			GdkEventMask mask;

			mask = gdk_window_get_events(xscr->proxy);
			mask |=
			    GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK |
			    GDK_SUBSTRUCTURE_MASK;
			gdk_window_set_events(xscr->proxy, mask);
			DPRINTF("adding filter for desktop button proxy\n");
			gdk_window_add_filter(xscr->proxy, proxy_handler, xscr);
		}
	}
	if (data) {
		XFree(data);
		data = NULL;
	}
}

static void
update_current_desktop(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, i, j = 0, *x;
	unsigned long *data = NULL;
	Bool changed = False;
	XdeMonitor *xmon;

	DPRINT();
	if (prop == None || prop == _XA_WM_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_WM_DESKTOP, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= (unsigned long) xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < (unsigned long) xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_WIN_WORKSPACE) {
		if (XGetWindowProperty(dpy, root, _XA_WIN_WORKSPACE, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= (unsigned long) xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < (unsigned long) xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_NET_CURRENT_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_NET_CURRENT_DESKTOP, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= (unsigned long) xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < (unsigned long) xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (changed) {
		DPRINTF("Current desktop changed.\n");
		add_deferred_refresh_desktop(xscr);
	} else
		DPRINTF("No change in current desktop.\n");
}

static void
update_workarea(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, *data = NULL, i, d;
	long length;
	Bool propok = False, changed = False;
	XdeDesktop *desk;
	GdkRectangle workarea = { 0, };

	DPRINT();

	if (prop == None || prop == _XA_WIN_WORKAREA) {
		length = 4;
	      win_again:
		if (XGetWindowProperty
		    (dpy, root, _XA_WIN_WORKAREA, 0, length, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 4 && data) {
			if (after > 0 && length == 4) {
				XFree(data);
				data = NULL;
				length += (after + 3) >> 2;
				goto win_again;
			}
			workarea.x = data[0];
			workarea.y = data[1];
			workarea.width = data[2] - workarea.x;
			workarea.height = data[3] - workarea.y;
			desk = xscr->desk;
			if (desk->workarea.x != workarea.x ||
			    desk->workarea.y != workarea.y ||
			    desk->workarea.width != workarea.width ||
			    desk->workarea.height != workarea.height) {
				desk->workarea.x = workarea.x;
				desk->workarea.y = workarea.y;
				desk->workarea.width = workarea.width;
				desk->workarea.height = workarea.height;
				changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (prop == None || prop == _XA_NET_WORKAREA) {
		length = 4;
	      net_again:
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_WORKAREA, 0, length, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 4 && data) {
			if (after > 0 && length == 4) {
				XFree(data);
				data = NULL;
				length += (after + 3) >> 2;
				goto net_again;
			}
			for (i = 0, d = 0; d < (nitems >> 2) && d < (unsigned long) xscr->desks; d++, i += 4) {
				XdeDesktop *desk;

				workarea.x = data[i + 0];
				workarea.y = data[i + 1];
				workarea.width = data[i + 2];
				workarea.height = data[i + 3];
				desk = g_ptr_array_index(xscr->desktops, d);
				if (desk->workarea.x != workarea.x ||
				    desk->workarea.y != workarea.y ||
				    desk->workarea.width != workarea.width ||
				    desk->workarea.height != workarea.height) {
					desk->workarea.x = workarea.x;
					desk->workarea.y = workarea.y;
					desk->workarea.width = workarea.width;
					desk->workarea.height = workarea.height;
					changed = True;
				}
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (!propok)
		EPRINTF("wrong property passed\n");
	if (changed) {
		/* FIXME: we need to update the layout of icons on the desktop. */
	}
}

static void update_root_pixmap(XdeScreen *xscr, Atom prop);

static void
init_window(XdeScreen *xscr)
{
	GtkWindow *win;
	GdkWindow *window;
	char *geometry;
	GtkWidget *aln;
	GtkWidget *tab;

#if 0
	GtkTargetEntry *targets;
	GtkWidget *fix;
#endif
	GdkGeometry hints = {
		.min_width = xscr->width,
		.min_height = xscr->height,
		.max_width = xscr->width,
		.max_height = xscr->height,
	};

	xscr->desktop = win = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_screen(win, xscr->scrn);
	gtk_window_set_accept_focus(win, FALSE);
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_window_set_decorated(win, FALSE);
	gtk_window_set_default_size(win, xscr->width, xscr->height);
	gtk_window_set_geometry_hints(win, GTK_WIDGET(win), &hints,
				      GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_USER_SIZE);
	gtk_window_set_deletable(win, FALSE);
	gtk_window_set_focus_on_map(win, FALSE);
#if 0
	gtk_window_set_frame_dimensions(win, 0, 0, 0, 0);
	gtk_window_fullscreen(win);
#endif
	gtk_window_set_gravity(win, GDK_GRAVITY_STATIC);
	gtk_window_set_has_frame(win, FALSE);
#if 0
	gtk_window_set_keep_below(win, TRUE);
#endif
	gtk_window_move(win, 0, 0);
	gtk_window_set_opacity(win, 1.0);
	gtk_window_set_position(win, GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_resizable(win, FALSE);
	gtk_window_resize(win, xscr->width, xscr->height);
	gtk_window_set_skip_pager_hint(win, TRUE);
	gtk_window_set_skip_taskbar_hint(win, TRUE);
	gtk_window_stick(win);
	gtk_window_set_type_hint(win, GDK_WINDOW_TYPE_HINT_DESKTOP);

	gtk_widget_set_app_paintable(GTK_WIDGET(win), TRUE);
	if (!gtk_widget_get_double_buffered(GTK_WIDGET(win)))
		gtk_widget_set_double_buffered(GTK_WIDGET(win), TRUE);
#if 0
	targets = calloc(5, sizeof(*targets));

	targets[0].target = "text/uri-list";
	targets[0].flags = 0;
	targets[0].info = TARGET_URI_LIST;

	targets[1].target = "text/x-moz-url";
	targets[1].flags = 0;
	targets[1].info = TARGET_MOZ_URL;

	targets[2].target = "XdndDirectSave0";
	targets[2].flags = 0;
	targets[2].info = TARGET_XDS;

	targets[3].target = "application/octet-stream";
	targets[3].flags = 0;
	targets[3].info = TARGET_RAW;

	gtk_drag_dest_set(GTK_WIDGET(win), GTK_DEST_DEFAULT_DROP, targets, 4,
			  GDK_ACTION_COPY | GDK_ACTION_ASK | GDK_ACTION_MOVE |
			  GDK_ACTION_LINK | GDK_ACTION_PRIVATE);
	gtk_drag_dest_add_image_targets(GTK_WIDGET(win));
	gtk_drag_dest_add_text_targets(GTK_WIDGET(win));
	gtk_drag_dest_add_uri_targets(GTK_WIDGET(win));

	free(targets);
#endif
	gtk_widget_set_size_request(GTK_WIDGET(win), xscr->width, xscr->height);
	geometry = g_strdup_printf("%dx%d+0+0", xscr->width, xscr->height);
	gtk_window_parse_geometry(win, geometry);
	g_free(geometry);
	gtk_widget_add_events(GTK_WIDGET(win),
			      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			      GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK |
			      GDK_BUTTON3_MOTION_MASK);
	gtk_widget_realize(GTK_WIDGET(win));
	window = gtk_widget_get_window(GTK_WIDGET(win));
	gdk_window_set_override_redirect(window, TRUE);
	gdk_window_set_back_pixmap(window, NULL, TRUE);

#if 0
	g_signal_connect(G_OBJECT(win), "drag_drop", G_CALLBACK(drag_drop), xscr);
	g_signal_connect(G_OBJECT(win), "drag_data_received", G_CALLBACK(drag_data_received), xscr);
	g_signal_connect(G_OBJECT(win), "drag_motion", G_CALLBACK(drag_motion), xscr);
	g_signal_connect(G_OBJECT(win), "drag_leave", G_CALLBACK(drag_leave), xscr);
#endif

	aln = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
//	fix = gtk_fixed_new();
//	gtk_widget_set_size_request(fix, xscr->width, xscr->height);

	gtk_container_set_border_width(GTK_CONTAINER(win), 0);
//	gtk_container_add(GTK_CONTAINER(win), fix);
	gtk_container_add(GTK_CONTAINER(win), aln);

	xscr->table = tab = gtk_table_new(1, 1, TRUE);

	gtk_table_set_col_spacings(GTK_TABLE(tab), 0);
	gtk_table_set_row_spacings(GTK_TABLE(tab), 0);
	gtk_table_set_homogeneous(GTK_TABLE(tab), TRUE);
	gtk_widget_set_size_request(tab, ICON_WIDE, ICON_HIGH);
//      gtk_widget_set_tooltip_text(tab, "Click Me!");
//	gtk_fixed_put(GTK_FIXED(fix), tab, 0, 0);

	gtk_container_add(GTK_CONTAINER(aln), tab);

	update_root_pixmap(xscr, None);

	gtk_widget_show(tab);
//	gtk_widget_show(fix);
	gtk_widget_show(aln);
	gtk_widget_show(GTK_WIDGET(win));

	gdk_window_lower(window);

}

static void
update_screen_size(XdeScreen *xscr, int new_width, int new_height)
{
	(void) xscr;
	(void) new_width;
	(void) new_height;
}

static void
create_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
	(void) xscr;
	(void) m;
	memset(mon, 0, sizeof(*mon));
}

static void
delete_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
	(void) xscr;
	(void) mon;
	(void) m;
}

static void
update_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
	(void) xscr;
	(void) mon;
	(void) m;
}

static void
update_screen(XdeScreen *xscr)
{
	(void) xscr;
}

static void
update_root_pixmap(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Pixmap pmap = None;
	XdeDesktop *desk = xscr->desk;

	DPRINT();
	if (prop == None || prop == _XA_ESETROOT_PMAP_ID) {
		if (XGetWindowProperty
		    (dpy, root, _XA_ESETROOT_PMAP_ID, 0, 1, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			pmap = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_XROOTPMAP_ID) {
		if (XGetWindowProperty
		    (dpy, root, _XA_XROOTPMAP_ID, 0, 1, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			pmap = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (pmap && (!desk->pixmap || desk->pixmap->pmap != pmap)) {
		if (desk->pixmap)
			xde_pixmap_unref(xscr, &desk->pixmap);
		if (pmap)
			xde_pixmap_ref((desk->pixmap = get_pixmap(xscr, pmap)));
		set_style(xscr);
	}
}

static void
refresh_screen(XdeScreen *xscr, GdkScreen *scrn)
{
	XdeMonitor *mon;
	int m, nmon, width, height, index;

	index = gdk_screen_get_number(scrn);
	if (xscr->index != index) {
		EPRINTF("Arrrghhh! screen index changed from %d to %d\n", xscr->index, index);
		xscr->index = index;
	}
	if (xscr->scrn != scrn) {
		DPRINTF("Arrrghhh! screen pointer changed from %p to %p\n", xscr->scrn, scrn);
		xscr->scrn = scrn;
	}
	width = gdk_screen_get_width(scrn);
	height = gdk_screen_get_height(scrn);
	if (xscr->width != width || xscr->height != height) {
		DPRINTF("Screen %d dimensions changed %dx%d -> %dx%d\n", index,
			xscr->width, xscr->height, width, height);
		/* FIXME: reset size of screen */
		update_screen_size(xscr, width, height);
		xscr->width = width;
		xscr->height = height;
	}
	nmon = gdk_screen_get_n_monitors(scrn);
	DPRINTF("Reallocating %d monitors\n", nmon);
	xscr->mons = realloc(xscr->mons, nmon * sizeof(*xscr->mons));
	if (nmon > xscr->nmon) {
		DPRINTF("Screen %d number of monitors increased from %d to %d\n",
			index, xscr->nmon, nmon);
		for (m = xscr->nmon; m < nmon; m++) {
			mon = xscr->mons + m;
			create_monitor(xscr, mon, m);
		}
	} else if (nmon < xscr->nmon) {
		DPRINTF("Screen %d number of monitors decreased from %d to %d\n",
			index, xscr->nmon, nmon);
		for (m = nmon; m < xscr->nmon; m++) {
			mon = xscr->mons + m;
			delete_monitor(xscr, mon, m);
		}
	}
	if (nmon != xscr->nmon)
		xscr->nmon = nmon;
	for (m = 0, mon = xscr->mons; m < nmon; m++, mon++)
		update_monitor(xscr, mon, m);
	update_screen(xscr);
}

/** @brief monitors changed
  *
  * The number and/or size of monitors belonging to a screen have changed.  This
  * may be as a result of RANDR or XINERAMA changes.  Walk through the monitors
  * and adjust the necessary parameters.
  */
static void
on_monitors_changed(GdkScreen *scrn, gpointer xscr)
{
	refresh_screen(xscr, scrn);
}

/** @brief screen size changed
  *
  * The size of the screen changed.  This may be as a result of RANDR or
  * XINERAMA changes.  Walk through the screen and the monitors on the screen
  * and adjust the necessary parameters.
  */
static void
on_size_changed(GdkScreen *scrn, gpointer xscr)
{
	refresh_screen(xscr, scrn);
}

static void
init_monitors(XdeScreen *xscr)
{
	int m;
	XdeMonitor *mon;

	g_signal_connect(G_OBJECT(xscr->scrn), "monitors-changed",
			G_CALLBACK(on_monitors_changed), xscr);
	g_signal_connect(G_OBJECT(xscr->scrn), "size-changed",
			G_CALLBACK(on_size_changed), xscr);

	xscr->nmon = gdk_screen_get_n_monitors(xscr->scrn);
	xscr->mons = calloc(xscr->nmon, sizeof(*xscr->mons));
	for (m = 0, mon = xscr->mons; m < xscr->nmon; m++, mon++) {
		mon->index = m;
		gdk_screen_get_monitor_geometry(xscr->scrn, m, &mon->geom);
	}
}

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	g_signal_connect(G_OBJECT(wnck), "workspace_destroyed",
			 G_CALLBACK(workspace_destroyed), xscr);
	g_signal_connect(G_OBJECT(wnck), "workspace_created",
			 G_CALLBACK(workspace_created), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_manager_changed",
			 G_CALLBACK(window_manager_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "viewports_changed",
			 G_CALLBACK(viewports_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "background_changed",
			 G_CALLBACK(background_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "active_workspace_changed",
			 G_CALLBACK(active_workspace_changed), xscr);

#if 0
	g_signal_connect(G_OBJECT(wnck), "showing_desktop_changed",
			G_CALLBACK(showing_desktop_changed), xscr);
#endif

	wnck_screen_force_update(wnck);
	window_manager_changed(wnck, xscr);
}

static GdkFilterReturn selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static GdkFilterReturn root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static void update_layout(XdeScreen *xscr, Atom prop);
static void update_theme(XdeScreen *xscr, Atom prop);


/** @brief get the mime type of a file
  *
  * Get the mime type for the specified file.  This method uses gnome_vfs_uri
  * to obtain information about the mime type of the file.  The XDG
  * shared-mime specification could be used directly, however, gnome-vfs-2.0
  * gives good results for the most part.  As a fall back, when gnome-vfs-2.0
  * cannot determine the mime type, the "file" program is queried.  "file"
  * gives less consistent requlest thatn gnome-vfs-2.0.  The previous two
  * approaches examine the file but do not consider the name.  As a final fall
  * back, gnome-vfs-2.0 is used to query for a mime type based solely on the
  * file name.  Heuristically, this approach gives good results for
  * determining the mime types of any file.
  */
static char *
get_mime_type_vfs(const char *file)
{
	GnomeVFSFileInfo *info;
	char *mime = NULL;
	const char *type;

	if ((info = gnome_vfs_file_info_new())) {
		if (gnome_vfs_get_file_info(file, info,
					    GNOME_VFS_FILE_INFO_DEFAULT |
					    GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					    GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE) ==
		    GNOME_VFS_OK) {
			if ((type = gnome_vfs_file_info_get_mime_type(info)))
				mime = strdup(type);
		}
		gnome_vfs_file_info_unref(info);
	}
	if (!mime) {
		FILE *cmd;
		char *command;

		command = g_strdup_printf("file -b --mime-type \"%s\"", file);
		if ((cmd = popen(command, "r"))) {
			char buffer[BUFSIZ] = { 0, };

			if (fgets(buffer, BUFSIZ, cmd)) {
				*strchrnul(buffer, '\n') = '\0';
				mime = strdup(buffer);
			}
			pclose(cmd);
		}
		g_free(command);
	}
	if (!mime) {
		if ((type = gnome_vfs_get_mime_type_for_name(file)))
			mime = strdup(type);
	}
	return (mime);
}

/** @brief gio version of get_mime_type()
  */
static char *
get_mime_type_gio(const char *filename)
{
	GFile *file;
	GFileInfo *info;
	char *mime = NULL;
	const char *type;

	if ((file = g_file_new_for_path(filename))) {
		if ((info = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					      G_FILE_QUERY_INFO_NONE, NULL, NULL))) {
			if ((type = g_file_info_get_attribute_string(info,
								     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)))
				mime = g_content_type_get_mime_type(type);
			g_object_unref(info);
		}
		g_object_unref(file);
	}
	return (mime);
}

char *
get_mime_type(const char *filename)
{
	char *mime;

	if (!(mime = get_mime_type_gio(filename)))
		mime = get_mime_type_vfs(filename);
	return (mime);
}

/** @brief get icon names for a content type
  *
  * Given a mime type, returns a list of icon names, or NULL when
  * unsuccessful.  The icon names are in order of preference, starting with
  * the mime type supplied, any aliases of that mime type, and any subclasses
  * of that mime type.  The purpose of this method is to always find a
  * reasonable icon representation of the mime type.  Use when displaying an
  * icon for a given desktop object.
  */
static GList *
get_icons_vfs(const char *mime)
{
	GList *icons = NULL, *aliases, *mimes = NULL, *list, *classes;

	if (!mime)
		return (icons);
	mimes = g_list_append(mimes, g_strdup(mime));
	aliases = g_hash_table_lookup(MIME_ALIASES, mime);
	for (list = aliases; list; list = list->next)
		mimes = g_list_append(mimes, g_strdup(list->data));
	classes = g_hash_table_lookup(MIME_SUBCLASSES, mime);
	for (list = classes; list; list = list->next)
		mimes = g_list_append(mimes, g_strdup(list->data));
	for (list = mimes; list; list = list->next) {
		char *p, *icon1, *icon2, *icon3;

		p = icon1 = g_strdup(list->data);
		while ((p = strchr(p, '/')))
			*p = '-';
		icons = g_list_append(icons, g_strdup_printf("gnome-mime-%s", icon1));
		g_free(icon1);
		if ((icon3 = g_hash_table_lookup(MIME_GENERIC_ICONS, list->data)))
			icons = g_list_append(icons, g_strdup(icon3));
		icon2 = g_strdup(list->data);
		*strchrnul(icon2, '/') = '\0';
		icons = g_list_append(icons, g_strdup_printf("gnome-mime-%s", icon2));
		g_free(icon2);
	}
	return (icons);
}

/** @brief gio version of get_icons().
  */
static GList *
get_icons_gio(const char *mime)
{
	GList *icons = NULL;
	GIcon *icon;
	char *name, *type;

	if (!(type = g_content_type_from_mime_type(mime)))
		return (icons);
	if ((icon = g_content_type_get_icon(type))) {
		if ((name = g_icon_to_string(icon)))
			icons = g_list_append(icons, name);
		g_object_unref(icon);
	}
	if ((name = g_content_type_get_generic_icon_name(type)))
		icons = g_list_append(icons, name);
	g_free(type);
	return (icons);
}

GList *
get_icons(const char *mime)
{
	GList *list;

	if (!(list = get_icons_gio(mime)))
		list = get_icons_vfs(mime);
	return (list);
}

/** @brief application ids for a mime type
  *
  * Given a mime type, returns a list of the primary application ids and the
  * subclass application ids associated wtih the mime type.  This is used for
  * determining which applications should be used to open a given desktop
  * file, and which subclass applications can be used as a fall back or for
  * opening a desktop file using a subclass mime type.  For example, a browser
  * (firefox) could be returned in the apps list for "text/html" and a text
  * editor (vim) in the subs list.
  */
void
get_apps_and_subs_vfs(const char *mime, GList **apps_p, GList **subs_p)
{
	GList *apps = NULL, *subs = NULL, *list;
	gpointer app;

	if (!mime)
		goto done;
	if ((app = g_hash_table_lookup(MIME_APPLICATIONS, mime)))
		apps = g_list_append(apps, app);
	for (list = g_hash_table_lookup(MIME_SUBCLASSES, mime); list; list = list->next)
		if ((app = g_hash_table_lookup(MIME_APPLICATIONS, list->data)))
			subs = g_list_append(subs, app);
	/* TODO: sort subs alphabetically by name */
      done:
	if (apps_p)
		*apps_p = apps;
	if (subs_p)
		*subs_p = subs;
	return;
}

/** @brief gio version of get_apps_and_subs().
  */
void
get_apps_and_subs_gio(const char *mime, GList **apps_p, GList **subs_p)
{
	GList *apps = NULL, *subs = NULL;
	char *type;

	if (!(type = g_content_type_from_mime_type(mime)))
		goto done;
	apps = g_app_info_get_recommended_for_type(type);
	subs = g_app_info_get_fallback_for_type(type);
	g_free(type);
      done:
	if (apps_p)
		*apps_p = apps;
	if (subs_p)
		*subs_p = subs;
	return;
}

void
get_apps_and_subs(const char *mime, GList **apps_p, GList **subs_p)
{
	GList *apps_g = NULL, *subs_g = NULL;
	GList *apps_v = NULL, *subs_v = NULL;

	get_apps_and_subs_gio(mime, &apps_g, &subs_g);
	if (!apps_g || !subs_g)
		get_apps_and_subs_vfs(mime, &apps_v, &subs_v);
	if (apps_p)
		*apps_p = g_list_concat(apps_g, apps_v);
	if (subs_p)
		*subs_p = g_list_concat(subs_g, subs_v);
	return;
}

static void
get_keys(gpointer key, gpointer value, gpointer user_data)
{
	GList **list_p = user_data;

	(void) value;
	*list_p = g_list_append(*list_p, strdup(key));
}

static gint
string_compare(gconstpointer a, gconstpointer b)
{
	return g_strcmp0(a, b);
}

/** @brief get list of used desktops
  *
  * Returns a list of the desktops that appeared in the "OnlyShowIn" and
  * "NotShowIn" key fields of XDG desktop application files.  This will be
  * used to present the user a choice when adding custom applications.
  */
GList *
get_desktops(void)
{
	GList *desktops = NULL;

	g_hash_table_foreach(XDG_DESKTOPS, &get_keys, &desktops);
	desktops = g_list_sort(desktops, &string_compare);
	return (desktops);
}

/** @brief get list of used categories
  *
  * Returns a list of the categories that appeared in the "Categories" key
  * fields of XDG desktop applications files.  This will be used to present
  * the user a choice when adding custom applications.
  */
GList *
get_categories(void)
{
	GList *categories = NULL;

	g_hash_table_foreach(XDG_CATEGORIES, &get_keys, &categories);
	categories = g_list_sort(categories, &string_compare);
	return (categories);
}

static void
list_free(gpointer data)
{
	g_list_free_full(data, g_free);
}

static void
_get_dir_apps(const char *dir, const char *path, const char *pref, GHashTable *appids)
{
	DIR *dh;
	char *dirname;

	dirname = g_strdup_printf("%s/%s", dir, path);
	if ((dh = opendir(dirname))) {
		struct dirent *d;

		while ((d = readdir(dh))) {
			char *name, *apid;

			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
			/* TODO: provide option for hidden files. */
			if (*d->d_name == '.')
				continue;
			name =
			    path ? g_strdup_printf("%s/%s", path, d->d_name) : g_strdup(d->d_name);
			apid =
			    pref ? g_strdup_printf("%s-%s", pref, d->d_name) : g_strdup(d->d_name);
			if (d->d_type == DT_DIR) {
				_get_dir_apps(dir, name, apid, appids);
			} else if ((d->d_type == DT_LNK || d->d_type == DT_REG)
				   && strstr(d->d_name, ".desktop")) {
				g_hash_table_replace(appids, apid, name);
				continue;
			}
			g_free(apid);
			g_free(name);
		}
		closedir(dh);
	}
	g_free(dirname);
}

static void
app_free(gpointer user_data)
{
	g_object_unref(user_data);
}

GHashTable *
get_applications(void)
{
	const gchar *const *sysdirs;
	const gchar *usrdir;
	GList *dirs = NULL, *dir;
	GHashTable *appids, *apps;
	GHashTableIter iter;
	gpointer key, val;

	usrdir = g_get_user_data_dir();
	dirs = g_list_append(dirs, g_strdup_printf("%s/applications", usrdir));
	for (sysdirs = g_get_system_data_dirs(); sysdirs && *sysdirs; sysdirs++)
		dirs = g_list_prepend(dirs, g_strdup_printf("%s/applications", *sysdirs));
	appids = g_hash_table_new_full(&g_str_hash, &g_str_equal, &g_free, &g_free);
	for (dir = dirs; dir; dir = dir->next)
		_get_dir_apps(dir->data, NULL, NULL, appids);
	apps = g_hash_table_new_full(&g_str_hash, &g_str_equal, &g_free, &app_free);
	g_hash_table_iter_init(&iter, appids);
	while (g_hash_table_iter_next(&iter, &key, &val)) {
		GDesktopAppInfo *app;

		if ((app = g_desktop_app_info_new_from_filename(val)))
			g_hash_table_replace(apps, strdup(key), app);
	}
	g_hash_table_destroy(appids);
	return (apps);
}

/** @brief get hash of default applications, added and removed associations.
  *
  * Merge all the "mimeapps.list" files into a single hash elmeent with
  * "Default Applications", "Added Associations" and "Removed Associations"
  * tags.
  */
void
get_mimeapps(GHashTable **default_applications_p, GHashTable **added_associations_p,
	     GHashTable **removed_associations_p)
{
	GHashTable *default_applications;
	GHashTable *added_associations;
	GHashTable *removed_associations;
	const gchar *const *sysdirs;
	const gchar *usrdir;
	GList *dirs = NULL, *dir;
	char *b;

	default_applications = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, list_free);
	added_associations = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, list_free);
	removed_associations = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, list_free);

	usrdir = g_get_user_data_dir();
	dirs = g_list_append(dirs, g_strdup_printf("%s/applications/mimeapps.list", usrdir));
	for (sysdirs = g_get_system_data_dirs(); sysdirs && *sysdirs; sysdirs++)
		dirs = g_list_prepend(dirs,
				      g_strdup_printf("%s/applications/mimeapps.list", *sysdirs));
	b = calloc(BUFSIZ, sizeof(*b));
	for (dir = dirs; dir; dir = dir->next) {
		FILE *file;
		char *p, *q, *section = NULL, *mime, *app;
		GHashTable *table = NULL;

		if (!(file = fopen(dir->data, "r"))) {
			DPRINTF("%s: %s\n", (char *) dir->data, strerror(errno));
			continue;
		}
		while (fgets(b, BUFSIZ, file)) {
			*strchrnul(b, '\n') = '\0';
			if ((p = strchr(b, '[')) && (q = strrchr(b, ']'))) {
				p++;
				*q = '\0';
				if (!strcmp(p, "Default Applications")) {
					section = strdup(p);
					table = default_applications;
				} else if (!strcmp(p, "Added Associations")) {
					section = strdup(p);
					table = added_associations;
				} else if (!strcmp(p, "Removed Associations")) {
					section = strdup(p);
					table = removed_associations;
				} else {
					free(section);
					section = NULL;
					table = NULL;
				}
			} else if (section && (q = strchr(b, '='))) {
				GList *apps = NULL;

				*q++ = '\0';
				mime = strdup(b);
				for (p = q; (app = strtok(p, ";")); p = NULL)
					apps = g_list_append(apps, strdup(app));
				g_hash_table_replace(table, mime, apps);
			}
		}
	}
	free(b);
	if (default_applications_p)
		*default_applications_p = default_applications;
	if (added_associations_p)
		*added_associations_p = added_associations;
	if (removed_associations_p)
		*removed_associations_p = removed_associations;
	return;
}

/** @brief read generic icons
  *
  * Initialization function that reads the XDG shared-mime specification
  * compliant generic icons from the files in %s/mime/generic-icons and places
  * the icons into a global hash MIME_GENERIC_ICONS keyed by mime type.  This
  * hash is later used by get_icons() to find icons for various mime types.
  *
  * This function is idempotent and can be called at any time to update the
  * hash.
  */
static void
read_icons(void)
{
	const gchar *const *sysdirs;
	const gchar *usrdir;
	GList *dirs = NULL, *dir;
	char *b;

	if (!MIME_GENERIC_ICONS)
		MIME_GENERIC_ICONS =
		    g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, g_free);
	usrdir = g_get_user_data_dir();
	dirs = g_list_append(dirs, g_strdup_printf("%s/mime/generic-icons", usrdir));
	for (sysdirs = g_get_system_data_dirs(); sysdirs && *sysdirs; sysdirs++)
		dirs = g_list_prepend(dirs, g_strdup_printf("%s/mime/generic-icons", *sysdirs));
	b = calloc(BUFSIZ, sizeof(*b));
	for (dir = dirs; dir; dir = dir->next) {
		FILE *file;
		char *p;

		if (!(file = fopen(dir->data, "r"))) {
			DPRINTF("%s: %s\n", (char *) dir->data, strerror(errno));
			continue;
		}
		while (fgets(b, BUFSIZ, file)) {
			*strchrnul(b, '\n') = '\0';
			if ((p = strchr(b, ':'))) {
				*p++ = '\0';
				g_hash_table_replace(MIME_GENERIC_ICONS, strdup(b), strdup(p));
			}
		}
		fclose(file);
	}
	free(b);
	g_list_free_full(dirs, &xde_list_free);
}

/** @brief read mime aliases
  *
  * Initialization function that reads the XDG shared-mime specification
  * compilant aliases from the files in %s/mime/aliases and places the aliases
  * into a global hash MIME_ALIASES keyed by mime type.  This is lated used by
  * get_icons() and read_mimeapps() to find icons and applications for various
  * mime types.
  *
  * This method is idempotent and can be called at any time to update the
  * hash.
  */
static void
read_aliases(void)
{
	const gchar *const *sysdirs;
	const gchar *usrdir;
	GList *dirs = NULL, *dir;
	char *b;

	if (!MIME_ALIASES)
		MIME_ALIASES = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, list_free);
	usrdir = g_get_user_data_dir();
	dirs = g_list_append(dirs, g_strdup_printf("%s/mime/aliases", usrdir));
	for (sysdirs = g_get_system_data_dirs(); sysdirs && *sysdirs; sysdirs++)
		dirs = g_list_prepend(dirs, g_strdup_printf("%s/mime/aliases", *sysdirs));
	b = calloc(BUFSIZ, sizeof(*b));
	for (dir = dirs; dir; dir = dir->next) {
		FILE *file;
		char *p, *q, *k;
		GList *list;

		if (!(file = fopen(dir->data, "r"))) {
			DPRINTF("%s: %s\n", (char *) dir->data, strerror(errno));
			continue;
		}
		while (fgets(b, BUFSIZ, file)) {
			*strchrnul(b, '\n') = '\0';
			if ((p = strtok(b, " \t")) && (q = strtok(NULL, " \t"))) {
				list = NULL;
				if (g_hash_table_lookup_extended
				    (MIME_ALIASES, p, (gpointer *) &k, (gpointer *) &list))
					g_hash_table_steal(MIME_ALIASES, k);
				else
					k = strdup(p);
				list = g_list_append(list, strdup(q));
				g_hash_table_replace(MIME_ALIASES, k, list);

				list = NULL;
				if (g_hash_table_lookup_extended
				    (MIME_ALIASES, q, (gpointer *) &k, (gpointer *) &list))
					g_hash_table_steal(MIME_ALIASES, k);
				else
					k = strdup(q);
				list = g_list_append(list, strdup(p));
				g_hash_table_replace(MIME_ALIASES, k, list);
			}
		}
		fclose(file);
	}
	free(b);
	g_list_free_full(dirs, &xde_list_free);
}

/** @brief read mime subclasses
  *
  * Initialization function that reads the XDG share-mime specification
  * compliant subclasses from the files in %s/mime/subclasses and places the
  * subclasses into a global hash MIME_SUBCLASSES keyed by mime type.  This is
  * later used by get_icons() and read_mimeapps() to find icons and
  * applications for various mime types.
  */
static void
read_subclasses(void)
{
	const gchar *const *sysdirs;
	const gchar *usrdir;
	GList *dirs = NULL, *dir;
	char *b;

	if (!MIME_SUBCLASSES)
		MIME_SUBCLASSES =
		    g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, list_free);
	usrdir = g_get_user_data_dir();
	dirs = g_list_append(dirs, g_strdup_printf("%s/mime/subclasses", usrdir));
	for (sysdirs = g_get_system_data_dirs(); sysdirs && *sysdirs; sysdirs++)
		dirs = g_list_prepend(dirs, g_strdup_printf("%s/mime/subclasses", *sysdirs));
	b = calloc(BUFSIZ, sizeof(*b));
	for (dir = dirs; dir; dir = dir->next) {
		FILE *file;
		char *p, *q, *k;

		if (!(file = fopen(dir->data, "r"))) {
			DPRINTF("%s: %s\n", (char *) dir->data, strerror(errno));
			continue;
		}
		while (fgets(b, BUFSIZ, file)) {
			*strchrnul(b, '\n') = '\0';
			if ((p = strtok(b, " \t")) && (q = strtok(NULL, " \t"))) {
				GList *list = NULL;

				if (g_hash_table_lookup_extended
				    (MIME_SUBCLASSES, p, (gpointer *) &k, (gpointer *) &list))
					g_hash_table_steal(MIME_SUBCLASSES, k);
				else
					k = strdup(p);
				list = g_list_append(list, strdup(q));
				g_hash_table_replace(MIME_SUBCLASSES, k, list);
			}
		}
		fclose(file);
	}
	free(b);
	g_list_free_full(dirs, &xde_list_free);
}

/** @brief map gvfs applications to mime types
  *
  * gnome-vfs-2.0 has its own idea of the mapping of mime types to
  * applications outside of the XDG desktop specificaiton.  This method uses
  * the gnome-vfs application registry to retrieve those applications.  This
  * provides a somewhat richer mapping verses using the XDG desktop
  * specification applications files alone.  This method is used by
  * read_mimeapps() to get a fuller set of mime type to application mappings.
  */
GList *
read_gvfsapps(void)
{
	GList *apps = NULL;

	return (apps);
}

/** @brief map applications to mime types
  *
  * Initialization function that uses read_gvfsapps() and get_applications()
  * to get all of the applications known to gnome-vfs-2.0 and those specified
  * according to the XDG desktop specification.  These applicaitons are placed
  * into the global hash MIME_APPLICATIONS keyed by application id.  This is
  * later used by get_apps_and_subs() to retrieve applications and subclass
  * applications associated with a mime type.
  *
  * This method is idempotent and can be called at any time to update the
  * hash.
  */
static void
read_mimeapps_vfs(void)
{
#if 0
	GList *gvfs_list, *apps_list, *list, *item;

	if (!MIME_APPLICATIONS)
		MIME_APPLICATIONS = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, g_free);
	gvfs_list = read_gvfsapps();
	apps_list = get_applications();
	list = g_list_concat(gvfs_list, apps_list);
	for (item = list; item; item = item->next) {
	}
#endif
}

static gboolean
add_application(gpointer key, gpointer value, gpointer user_data)
{
	GDesktopAppInfo *app = user_data;
	GList *list = NULL;
	char *k;

	(void) value;
	if ((g_hash_table_lookup_extended
	     (MIME_APPLICATIONS, key, (gpointer *) &k, (gpointer *) &list)))
		g_hash_table_steal(MIME_APPLICATIONS, k);
	else
		k = strdup(key);
	list = g_list_append(list, app);
	g_hash_table_replace(MIME_APPLICATIONS, k, list);
	return TRUE;
}

/** @brief a gio version of read_mimeapps()
  */
static void
read_mimeapps_gio(void)
{
	GList *list, *apps;

	for (list = apps = g_app_info_get_all(); list; list = list->next) {
		GDesktopAppInfo *app = list->data;
		char *value, *p, *token;
		GHashTable *types;
		GList *alias;

		for (p = value = g_desktop_app_info_get_string(app, "OnlyShowIn");
		     (token = strtok(p, ";")); p = NULL)
			g_hash_table_add(XDG_DESKTOPS, strdup(token));
		g_free(value);
		for (p = value = g_desktop_app_info_get_string(app, "NotShowIn");
		     (token = strtok(p, ";")); p = NULL)
			g_hash_table_add(XDG_DESKTOPS, strdup(token));
		g_free(value);
		for (p = value = g_desktop_app_info_get_string(app, "Categories");
		     (token = strtok(p, ";")); p = NULL)
			g_hash_table_add(XDG_CATEGORIES, strdup(token));
		g_free(value);
		types = g_hash_table_new_full(&g_str_hash, &g_str_equal, g_free, NULL);
		for (p = value = g_desktop_app_info_get_string(app, "MimeType");
		     (token = strtok(p, ";")); p = NULL) {
			g_hash_table_add(types, strdup(token));
			for (alias = g_hash_table_lookup(MIME_ALIASES, token); alias;
			     alias = alias->next)
				g_hash_table_add(types, strdup(alias->data));
		}
		g_free(value);
		g_hash_table_foreach_remove(types, &add_application, app);
		g_hash_table_destroy(types);
	}
	g_list_free(apps);
}

static void
read_mimeapps(void)
{
	read_mimeapps_vfs();
	read_mimeapps_gio();
}

static void
read_primary_data(void)
{
	read_icons();
	read_aliases();
	read_subclasses();
	read_mimeapps();
}

static void
free_desktop(gpointer data)
{
	XdeDesktop *desk = data;

	if (desk->pixmap) {
		/* need to dereference this pixmap */
	}
	free(desk);
}

static void
do_run(int argc, char *argv[], Bool replace)
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *root = gdk_screen_get_root_window(scrn), *sel;
	char selection[64] = { 0, };
	Window selwin, owner;
	XdeScreen *xscr;
	int s, nscr;

	(void) argc;
	(void) argv;
	DPRINT();
	selwin = XCreateSimpleWindow(dpy, GDK_WINDOW_XID(root), 0, 0, 1, 1, 0, 0, 0);

	if ((owner = get_selection(replace, selwin))) {
		if (!replace) {
			XDestroyWindow(dpy, selwin);
			EPRINTF("%s: instance already running\n", NAME);
			exit(EXIT_FAILURE);
		}
	}
	XSelectInput(dpy, selwin,
		     StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);

	nscr = gdk_display_get_n_screens(disp);
	screens = calloc(nscr, sizeof(*screens));

	sel = gdk_x11_window_foreign_new_for_display(disp, selwin);
	gdk_window_add_filter(sel, selwin_handler, screens);

	read_primary_data();

	for (s = 0, xscr = screens; s < nscr; s++, xscr++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		xscr->index = s;
		xscr->atom = XInternAtom(dpy, selection, False);
		xscr->disp = disp;
		xscr->scrn = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(xscr->scrn);
		xscr->selwin = selwin;
		xscr->width = gdk_screen_get_width(xscr->scrn);
		xscr->height = gdk_screen_get_height(xscr->scrn);
		xscr->desktops = g_ptr_array_new_full(64, &free_desktop);
		xscr->pixmaps = XUniqueContext();
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_wnck(xscr);
		init_monitors(xscr);
		init_window(xscr);
		if (options.proxy)
			setup_button_proxy(xscr);
		update_root_pixmap(xscr, None);
		update_layout(xscr, None);
		update_current_desktop(xscr, None);
		update_theme(xscr, None);
		update_desktop(xscr->desk);
	}
	gtk_main();
}

/** @brief Ask a running instance to quit.
  *
  * This is performed by checking for an owner of the selection and clearing the
  * selection if it exists.
  */
static void
do_quit(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	DPRINT();
	get_selection(True, None);
}

static void
update_theme(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;
	Bool changed = False;
	GtkSettings *set;

	(void) prop;
	DPRINT();
	gtk_rc_reparse_all();
	if (XGetTextProperty(dpy, root, &xtp, _XA_XDE_THEME_NAME)) {
		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix) + 1;
				rc_string = calloc(len, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
				if (!xscr->theme || strcmp(xscr->theme, list[0])) {
					free(xscr->theme);
					xscr->theme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for property\n");
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF("could not get _XDE_THEME_NAME for root 0x%lx\n", root);
	if ((set = gtk_settings_get_for_screen(xscr->scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *theme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-theme-name", &theme_v);
		theme = g_value_get_string(&theme_v);
		if (theme && (!xscr->theme || strcmp(xscr->theme, theme))) {
			free(xscr->theme);
			xscr->theme = strdup(theme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF("New theme is %s\n", xscr->theme);
		/* FIXME: do somthing more about it. */
	}
}

static void
refresh_desktop(XdeScreen *xscr)
{
	(void) xscr;
}

static void
refresh_layout(XdeScreen *xscr)
{
	refresh_desktop(xscr);
}

static void
update_layout(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0, num, desks = xscr->desks;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Bool propok = False, layout_changed = False, number_changed = False;

	DPRINT();
	if (prop == None || prop == _XA_NET_DESKTOP_LAYOUT) {
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_DESKTOP_LAYOUT, 0, 4, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 4 && data) {
			if (xscr->cols != (int) data[1] || xscr->rows != (int) data[2]) {
				xscr->cols = data[1];
				xscr->rows = data[2];
				layout_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (prop == None || prop == _XA_WIN_WORKSPACE_COUNT) {
		if (XGetWindowProperty
		    (dpy, root, _XA_WIN_WORKSPACE_COUNT, 0, 1, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			if (xscr->desks != (int) data[0]) {
				xscr->desks = data[0];
				number_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (prop == None || prop == _XA_NET_NUMBER_OF_DESKTOPS) {
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_NUMBER_OF_DESKTOPS, 0, 1, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			if (xscr->desks != (int) data[0]) {
				xscr->desks = data[0];
				number_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (!propok)
		EPRINTF("wrong property passed\n");

	if (number_changed) {
		if (xscr->desks <= 0)
			xscr->desks = 1;
		if (xscr->desks > 64)
			xscr->desks = 64;
		if (xscr->desks == desks)
			number_changed = False;
		else {
			g_ptr_array_set_size(xscr->desktops, xscr->desks);
			for (num = desks; num < xscr->desks; num++) {
				XdeDesktop *desk;

				desk = calloc(1, sizeof(*desk));
				desk->xscr = xscr;
				desk->index = num;
				g_ptr_array_insert(xscr->desktops, num, desk);
			}
		}
	}
	if (layout_changed || number_changed) {
		if (xscr->rows <= 0 && xscr->cols <= 0) {
			xscr->rows = 1;
			xscr->cols = 0;
		}
		if (xscr->cols > xscr->desks) {
			xscr->cols = xscr->desks;
			xscr->rows = 1;
		}
		if (xscr->rows > xscr->desks) {
			xscr->rows = xscr->desks;
			xscr->cols = 1;
		}
		if (xscr->cols == 0)
			for (num = xscr->desks; num > 0; xscr->cols++, num -= xscr->rows) ;
		if (xscr->rows == 0)
			for (num = xscr->desks; num > 0; xscr->rows++, num -= xscr->cols) ;

		add_deferred_refresh_layout(xscr);
	}
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 2) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	    if (xev->xproperty.atom == _XA_XDE_THEME_NAME
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_theme(xscr, xev->xproperty.atom);
		return GDK_FILTER_REMOVE;	/* event handled */
	} else
	    if (xev->xproperty.atom == _XA_NET_DESKTOP_LAYOUT
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_CURRENT_DESKTOP
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_WORKSPACE_COUNT
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_WORKSPACE
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WM_DESKTOP
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_XROOTPMAP_ID
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_root_pixmap(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_ESETROOT_PMAP_ID
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_root_pixmap(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_WORKAREA
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_workarea(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_WORKAREA
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_workarea(xscr, xev->xproperty.atom);
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);

	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s))
			xscr = screens + s;

	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> ClientMessage: %p\n", xscr);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xclient.window);
		fprintf(stderr, "    --> message_type = %s\n",
			XGetAtomName(dpy, xev->xclient.message_type));
		fprintf(stderr, "    --> format = %d\n", xev->xclient.format);
		switch (xev->xclient.format) {
			int i;

		case 8:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 20; i++)
				fprintf(stderr, " %02x", xev->xclient.data.b[i]);
			fprintf(stderr, "\n");
			break;
		case 16:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 10; i++)
				fprintf(stderr, " %04x", xev->xclient.data.s[i]);
			fprintf(stderr, "\n");
			break;
		case 32:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 5; i++)
				fprintf(stderr, " %08lx", xev->xclient.data.l[i]);
			fprintf(stderr, "\n");
			break;
		}
		fprintf(stderr, "<== ClientMessage: %p\n", xscr);
	}
	if (xscr && xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, xev->xclient.message_type);
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_SelectionClear(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> SelectionClear: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xselectionclear.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xselectionclear.window);
		fprintf(stderr, "    --> selection = %s\n",
			XGetAtomName(dpy, xev->xselectionclear.selection));
		fprintf(stderr, "    --> time = %lu\n", xev->xselectionclear.time);
		fprintf(stderr, "<== SelectionClear: %p\n", xscr);
	}
	if (xscr && xev->xselectionclear.window == xscr->selwin) {
		XDestroyWindow(dpy, xscr->selwin);
		EPRINTF("selection cleared, exiting\n");
		exit(EXIT_SUCCESS);
	}
	if (xscr && xev->xselectionclear.window == xscr->laywin){
		XDestroyWindow(dpy, xscr->laywin);
		xscr->laywin = None;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	(void) event;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case PropertyNotify:
		return event_handler_PropertyNotify(dpy, xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	(void) event;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case SelectionClear:
		return event_handler_SelectionClear(dpy, xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}

#if 0
static GdkFilterReturn
laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case SelectionClear:
		return event_handler_SelectionClear(dpy, xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}
#endif

static GdkFilterReturn
client_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	Display *dpy = (typeof(dpy)) data;

	(void) event;
	DPRINT();
	switch (xev->type) {
	case ClientMessage:
		return event_handler_ClientMessage(dpy, xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

#if 0
static void
set_current_desktop(XdeScreen *xscr, int index, Time timestamp)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	XEvent ev;

	DPRINT();
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = False;
	ev.xclient.display = dpy;
	ev.xclient.window = root;
	ev.xclient.message_type = _XA_NET_CURRENT_DESKTOP;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = index;
	ev.xclient.data.l[1] = timestamp;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}
#endif

static GdkFilterReturn
proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	int num;

	(void) event;
	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case ButtonPress:
		if (options.debug) {
			fprintf(stderr, "==> ButtonPress: %p\n", xscr);
			fprintf(stderr, "    --> send_event = %s\n",
				xev->xbutton.send_event ? "true" : "false");
			fprintf(stderr, "    --> window = 0x%lx\n", xev->xbutton.window);
			fprintf(stderr, "    --> root = 0x%lx\n", xev->xbutton.root);
			fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xbutton.subwindow);
			fprintf(stderr, "    --> time = %lu\n", xev->xbutton.time);
			fprintf(stderr, "    --> x = %d\n", xev->xbutton.x);
			fprintf(stderr, "    --> y = %d\n", xev->xbutton.y);
			fprintf(stderr, "    --> x_root = %d\n", xev->xbutton.x_root);
			fprintf(stderr, "    --> y_root = %d\n", xev->xbutton.y_root);
			fprintf(stderr, "    --> state = 0x%08x\n", xev->xbutton.state);
			fprintf(stderr, "    --> button = %u\n", xev->xbutton.button);
			fprintf(stderr, "    --> same_screen = %s\n",
				xev->xbutton.same_screen ? "true" : "false");
			fprintf(stderr, "<== ButtonPress: %p\n", xscr);
		}
		switch (xev->xbutton.button) {
		case 4:
			update_current_desktop(xscr, None);
			num = (xscr->current - 1 + xscr->desks) % xscr->desks;
#if 0
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
#else
			(void) num;
			break;
#endif
		case 5:
			update_current_desktop(xscr, None);
			num = (xscr->current + 1 + xscr->desks) % xscr->desks;
#if 0
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
#else
			(void) num;
			break;
#endif
		}
		return GDK_FILTER_CONTINUE;
	case ButtonRelease:
		if (options.debug > 1) {
			fprintf(stderr, "==> ButtonRelease: %p\n", xscr);
			fprintf(stderr, "    --> send_event = %s\n",
				xev->xbutton.send_event ? "true" : "false");
			fprintf(stderr, "    --> window = 0x%lx\n", xev->xbutton.window);
			fprintf(stderr, "    --> root = 0x%lx\n", xev->xbutton.root);
			fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xbutton.subwindow);
			fprintf(stderr, "    --> time = %lu\n", xev->xbutton.time);
			fprintf(stderr, "    --> x = %d\n", xev->xbutton.x);
			fprintf(stderr, "    --> y = %d\n", xev->xbutton.y);
			fprintf(stderr, "    --> x_root = %d\n", xev->xbutton.x_root);
			fprintf(stderr, "    --> y_root = %d\n", xev->xbutton.y_root);
			fprintf(stderr, "    --> state = 0x%08x\n", xev->xbutton.state);
			fprintf(stderr, "    --> button = %u\n", xev->xbutton.button);
			fprintf(stderr, "    --> same_screen = %s\n",
				xev->xbutton.same_screen ? "true" : "false");
			fprintf(stderr, "<== ButtonRelease: %p\n", xscr);
		}
		return GDK_FILTER_CONTINUE;
	case PropertyNotify:
		if (options.debug > 2) {
			fprintf(stderr, "==> PropertyNotify:\n");
			fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
			fprintf(stderr, "    --> atom = %s\n",
				XGetAtomName(dpy, xev->xproperty.atom));
			fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
			fprintf(stderr, "    --> state = %s\n",
				(xev->xproperty.state ==
				 PropertyNewValue) ? "NewValue" : "Delete");
			fprintf(stderr, "<== PropertyNotify:\n");
		}
		return GDK_FILTER_CONTINUE;
	}
	EPRINTF("wrong message type for handler %d on window 0x%08lx\n", xev->type,
		xev->xany.window);
	return GDK_FILTER_CONTINUE;
}

static void
clientSetProperties(SmcConn smcConn, SmPointer data)
{
	char userID[20];
	int i, j, argc = saveArgc;
	char **argv = saveArgv;
	char *cwd = NULL;
	char hint;
	struct passwd *pw;
	SmPropValue *penv = NULL, *prst = NULL, *pcln = NULL;
	SmPropValue propval[11];
	SmProp prop[11];

	SmProp *props[11] = {
		&prop[0], &prop[1], &prop[2], &prop[3], &prop[4],
		&prop[5], &prop[6], &prop[7], &prop[8], &prop[9],
		&prop[10]
	};

	(void) data;
	j = 0;

	/* CloneCommand: This is like the RestartCommand except it restarts a copy of the 
	   application.  The only difference is that the application doesn't supply its
	   client id at register time.  On POSIX systems the type should be a
	   LISTofARRAY8. */
	prop[j].name = SmCloneCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = pcln = calloc(argc, sizeof(*pcln));
	prop[j].num_vals = 0;
	props[j] = &prop[j];
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-clientId") || !strcmp(argv[i], "-restore"))
			i++;
		else {
			prop[j].vals[prop[j].num_vals].value = (SmPointer) argv[i];
			prop[j].vals[prop[j].num_vals++].length = strlen(argv[i]);
		}
	}
	j++;

#if 0
	/* CurrentDirectory: On POSIX-based systems, specifies the value of the current
	   directory that needs to be set up prior to starting the program and should be
	   of type ARRAY8. */
	prop[j].name = SmCurrentDirectory;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = NULL;
	propval[j].length = 0;
	cwd = calloc(PATH_MAX + 1, sizeof(propval[j].value[0]));
	if (getcwd(cwd, PATH_MAX)) {
		propval[j].value = cwd;
		propval[j].length = strlen(propval[j].value);
		j++;
	} else {
		free(cwd);
		cwd = NULL;
	}
#endif

#if 0
	/* DiscardCommand: The discard command contains a command that when delivered to
	   the host that the client is running on (determined from the connection), will
	   cause it to discard any information about the current state.  If this command
	   is not specified, the SM will assume that all of the client's state is encoded
	   in the RestartCommand [and properties].  On POSIX systems the type should be
	   LISTofARRAY8. */
	prop[j].name = SmDiscardCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = "/bin/true";
	propval[j].length = strlen("/bin/true");
	j++;
#endif

#if 0
	char **env;

	/* Environment: On POSIX based systems, this will be of type LISTofARRAY8 where
	   the ARRAY8s alternate between environment variable name and environment
	   variable value. */
	/* XXX: we might want to filter a few out */
	for (i = 0, env = environ; *env; i += 2, env++) ;
	prop[j].name = SmEnvironment;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = penv = calloc(i, sizeof(*penv));
	prop[j].num_vals = i;
	props[j] = &prop[j];
	for (i = 0, env = environ; *env; i += 2, env++) {
		char *equal;
		int len;

		equal = strchrnul(*env, '=');
		len = (int) (*env - equal);
		if (*equal)
			equal++;
		prop[j].vals[i].value = *env;
		prop[j].vals[i].length = len;
		prop[j].vals[i + 1].value = equal;
		prop[j].vals[i + 1].length = strlen(equal);
	}
	j++;
#endif

#if 0
	char procID[20];

	/* ProcessID: This specifies an OS-specific identifier for the process. On POSIX
	   systems this should be of type ARRAY8 and contain the return of getpid()
	   turned into a Latin-1 (decimal) string. */
	prop[j].name = SmProcessID;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	snprintf(procID, sizeof(procID), "%ld", (long) getpid());
	propval[j].value = procID;
	propval[j].length = strlen(procID);
	j++;
#endif

	/* Program: The name of the program that is running.  On POSIX systems, this
	   should eb the first parameter passed to execve(3) and should be of type
	   ARRAY8. */
	prop[j].name = SmProgram;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = argv[0];
	propval[j].length = strlen(argv[0]);
	j++;

	/* RestartCommand: The restart command contains a command that when delivered to
	   the host that the client is running on (determined from the connection), will
	   cause the client to restart in its current state.  On POSIX-based systems this 
	   if of type LISTofARRAY8 and each of the elements in the array represents an
	   element in the argv[] array.  This restart command should ensure that the
	   client restarts with the specified client-ID.  */
	prop[j].name = SmRestartCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = prst = calloc(argc + 4, sizeof(*prst));
	prop[j].num_vals = 0;
	props[j] = &prop[j];
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-clientId") || !strcmp(argv[i], "-restore"))
			i++;
		else {
			prop[j].vals[prop[j].num_vals].value = (SmPointer) argv[i];
			prop[j].vals[prop[j].num_vals++].length = strlen(argv[i]);
		}
	}
	prop[j].vals[prop[j].num_vals].value = (SmPointer) "-clientId";
	prop[j].vals[prop[j].num_vals++].length = 9;
	prop[j].vals[prop[j].num_vals].value = (SmPointer) options.clientId;
	prop[j].vals[prop[j].num_vals++].length = strlen(options.clientId);

	prop[j].vals[prop[j].num_vals].value = (SmPointer) "-restore";
	prop[j].vals[prop[j].num_vals++].length = 9;
	prop[j].vals[prop[j].num_vals].value = (SmPointer) options.saveFile;
	prop[j].vals[prop[j].num_vals++].length = strlen(options.saveFile);
	j++;

	/* ResignCommand: A client that sets the RestartStyleHint to RestartAnyway uses
	   this property to specify a command that undoes the effect of the client and
	   removes any saved state. */
	prop[j].name = SmResignCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = calloc(2, sizeof(*prop[j].vals));
	prop[j].num_vals = 2;
	props[j] = &prop[j];
	prop[j].vals[0].value = "/usr/bin/xde-pager";
	prop[j].vals[0].length = strlen("/usr/bin/xde-pager");
	prop[j].vals[1].value = "-quit";
	prop[j].vals[1].length = strlen("-quit");
	j++;

	/* RestartStyleHint: If the RestartStyleHint property is present, it will contain 
	   the style of restarting the client prefers.  If this flag is not specified,
	   RestartIfRunning is assumed.  The possible values are as follows:
	   RestartIfRunning(0), RestartAnyway(1), RestartImmediately(2), RestartNever(3). 
	   The RestartIfRunning(0) style is used in the usual case.  The client should be 
	   restarted in the next session if it is connected to the session manager at the
	   end of the current session. The RestartAnyway(1) style is used to tell the SM
	   that the application should be restarted in the next session even if it exits
	   before the current session is terminated. It should be noted that this is only
	   a hint and the SM will follow the policies specified by its users in
	   determining what applications to restart.  A client that uses RestartAnyway(1)
	   should also set the ResignCommand and ShutdownCommand properties to the
	   commands that undo the state of the client after it exits.  The
	   RestartImmediately(2) style is like RestartAnyway(1) but in addition, the
	   client is meant to run continuously.  If the client exits, the SM should try to 
	   restart it in the current session.  The RestartNever(3) style specifies that
	   the client does not wish to be restarted in the next session. */
	prop[j].name = SmRestartStyleHint;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[0];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	hint = SmRestartImmediately;	/* <--- */
	propval[j].value = &hint;
	propval[j].length = 1;
	j++;

	/* ShutdownCommand: This command is executed at shutdown time to clean up after a 
	   client that is no longer running but retained its state by setting
	   RestartStyleHint to RestartAnyway(1).  The command must not remove any saved
	   state as the client is still part of the session. */
	prop[j].name = SmShutdownCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = calloc(2, sizeof(*prop[j].vals));
	prop[j].num_vals = 2;
	props[j] = &prop[j];
	prop[j].vals[0].value = "/usr/bin/xde-pager";
	prop[j].vals[0].length = strlen("/usr/bin/xde-pager");
	prop[j].vals[1].value = "-quit";
	prop[j].vals[1].length = strlen("-quit");
	j++;

	/* UserID: Specifies the user's ID.  On POSIX-based systems this will contain the 
	   user's name (the pw_name field of struct passwd).  */
	errno = 0;
	prop[j].name = SmUserID;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	if ((pw = getpwuid(getuid())))
		strncpy(userID, pw->pw_name, sizeof(userID) - 1);
	else {
		EPRINTF("%s: %s\n", "getpwuid()", strerror(errno));
		snprintf(userID, sizeof(userID), "%ld", (long) getuid());
	}
	propval[j].value = userID;
	propval[j].length = strlen(userID);
	j++;

	SmcSetProperties(smcConn, j, props);

	free(cwd);
	free(pcln);
	free(prst);
	free(penv);
}

static Bool saving_yourself;
static Bool shutting_down;

static void
clientSaveYourselfPhase2CB(SmcConn smcConn, SmPointer data)
{
	clientSetProperties(smcConn, data);
	SmcSaveYourselfDone(smcConn, True);
}

/** @brief save yourself
  *
  * The session manager sends a "Save Yourself" message to a client either to
  * check-point it or just before termination so that it can save its state.
  * The client responds with zero or more calls to SmcSetProperties to update
  * the properties indicating how to restart the client.  When all the
  * properties have been set, the client calls SmcSaveYourselfDone.
  *
  * If interact_type is SmcInteractStyleNone, the client must not interact with
  * the user while saving state.  If interact_style is SmInteractStyleErrors,
  * the client may interact with the user only if an error condition arises.  If
  * interact_style is  SmInteractStyleAny then the client may interact with the
  * user for any purpose.  Because only one client can interact with the user at
  * a time, the client must call SmcInteractRequest and wait for an "Interact"
  * message from the session maanger.  When the client is done interacting with
  * the user, it calls SmcInteractDone.  The client may only call
  * SmcInteractRequest() after it receives a "Save Yourself" message and before
  * it calls SmcSaveYourSelfDone().
  */
static void
clientSaveYourselfCB(SmcConn smcConn, SmPointer data, int saveType, Bool shutdown,
		     int interactStyle, Bool fast)
{
	(void) saveType;
	(void) interactStyle;
	(void) fast;
	if (!(shutting_down = shutdown)) {
		if (!SmcRequestSaveYourselfPhase2(smcConn, clientSaveYourselfPhase2CB, data))
			SmcSaveYourselfDone(smcConn, False);
		return;
	}
	clientSetProperties(smcConn, data);
	SmcSaveYourselfDone(smcConn, True);
}

/** @brief die
  *
  * The session manager sends a "Die" message to a client when it wants it to
  * die.  The client should respond by calling SmcCloseConnection.  A session
  * manager that behaves properly will send a "Save Yourself" message before the
  * "Die" message.
  */
static void
clientDieCB(SmcConn smcConn, SmPointer data)
{
	(void) data;
	SmcCloseConnection(smcConn, 0, NULL);
	shutting_down = False;
	gtk_main_quit();
}

static void
clientSaveCompleteCB(SmcConn smcConn, SmPointer data)
{
	(void) smcConn;
	(void) data;
	if (saving_yourself) {
		saving_yourself = False;
		gtk_main_quit();
	}

}

/** @brief shutdown cancelled
  *
  * The session manager sends a "Shutdown Cancelled" message when the user
  * cancelled the shutdown during an interaction (see Section 5.5, "Interacting
  * With the User").  The client can now continue as if the shutdown had never
  * happended.  If the client has not called SmcSaveYourselfDone() yet, it can
  * either abort the save and then send SmcSaveYourselfDone() with the success
  * argument set to False or it can continue with the save and then call
  * SmcSaveYourselfDone() with the success argument set to reflect the outcome
  * of the save.
  */
static void
clientShutdownCancelledCB(SmcConn smcConn, SmPointer data)
{
	(void) smcConn;
	(void) data;
	shutting_down = False;
	gtk_main_quit();
}

/* *INDENT-OFF* */
static unsigned long clientCBMask =
	SmcSaveYourselfProcMask |
	SmcDieProcMask |
	SmcSaveCompleteProcMask |
	SmcShutdownCancelledProcMask;

static SmcCallbacks clientCBs = {
	.save_yourself = {
		.callback = &clientSaveYourselfCB,
		.client_data = NULL,
	},
	.die = {
		.callback = &clientDieCB,
		.client_data = NULL,
	},
	.save_complete = {
		.callback = &clientSaveCompleteCB,
		.client_data = NULL,
	},
	.shutdown_cancelled = {
		.callback = &clientShutdownCancelledCB,
		.client_data = NULL,
	},
};
/* *INDENT-ON* */

static gboolean
on_ifd_watch(GIOChannel *chan, GIOCondition cond, pointer data)
{
	SmcConn smcConn = data;
	IceConn iceConn = SmcGetIceConnection(smcConn);

	(void) chan;
	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		EPRINTF("poll failed: %s %s %s\n",
			(cond & G_IO_NVAL) ? "NVAL" : "",
			(cond & G_IO_HUP) ? "HUP" : "", (cond & G_IO_ERR) ? "ERR" : "");
		return G_SOURCE_REMOVE;	/* remove event source */
	} else if (cond & (G_IO_IN | G_IO_PRI)) {
		IceProcessMessages(iceConn, NULL, NULL);
	}
	return G_SOURCE_CONTINUE;	/* keep event source */
}

static void
init_smclient(void)
{
	char err[256] = { 0, };
	GIOChannel *chan;
	int ifd, mask = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_PRI;
	char *env;
	SmcConn smcConn;
	IceConn iceConn;

	if (!(env = getenv("SESSION_MANAGER"))) {
		if (options.clientId)
			EPRINTF("clientId provided but no SESSION_MANAGER\n");
		return;
	}
	smcConn = SmcOpenConnection(env, NULL, SmProtoMajor, SmProtoMinor,
				    clientCBMask, &clientCBs, options.clientId,
				    &options.clientId, sizeof(err), err);
	if (!smcConn) {
		EPRINTF("SmcOpenConnection: %s\n", err);
		return;
	}
	iceConn = SmcGetIceConnection(smcConn);
	ifd = IceConnectionNumber(iceConn);
	chan = g_io_channel_unix_new(ifd);
	g_io_add_watch(chan, mask, on_ifd_watch, smcConn);
}

static void
startup(int argc, char *argv[])
{
	GdkAtom atom;
	GdkEventMask mask;
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	Display *dpy;
	char *file;
	int nscr;

	file = g_strdup_printf("%s/.gtkrc-2.0.xde", g_get_home_dir());
	gtk_rc_add_default_file(file);
	g_free(file);

	init_smclient();

	gtk_init(&argc, &argv);

	gnome_vfs_init();

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		EPRINTF("bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_LAYOUT");
	_XA_NET_DESKTOP_LAYOUT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_NAMES");
	_XA_NET_DESKTOP_NAMES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_NUMBER_OF_DESKTOPS");
	_XA_NET_NUMBER_OF_DESKTOPS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_CURRENT_DESKTOP");
	_XA_NET_CURRENT_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_DESKTOP_BUTTON_PROXY");
	_XA_WIN_DESKTOP_BUTTON_PROXY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE_COUNT");
	_XA_WIN_WORKSPACE_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE");
	_XA_WIN_WORKSPACE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("WM_DESKTOP");
	_XA_WM_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XROOTPMAP_ID");
	_XA_XROOTPMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("ESETROOT_PMAP_ID");
	_XA_ESETROOT_PMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_WORKAREA");
	_XA_NET_WORKAREA = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKAREA");
	_XA_WIN_WORKAREA = gdk_x11_atom_to_xatom_for_display(disp, atom);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);
}

static void
copying(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
--------------------------------------------------------------------------------\n\
%1$s\n\
--------------------------------------------------------------------------------\n\
Copyright (c) 2008-2020  Monavacon Limited <http://www.monavacon.com/>\n\
Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>\n\
Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>\n\
\n\
All Rights Reserved.\n\
--------------------------------------------------------------------------------\n\
This program is free software: you can  redistribute it  and/or modify  it under\n\
the terms of the  GNU Affero  General  Public  License  as published by the Free\n\
Software Foundation, version 3 of the license.\n\
\n\
This program is distributed in the hope that it will  be useful, but WITHOUT ANY\n\
WARRANTY; without even  the implied warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.\n\
\n\
You should have received a copy of the  GNU Affero General Public License  along\n\
with this program.   If not, see <http://www.gnu.org/licenses/>, or write to the\n\
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\
--------------------------------------------------------------------------------\n\
U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on behalf\n\
of the U.S. Government (\"Government\"), the following provisions apply to you. If\n\
the Software is supplied by the Department of Defense (\"DoD\"), it is classified\n\
as \"Commercial  Computer  Software\"  under  paragraph  252.227-7014  of the  DoD\n\
Supplement  to the  Federal Acquisition Regulations  (\"DFARS\") (or any successor\n\
regulations) and the  Government  is acquiring  only the  license rights granted\n\
herein (the license rights customarily provided to non-Government users). If the\n\
Software is supplied to any unit or agency of the Government  other than DoD, it\n\
is  classified as  \"Restricted Computer Software\" and the Government's rights in\n\
the Software  are defined  in  paragraph 52.227-19  of the  Federal  Acquisition\n\
Regulations (\"FAR\")  (or any successor regulations) or, in the cases of NASA, in\n\
paragraph  18.52.227-86 of  the  NASA  Supplement  to the FAR (or any  successor\n\
regulations).\n\
--------------------------------------------------------------------------------\n\
", NAME " " VERSION);
}

static void
version(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
%1$s (OpenSS7 %2$s) %3$s\n\
Written by Brian Bidulock.\n\
\n\
Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2020  Monavacon Limited.\n\
Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008  OpenSS7 Corporation.\n\
Copyright (c) 1997, 1998, 1999, 2000, 2001  Brian F. G. Bidulock.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
\n\
Distributed by OpenSS7 under GNU Affero General Public License Version 3,\n\
with conditions, incorporated herein by reference.\n\
\n\
See `%1$s --copying' for copying permissions.\n\
", NAME, PACKAGE, VERSION);
}

static void
usage(int argc, char *argv[])
{
	(void) argc;
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stderr, "\
Usage:\n\
    %1$s [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

static const char *
show_bool(Bool value)
{
	if (value)
		return ("true");
	return ("false");
}

static void
help(int argc, char *argv[])
{
	(void) argc;
	if (!options.output && !options.debug)
		return;
	/* *INDENT-OFF* */
	(void) fprintf(stdout, "\
Usage:\n\
    %1$s [options]\n\
    %1$s {-r|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
    -r, --replace\n\
        replace a running instance\n\
    -q, --quit\n\
        ask a running instance to quit\n\
    -h, --help, -?, --?\n\
        print this usage information and exit\n\
    -V, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
Options:\n\
    -d, --display DISPLAY\n\
        specify the X display, DISPLAY, to use [default: %4$s]\n\
    -s, --screen SCREEN\n\
        specify the screen number, SCREEN, to use [default: %5$d]\n\
    -p, --proxy\n\
        respond to button proxy [default: %6$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
        this option may be repeated.\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
Session Management:\n\
    -clientID CLIENTID\n\
        client id for session management [default: %7$s]\n\
    -restore SAVEFILE\n\
        file in which to save session info [default: %8$s]\n\
", argv[0] 
	, options.debug
	, options.output
	, options.display
	, options.screen
	, show_bool(options.proxy)
	, options.clientId
	, options.saveFile
);
	/* *INDENT-ON* */
}

static void
set_defaults(void)
{
	const char *env;

	if ((env = getenv("DISPLAY")))
		options.display = strdup(env);
}

static void
get_defaults(void)
{
	const char *p;
	int n;

	if (!options.display) {
		EPRINTF("No DISPLAY environment variable nor --display option\n");
		exit(EXIT_FAILURE);
	}
	if (options.screen < 0 && (p = strrchr(options.display, '.'))
	    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
		options.screen = atoi(p);
	if (options.command == CommandDefault)
		options.command = CommandRun;

}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocale(LC_ALL, "");

	set_defaults();

	saveArgc = argc;
	saveArgv = argv;

	while (1) {
		int c, val;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",	required_argument,	NULL,	'd'},
			{"screen",	required_argument,	NULL,	's'},
			{"proxy",	no_argument,		NULL,	'p'},

			{"quit",	no_argument,		NULL,	'q'},
			{"replace",	no_argument,		NULL,	'r'},

			{"clientId",	required_argument,	NULL,	'8'},
			{"restore",	required_argument,	NULL,	'9'},

			{"debug",	optional_argument,	NULL,	'D'},
			{"verbose",	optional_argument,	NULL,	'v'},
			{"help",	no_argument,		NULL,	'h'},
			{"version",	no_argument,		NULL,	'V'},
			{"copying",	no_argument,		NULL,	'C'},
			{"?",		no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:pD::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:pD:vhVCH?");
#endif				/* _GNU_SOURCE */
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'd':	/* -d, --display DISPLAY */
			setenv("DISPLAY", optarg, TRUE);
			free(options.display);
			options.display = strdup(optarg);
			break;
		case 's':	/* -s, --screen SCREEN */
			options.screen = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 'p':	/* -p, --proxy */
			options.proxy = True;
			break;

		case 'q':	/* -q, --quit */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandQuit;
			options.command = CommandQuit;
			break;
		case 'r':	/* -r, --replace */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandReplace;
			options.command = CommandReplace;
			break;

		case '8':	/* -clientId CLIENTID */
			free(options.clientId);
			options.clientId = strdup(optarg);
			break;
		case '9':	/* -restore SAVEFILE */
			free(options.saveFile);
			options.saveFile = strdup(optarg);
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.debug++;
				break;
			}
			if ((val = strtol(optarg, &endptr, 0)) < 0)
				goto bad_option;
			if (endptr && *endptr)
				goto bad_option;
			options.debug = val;
			break;
		case 'v':	/* -v, --verbose [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.output++;
				break;
			}
			if ((val = strtol(optarg, &endptr, 0)) < 0)
				goto bad_option;
			if (endptr && *endptr)
				goto bad_option;
			options.output = val;
			break;
		case 'h':	/* -h, --help */
		case 'H':	/* -H, --? */
			command = CommandHelp;
			break;
		case 'V':	/* -V, --version */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':	/* -C, --copying */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandCopying;
			options.command = CommandCopying;
			break;
		case '?':
		default:
		      bad_option:
			optind--;
			goto bad_nonopt;
		      bad_nonopt:
			if (options.output || options.debug) {
				if (optind < argc) {
					fprintf(stderr, "%s: syntax error near '", argv[0]);
					while (optind < argc) {
						fprintf(stderr, "%s", argv[optind++]);
						fprintf(stderr, "%s", (optind < argc) ? " " : "");
					}
					fprintf(stderr, "'\n");
				} else {
					fprintf(stderr, "%s: missing option or argument", argv[0]);
					fprintf(stderr, "\n");
				}
				fflush(stderr);
			      bad_usage:
				usage(argc, argv);
			}
			exit(EXIT_SYNTAXERR);
		}
	}
	if (options.debug) {
		fprintf(stderr, "%s: option index = %d\n", argv[0], optind);
		fprintf(stderr, "%s: option count = %d\n", argv[0], argc);
	}
	if (optind < argc) {
		fprintf(stderr, "%s: excess non-option arguments near '", argv[0]);
		while (optind < argc) {
			fprintf(stderr, "%s", argv[optind++]);
			fprintf(stderr, "%s", (optind < argc) ? " " : "");
		}
		fprintf(stderr, "'\n");
		usage(argc, argv);
		exit(EXIT_SYNTAXERR);
	}
	get_defaults();
	startup(argc, argv);
	switch (command) {
	default:
	case CommandDefault:
	case CommandRun:
		DPRINTF("%s: running a new instance\n", argv[0]);
		do_run(argc, argv, False);
		break;
	case CommandQuit:
		DPRINTF("%s: asking existing instance to quit\n", argv[0]);
		do_quit(argc, argv);
		break;
	case CommandReplace:
		DPRINTF("%s: replacing existing instance\n", argv[0]);
		do_run(argc, argv, True);
		break;
	case CommandHelp:
		DPRINTF("%s: printing help message\n", argv[0]);
		help(argc, argv);
		break;
	case CommandVersion:
		DPRINTF("%s: printing version message\n", argv[0]);
		version(argc, argv);
		break;
	case CommandCopying:
		DPRINTF("%s: printing copying message\n", argv[0]);
		copying(argc, argv);
		break;
	}
	exit(EXIT_SUCCESS);
}

// vim: tw=100 com=sr0\:/**,mb\:*,ex\:*/,sr0\:/*,mb\:*,ex\:*/,b\:TRANS formatoptions+=tcqlor
