#include "vde-ns.h"

static const char* vdens_base_argv[] = {"vdens","-i","port","-m",NULL};
#define vdens_base_argv_n (sizeof(vdens_base_argv)/sizeof(vdens_base_argv[0]) - 1)

GSubprocess *gn_vde_ns_subprocess_node(GNNode *node, const gchar * const *argv, GSubprocessFlags flags, GError **error)
{
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	GPtrArray *real_argv = g_ptr_array_new();
	GListModel *port_list = node_class->query_portlist_model(node);
	
	// Build command line
	for (int i = 0; i < vdens_base_argv_n; i++)
		g_ptr_array_add(real_argv,(char*)vdens_base_argv[i]);
	
	guint port_count = g_list_model_get_n_items(port_list);
	for (int i = 0; i < port_count; i++)
		g_ptr_array_add(real_argv,(char*)gn_port_get_hub_sock(GN_PORT(g_list_model_get_item(port_list,i))));
	
	static const char* vdens_dhyphen = "--";
	g_ptr_array_add(real_argv,(char*)vdens_dhyphen);
	for (int i = 0; argv[i]; i++)
		g_ptr_array_add(real_argv,(char*)argv[i]);
	g_ptr_array_add(real_argv,NULL);
	
	GSubprocess *subprocess = g_subprocess_newv((const char*const*)real_argv->pdata,flags,error);
	g_ptr_array_unref(real_argv);
	return subprocess;
}
gboolean gn_vde_ns_can(GError **error)
{
	// TODO Cache result
	// Just check if vdens works
	char* stderr_content;
	gint exit_status;
	if (!g_spawn_sync(NULL,(char**)vdens_base_argv,NULL,G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL,NULL,NULL,NULL,&stderr_content,&exit_status,error))
		return FALSE;
	
	if (exit_status) {
		g_set_error_literal(error,g_quark_from_string("dummy-domain-error"),exit_status,stderr_content);
		g_free(stderr_content);
		return FALSE;
	} else {
		g_free(stderr_content);
		return TRUE;
	}
}
