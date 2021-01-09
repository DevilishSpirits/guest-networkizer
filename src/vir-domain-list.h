#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libvirt-gobject/libvirt-gobject.h>

GListModel *gn_vir_domain_list_get(GVirConnection *connection);
void gn_vir_domain_list_bind_list_box_to_connection(GtkListBox* box, GVirConnection *connection);
