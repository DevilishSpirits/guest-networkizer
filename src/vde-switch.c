#include "vde-switch.h"

G_DECLARE_FINAL_TYPE(GNVDESwitchPort,gn_vde_switch_port,GN,VDE_SWITCH_PORT,GNPort)
struct _GNVDESwitchPort {
	GNPort parent_instance;
	GSubprocess* real_side_plug;
	GSubprocess* buff_side_plug;
	int  port_no;
	char *sock_path;
	char *name;
};
G_DEFINE_TYPE (GNVDESwitchPort,gn_vde_switch_port,GN_TYPE_PORT)
static const char *gn_vde_switch_get_name(GNPort *port)
{
	GNVDESwitchPort *self = GN_VDE_SWITCH_PORT(port);
	if (!self->name)
		self->name = g_strdup_printf("Port %d",self->port_no+1);
	return self->name;
}
static void gn_vde_switch_port_init(GNVDESwitchPort *self)
{
}
static void gn_vde_switch_port_class_init(GNVDESwitchPortClass *klass)
{
	GNPortClass *port_class = GN_PORT_CLASS(klass);
	
	port_class->get_name = gn_vde_switch_get_name;
}
G_DEFINE_TYPE (GNVDESwitch,gn_vde_switch,GN_TYPE_NODE)

static void gn_vde_switch_init(GNVDESwitch *self)
{
	// TODO Move that elsewhere
	self->sock_path = g_dir_make_tmp (NULL,/* TODO GError **error */NULL);
	self->switch_process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE,NULL/* TODO GError **error */,"vde_switch","-s",self->sock_path,NULL);
	self->ports = g_list_store_new(GN_TYPE_PORT);
	// TODO Adaptative port number
	for (int i = 0; i < 8; i++) {
		// TODO Use clean initialization
		GNVDESwitchPort *port = GN_VDE_SWITCH_PORT(g_object_new(gn_vde_switch_port_get_type(),"node",self,NULL));
		port->sock_path = self->sock_path;
		port->port_no = i;
		g_list_store_append(self->ports,port);
		g_object_unref(port);
		
		int fds_r2b[2] = {-1,-1};
		int fds_b2r[2] = {-1,-1};
		g_unix_open_pipe(fds_r2b,0,NULL);
		g_unix_open_pipe(fds_b2r,0,NULL);
		port->real_side_plug = gn_mk_plug_no(self->sock_path,i+1,fds_b2r[0],fds_r2b[1],NULL);
		port->buff_side_plug = gn_port_do_plug(port,fds_r2b[0],fds_b2r[1],NULL);
		close(fds_r2b[0]);
		close(fds_r2b[1]);
		close(fds_b2r[0]);
		close(fds_b2r[1]);
	}
	
}

static void gn_vde_switch_dispose(GObject *gobject)
{
	GNVDESwitch *self = GN_VDE_SWITCH(gobject);
	g_clear_object(&self->ports);
	g_clear_object(&self->switch_process);
	G_OBJECT_CLASS(gn_vde_switch_parent_class)->dispose(gobject);
}

static void gn_vde_switch_finalize(GObject *gobject)
{
	GNVDESwitch *self = GN_VDE_SWITCH(gobject);
	g_free(self->sock_path);
	G_OBJECT_CLASS(gn_vde_switch_parent_class)->finalize(gobject);
}

static GListModel *gn_vde_switch_query_portlist_model(GNNode* node)
{
	return GN_VDE_SWITCH(node)->ports;
}
static void gn_vde_switch_class_init(GNVDESwitchClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GNNodeClass* nodeclass = GN_NODE_CLASS(klass);
	
	nodeclass->query_portlist_model = gn_vde_switch_query_portlist_model;
	
	objclass->dispose = gn_vde_switch_dispose;
	objclass->finalize = gn_vde_switch_finalize;
}
