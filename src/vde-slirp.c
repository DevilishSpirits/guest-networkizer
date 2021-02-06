#include "vde-slirp.h"

G_DECLARE_FINAL_TYPE(GNVDESlirpPort,gn_vde_slirp_port,GN,VDE_SLIRP_PORT,GNPort)
struct _GNVDESlirpPort {
	GNPort parent_instance;
	char *name;
};
G_DEFINE_TYPE (GNVDESlirpPort,gn_vde_slirp_port,GN_TYPE_PORT)
static const char *gn_vde_slirp_port_get_name(GNPort *port)
{
	GNVDESlirpPort *self = GN_VDE_SLIRP_PORT(port);
	if (!self->name)
		self->name = g_strdup("LAN");
	return self->name;
}

static void gn_vde_slirp_port_init(GNVDESlirpPort *self)
{
}
static void gn_vde_slirp_port_class_init(GNVDESlirpPortClass *klass)
{
	GNPortClass *port_class = GN_PORT_CLASS(klass);
	
	port_class->get_name = gn_vde_slirp_port_get_name;
}
G_DEFINE_TYPE (GNVDESlirp,gn_vde_slirp,GN_TYPE_NODE)

static gboolean gn_vde_slirp_start(GNNode *node, GError **error)
{
	GNVDESlirp *self = GN_VDE_SLIRP(node);
	self->slirp_process = g_subprocess_new(0,error,"slirpvde","-D","-q",gn_port_get_hub_sock(self->port),NULL);
	return self->slirp_process != NULL;
}
static gboolean gn_vde_slirp_stop(GNNode *node, GError **error)
{
	GNVDESlirp *self = GN_VDE_SLIRP(node);
	if (self->slirp_process) {
		g_subprocess_force_exit(self->slirp_process);
		g_object_unref(self->slirp_process);
		self->slirp_process = NULL;
	}
	return TRUE;
}
static GVirDomainState gn_vde_slirp_get_state(GNNode *node)
{
	GNVDESlirp *self = GN_VDE_SLIRP(node);
	if (self->slirp_process)
		return GVIR_DOMAIN_STATE_RUNNING;
	else return GVIR_DOMAIN_STATE_SHUTOFF;
}

static void gn_vde_slirp_init(GNVDESlirp *self)
{
	self->ports = g_list_store_new(GN_TYPE_PORT);
	self->port = GN_VDE_SLIRP_PORT(g_object_new(gn_vde_slirp_port_get_type(),"node",self,NULL));
	g_list_store_append(self->ports,self->port);
	g_object_unref(self->port);
}

static void gn_vde_slirp_dispose(GObject *gobject)
{
	GNVDESlirp *self = GN_VDE_SLIRP(gobject);
	GNNode *node = GN_NODE(gobject);
	g_clear_object(&self->ports);
	gn_vde_slirp_stop(node,NULL);
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
	
	nodeclass->start = gn_vde_slirp_start;
	nodeclass->stop = gn_vde_slirp_stop;
	nodeclass->get_state = gn_vde_slirp_get_state;
	nodeclass->query_portlist_model = gn_vde_slirp_query_portlist_model;
	nodeclass->widget_control_type = GN_TYPE_VDE_SLIRP_WIDGET;
	
	objclass->dispose = gn_vde_slirp_dispose;
	objclass->finalize = gn_vde_slirp_finalize;
}
