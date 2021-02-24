#include "common.h"
#include <libvirt-gobject/libvirt-gobject.h>

GtkApplication *app = NULL;
GNMainWindow   *mainwindow = NULL;
GVirConnection *vir_connection = NULL;

static int    global_argc;
static char** global_argv;

gboolean do_main_test(gpointer user_data)
{
	main_test(global_argc,global_argv);
	return G_SOURCE_REMOVE;
}

static void app_startup(GApplication *application, gpointer user_data)
{
	GError *error = NULL;
	// Install local hicolor theme
	gtk_icon_theme_add_resource_path(gtk_icon_theme_get_default(),"/me/d_spirits/guest_networkizer/icons");
	mainwindow = GN_MAIN_WINDOW(gtk_widget_new(GN_TYPE_MAIN_WINDOW,"application",application,"visible",TRUE,NULL));
	gtk_window_set_application(GTK_WINDOW(mainwindow),app);
	g_idle_add(do_main_test,NULL);
}
static void app_activate(GApplication *application, gpointer user_data)
{
	gtk_window_present(GTK_WINDOW(mainwindow));
}

int main(int argc, char** argv)
{
	if (!gtk_init_check(&argc,&argv)) {
		puts("Bail out! GTK initialization failed.");
		return 63;
	}
	global_argc = argc;
	global_argv = argv;
	app = gtk_application_new("me.d_spirits.guest_networkizer", G_APPLICATION_NON_UNIQUE);
	g_signal_connect(app,"startup",G_CALLBACK(app_startup),NULL);
	g_signal_connect(app,"activate",G_CALLBACK(app_activate),NULL);
	return g_application_run(G_APPLICATION(app),argc,argv);
}


