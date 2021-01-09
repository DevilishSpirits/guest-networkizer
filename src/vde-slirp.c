#include "vde-slirp.h"

G_DECLARE_FINAL_TYPE(GNVDESlirpPort,gn_vde_slirp_port,GN,VDE_SLIRP_PORT,GNPlug)
struct _GNVDESlirpPort {
	GNPlug parent_instance;
	GSubprocess *subprocess;
	char *name;
};
G_DEFINE_TYPE (GNVDESlirpPort,gn_vde_slirp_port,GN_TYPE_PLUG)
static const char *gn_vde_slirp_port_get_name(GNPort *port)
{
	GNVDESlirpPort *self = GN_VDE_SLIRP_PORT(port);
	if (!self->name)
		self->name = g_strdup("LAN");
	return self->name;
}
static gboolean gn_vde_slirp_port_connect_vde2(GNPlug* port, const char* socket_path, int port_no, GError **error)
{
	GNVDESlirpPort *self = GN_VDE_SLIRP_PORT(port);
	self->subprocess = g_subprocess_new(0,error,"slirpvde","-D","-q",socket_path,NULL);
	return self->subprocess != NULL;
}
static void gn_vde_slirp_port_disconnect(GNPlug* port)
{
	GNVDESlirpPort *self = GN_VDE_SLIRP_PORT(port);
	g_subprocess_force_exit(self->subprocess);
	g_clear_object(&self->subprocess);
}
static void gn_vde_slirp_port_init(GNVDESlirpPort *self)
{
}
static void gn_vde_slirp_port_class_init(GNVDESlirpPortClass *klass)
{
	GNPlugClass *plug_klass = GN_PLUG_CLASS(klass);
	GNPortClass *port_class = GN_PORT_CLASS(klass);
	
	plug_klass->connect_vde2 = gn_vde_slirp_port_connect_vde2;
	plug_klass->disconnect = gn_vde_slirp_port_disconnect;
	
	port_class->get_name = gn_vde_slirp_port_get_name;
}
G_DEFINE_TYPE (GNVDESlirp,gn_vde_slirp,GN_TYPE_NODE)

static void gn_vde_slirp_init(GNVDESlirp *self)
{
	self->ports = g_list_store_new(GN_TYPE_PLUG);
	GNVDESlirpPort *port = GN_VDE_SLIRP_PORT(g_object_new(gn_vde_slirp_port_get_type(),"node",self,NULL));
	g_list_store_append(self->ports,port);
	g_object_unref(port);
}

static void gn_vde_slirp_dispose(GObject *gobject)
{
	GNVDESlirp *self = GN_VDE_SLIRP(gobject);
	g_clear_object(&self->ports);
	G_OBJECT_CLASS(gn_vde_slirp_parent_class)->dispose(gobject);
}

static void gn_vde_slirp_finalize(GObject *gobject)
{
	//GNVDESlirp *self = GN_VDE_SLIRP(gobject);
	G_OBJECT_CLASS(gn_vde_slirp_parent_class)->finalize(gobject);
}

static GListModel *gn_vde_slirp_query_portlist_model(GNNode* node)
{
	return GN_VDE_SLIRP(node)->ports;
}
static void gn_vde_slirp_class_init(GNVDESlirpClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GNNodeClass* nodeclass = GN_NODE_CLASS(klass);
	
	nodeclass->query_portlist_model = gn_vde_slirp_query_portlist_model;
	
	objclass->dispose = gn_vde_slirp_dispose;
	objclass->finalize = gn_vde_slirp_finalize;
}
