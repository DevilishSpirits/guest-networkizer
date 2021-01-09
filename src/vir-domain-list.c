#include "vir-domain-list.h"

// TODO Filter non QEMU VMs
/* TODO Harden that with UUID only comparison
static gboolean gn_vir_domain_equal(gconstpointer a, gconstpointer b)
{
	// FIXME Is string comparison necessary here ?
	char* uuid_a = gvir_domain_get_uuid(GVIR_DOMAIN(a));
	char* uuid_b = gvir_domain_get_uuid(GVIR_DOMAIN(b));
	gboolean equal = g_str_equal(uuid_a,uuid_b);
	g_free(a);
	g_free(b);
	return equal;
}*/

gboolean gn_vir_domain_is_compatible(GVirDomain *domain)
{
	// TODO Filter for supported VMs only
	return true;
}

void gn_vir_domain_list_add(GListStore *store, GVirDomain *arg1)
{
	if (gn_vir_domain_is_compatible(arg1))
		g_list_store_insert(store,0/* TODO Sorting insertion */,arg1);
}
void gn_vir_domain_list_remove(GListStore *store, GVirDomain *arg1)
{
	guint position;
	if (g_list_store_find(store,arg1,&position))
		g_list_store_remove(store,position);
}

static GListModel *gn_vir_domain_list_new(GVirConnection *connection)
{
	GError *error = NULL;
	GListStore *store = g_list_store_new(GVIR_TYPE_DOMAIN);
	g_signal_connect_object(connection,"domain-added",G_CALLBACK(gn_vir_domain_list_add),store,G_CONNECT_SWAPPED);
	g_signal_connect_object(connection,"domain-removed",G_CALLBACK(gn_vir_domain_list_remove),store,G_CONNECT_SWAPPED);
	// Refresh domains
	if (!gvir_connection_fetch_domains(connection,NULL,&error)) {
		g_warning("In gn_vir_domain_list_new, gvir_connection_fetch_domains failed: %s",error->message);
		g_error_free(error);
	}
	// Add pre-exisiting domains
	GList *domains = gvir_connection_get_domains(connection);
	while (domains) {
		GVirDomain *domain = GVIR_DOMAIN(domains->data);
		gn_vir_domain_list_add(store,domain);
		g_object_unref(domain);
		
		if (domains->next) {
			domains = domains->next;
			g_list_free_1(domains->prev);
			domains->prev = NULL;
		} else {
			g_list_free_1(domains);
			domains = NULL;
			break;
		}
	}
	return G_LIST_MODEL(store);
}

GListModel *gn_vir_domain_list_get(GVirConnection *connection)
{
	static const char* data_key = "gn-vir-domain-list-instance";
	GObject *object = G_OBJECT(connection);
	GListModel *store = g_object_get_data(object,data_key);
	if (!store) {
		store = gn_vir_domain_list_new(connection);
		g_object_set_data_full(object,data_key,store,g_object_unref);
	}
	return store;
}

static GtkWidget *gn_vir_domain_list_bind_list_box_create_widget(gpointer item, gpointer user_data)
{
	GVirDomain *domain = GVIR_DOMAIN(item);
	GtkWidget *row = gtk_list_box_row_new();
	gtk_container_add(GTK_CONTAINER(row),gtk_label_new(gvir_domain_get_name(domain)));
	g_object_set_data_full(G_OBJECT(row),"vir-domain",domain,g_object_unref);
	gtk_widget_show_all(row);
	return row;
}
static void gn_vir_domain_list_bind_list_box(GtkListBox* box, GListModel *store)
{
	gtk_list_box_bind_model(box,store,gn_vir_domain_list_bind_list_box_create_widget,NULL,NULL);
}
void gn_vir_domain_list_bind_list_box_to_connection(GtkListBox* box, GVirConnection *connection)
{
	gn_vir_domain_list_bind_list_box(box,gn_vir_domain_list_get(connection));
}



