#include "gn-main-window.h"
#include "vir-domain-list.h"
#include <libvirt-gobject/libvirt-gobject.h>
#include <gtk/gtk.h>

GtkApplication *app = NULL;
GNMainWindow   *mainwindow = NULL;
GVirConnection *vir_connection = NULL;

static void app_startup(GApplication *application, gpointer user_data)
{
	GError *error = NULL;
	// Install local hicolor theme
	gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(),"/me/d_spirits/guest_networkizer/icons");
	// TODO Multiple windows
	mainwindow = GN_MAIN_WINDOW(gtk_widget_new(GN_TYPE_MAIN_WINDOW,"application",application,"visible",TRUE,NULL));
	gtk_window_set_application(GTK_WINDOW(mainwindow),app);
	// TODO Do not hard-code local QEMU socket
	vir_connection = gvir_connection_new("qemu:///session");
	if (!gvir_connection_open(vir_connection,NULL,&error))
		g_error("Failed to connect to 'qemu:///session': %s",error->message);
	// TODO Clean this shit
	gn_vir_domain_list_bind_list_box_to_connection(mainwindow->virt_listbox,vir_connection);
}
static void app_activate(GApplication *application, gpointer user_data)
{
	gtk_window_present(GTK_WINDOW(mainwindow));
}

int main(int argc, char** argv)
{
	app = gtk_application_new("me.d_spirits.guest_networkizer", G_APPLICATION_NON_UNIQUE);
	g_signal_connect(app,"startup",G_CALLBACK(app_startup),NULL);
	g_signal_connect(app,"activate",G_CALLBACK(app_activate),NULL);
	return g_application_run(G_APPLICATION(app),argc,argv);
}


