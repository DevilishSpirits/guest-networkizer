#include "vde-slirp.h"

const GNVDESlirpConfig gn_vde_slirp_config_defaults = {
	TRUE // DHCP is enabled by default
};

gboolean gn_vde_slirp_config_equal(const GNVDESlirpConfig* a, const GNVDESlirpConfig* b)
{
	return memcmp(a,b,sizeof(GNVDESlirpConfig)) == 0;
}
void gn_vde_slirp_config_copy(const GNVDESlirpConfig* from, GNVDESlirpConfig* to)
{
	memcpy(to,from,sizeof(GNVDESlirpConfig));
}
GNVDESlirpConfig* gn_vde_slirp_config_dup(const GNVDESlirpConfig* config)
{
	return memcpy(g_slice_new(GNVDESlirpConfig),config,sizeof(GNVDESlirpConfig));
}
void gn_vde_slirp_config_free(gpointer boxed)
{
	GNVDESlirpConfig* self = (GNVDESlirpConfig*)boxed;
	g_free(self);
}
G_DEFINE_BOXED_TYPE(GNVDESlirpConfig,gn_vde_slirp_config,gn_vde_slirp_config_dup,gn_vde_slirp_config_free)

G_DECLARE_FINAL_TYPE(GNVDESlirpPort,gn_vde_slirp_port,GN,VDE_SLIRP_PORT,GNPort)
struct _GNVDESlirpPort {
	GNPort parent_instance;
	char *name;
};
G_DEFINE_TYPE (GNVDESlirpPort,gn_vde_slirp_port,GN_TYPE_PORT)


enum {
	PROP_NONE,
	PROP_CONFIG,
	PROP_CURRENT_CONFIG,
	PROP_ENABLE_DHCP,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

static void gn_vde_slirp_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVDESlirp *self = GN_VDE_SLIRP(object);
	switch (property_id) {
		case PROP_CONFIG: {
			gn_vde_slirp_config_copy(g_value_get_boxed(value),&self->config);
		} break;
		case PROP_ENABLE_DHCP: {
			self->config.enable_dhcp = g_value_get_boolean(value);
			g_object_notify_by_pspec(G_OBJECT(self),obj_properties[PROP_CONFIG]);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_slirp_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVDESlirp *self = GN_VDE_SLIRP(object);
	switch (property_id) {
		case PROP_CONFIG: {
			g_value_take_boxed(value,gn_vde_slirp_config_dup(&self->config));
		} break;
		case PROP_CURRENT_CONFIG: {
			g_value_take_boxed(value,gn_vde_slirp_config_dup(&self->current_config));
		} break;
		case PROP_ENABLE_DHCP: {
			g_value_set_boolean(value,self->config.enable_dhcp);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

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
	// Update current_config
	gn_vde_slirp_config_copy(&self->config,&self->current_config);
	g_object_notify_by_pspec(G_OBJECT(self),obj_properties[PROP_CURRENT_CONFIG]);
	// Build argv
	static char* base_argv[] = {"slirpvde","-q","-s"};
	static char* option_dhcp = "-D";
	GPtrArray *argv = g_ptr_array_sized_new(4);
	for (int i = 0; i < sizeof(base_argv)/sizeof(base_argv[0]); i++)
		g_ptr_array_add(argv,base_argv[i]);
	g_ptr_array_add(argv,gn_port_get_hub_sock(self->port));
	
	if (self->current_config.enable_dhcp)
		g_ptr_array_add(argv,option_dhcp);
	// Start
	g_ptr_array_add(argv,NULL);
	self->slirp_process = g_subprocess_newv(argv->pdata,0,error);
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
gboolean gn_vde_slirp_need_reboot(const GNVDESlirp* slirp)
{
	return slirp->slirp_process && !gn_vde_slirp_config_equal(&slirp->config,&slirp->current_config);
}

static void gn_vde_slirp_init(GNVDESlirp *self)
{
	self->ports = g_list_store_new(GN_TYPE_PORT);
	self->port = GN_VDE_SLIRP_PORT(g_object_new(gn_vde_slirp_port_get_type(),"node",self,NULL));
	g_list_store_append(self->ports,self->port);
	g_object_unref(self->port);
	gn_vde_slirp_config_copy(&gn_vde_slirp_config_defaults,&self->config);
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
	
	objclass->get_property = gn_vde_slirp_get_property;
	objclass->set_property = gn_vde_slirp_set_property;
	objclass->dispose = gn_vde_slirp_dispose;
	objclass->finalize = gn_vde_slirp_finalize;
	
	obj_properties[PROP_CONFIG] = g_param_spec_boxed("config", "Configuration", "NAT configuration",
	GN_TYPE_VDE_SLIRP_CONFIG,G_PARAM_READWRITE);
	obj_properties[PROP_CURRENT_CONFIG] = g_param_spec_boxed("current-config", "Current configuration", "NAT configuration of the running instance",
	GN_TYPE_VDE_SLIRP_CONFIG,G_PARAM_READABLE);
	obj_properties[PROP_ENABLE_DHCP] = g_param_spec_boolean("enable-dhcp", "Enable DHCP", "Enable DHCP server",
	TRUE,G_PARAM_READWRITE);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
}
