#include <stdarg.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>
#include "gtk-info-popup.h"

#include <Scintilla.h>
#include <ScintillaWidget.h>

#include "sciteco.h"
#include "interface.h"
#include "interface-gtk.h"

InterfaceGtk interface;

extern "C" {
static gboolean cmdline_key_pressed(GtkWidget *widget, GdkEventKey *event,
				    gpointer user_data);
static gboolean exit_app(GtkWidget *w, GdkEventAny *e, gpointer p);
}

InterfaceGtk::InterfaceGtk()
{
	GtkWidget *window, *vbox;
	GtkWidget *info_content;

	gtk_init(NULL, NULL);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "SciTECO");
	g_signal_connect(G_OBJECT(window), "delete-event",
			 G_CALLBACK(exit_app), NULL);

	vbox = gtk_vbox_new(FALSE, 0);

	editor_widget = scintilla_new();
	scintilla_set_id(SCINTILLA(editor_widget), 0);
	gtk_widget_set_usize(editor_widget, 500, 300);
	gtk_widget_set_can_focus(editor_widget, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), editor_widget, TRUE, TRUE, 0);

	info_widget = gtk_info_bar_new();
	info_content = gtk_info_bar_get_content_area(GTK_INFO_BAR(info_widget));
	message_widget = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(message_widget), 0., 0.);
	gtk_container_add(GTK_CONTAINER(info_content), message_widget);
	gtk_box_pack_start(GTK_BOX(vbox), info_widget, FALSE, FALSE, 0);

	cmdline_widget = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(cmdline_widget), FALSE);
	gtk_editable_set_editable(GTK_EDITABLE(cmdline_widget), FALSE);
	widget_set_font(cmdline_widget, "Courier");
	g_signal_connect(G_OBJECT(cmdline_widget), "key-press-event",
			 G_CALLBACK(cmdline_key_pressed), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), cmdline_widget, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	popup_widget = gtk_info_popup_new(cmdline_widget);

	gtk_widget_grab_focus(cmdline_widget);
	gtk_widget_show_all(window);

	ssm(SCI_SETFOCUS, TRUE);

	cmdline_update();
}

void
InterfaceGtk::msg(MessageType type, const gchar *fmt, ...)
{
	static const GtkMessageType type2gtk[] = {
		/* [MSG_USER]		= */ GTK_MESSAGE_OTHER,
		/* [MSG_INFO]		= */ GTK_MESSAGE_INFO,
		/* [MSG_WARNING]	= */ GTK_MESSAGE_WARNING,
		/* [MSG_ERROR]		= */ GTK_MESSAGE_ERROR
	};

	va_list ap;
	gchar buf[255];

	va_start(ap, fmt);
	g_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	switch (type) {
	case MSG_USER:
		g_printf("%s\n", buf);
		break;
	case MSG_INFO:
		g_printf("Info: %s\n", buf);
		break;
	case MSG_WARNING:
		g_fprintf(stderr, "Warning: %s\n", buf);
		break;
	case MSG_ERROR:
		g_fprintf(stderr, "Error: %s\n", buf);
		break;
	}

	gtk_info_bar_set_message_type(GTK_INFO_BAR(info_widget),
				      type2gtk[type]);
	gtk_label_set_text(GTK_LABEL(message_widget), buf);
}

void
InterfaceGtk::cmdline_update(const gchar *cmdline)
{
	gint pos = 1;

	gtk_entry_set_text(GTK_ENTRY(cmdline_widget), "*");
	gtk_editable_insert_text(GTK_EDITABLE(cmdline_widget),
				 cmdline, -1, &pos);
	gtk_editable_set_position(GTK_EDITABLE(cmdline_widget), pos);
}

void
InterfaceGtk::popup_add_filename(PopupFileType type,
				 const gchar *filename, bool highlight)
{
	static const GtkInfoPopupFileType type2gtk[] = {
		/* [POPUP_FILE]		= */ GTK_INFO_POPUP_FILE,
		/* [POPUP_DIRECTORY]	= */ GTK_INFO_POPUP_DIRECTORY
	};

	gtk_info_popup_add_filename(GTK_INFO_POPUP(popup_widget),
				    type2gtk[type], filename, highlight);
}

void
InterfaceGtk::popup_clear(void)
{
	if (gtk_widget_get_visible(popup_widget)) {
		gtk_widget_hide(popup_widget);
		gtk_info_popup_clear(GTK_INFO_POPUP(popup_widget));
	}
}

void
InterfaceGtk::widget_set_font(GtkWidget *widget, const gchar *font_name)
{
	PangoFontDescription *font_desc;

	font_desc = pango_font_description_from_string(font_name);
	gtk_widget_modify_font(widget, font_desc);
	pango_font_description_free(font_desc);
}

/*
 * GTK+ callbacks
 */

static gboolean
cmdline_key_pressed(GtkWidget *widget, GdkEventKey *event,
		    gpointer user_data __attribute__((unused)))
{
#ifdef DEBUG
	g_printf("KEY \"%s\" (%d) SHIFT=%d CNTRL=%d\n",
		 event->string, *event->string,
		 event->state & GDK_SHIFT_MASK, event->state & GDK_CONTROL_MASK);
#endif

	switch (event->keyval) {
	case GDK_BackSpace:
		cmdline_keypress('\b');
		break;
	case GDK_Tab:
		cmdline_keypress('\t');
		break;
	case GDK_Return:
		switch (interface.ssm(SCI_GETEOLMODE)) {
		case SC_EOL_CR:
			cmdline_keypress('\r');
			break;
		case SC_EOL_CRLF:
			cmdline_keypress('\r');
			/* fall through */
		case SC_EOL_LF:
		default:
			cmdline_keypress('\n');
		}
		break;
	default:
		if (*event->string)
			cmdline_keypress(*event->string);
	}

	return TRUE;
}

static gboolean
exit_app(GtkWidget *w __attribute__((unused)),
	 GdkEventAny *e __attribute__((unused)),
	 gpointer p __attribute__((unused)))
{
	gtk_main_quit();
	return TRUE;
}