#include "vde-slirp.h"

gboolean gn_vde_slirp_config_equal(const GNVDESlirpConfig* a, const GNVDESlirpConfig* b)
{
	return TRUE
		&& g_inet_address_equal(a->dns_server,b->dns_server)
		&& a->enable_dhcp == b->enable_dhcp
		&& !g_strcmp0(a->tftp_share,b->tftp_share)
	;
}
void gn_vde_slirp_config_copy(const GNVDESlirpConfig* from, GNVDESlirpConfig* to)
{
	g_clear_object(&to->dns_server);
	to->dns_server = g_object_ref(from->dns_server);
	to->enable_dhcp = from->enable_dhcp;
	free(to->tftp_share);
	if (from->tftp_share)
		to->tftp_share = g_strdup(from->tftp_share);
	else to->tftp_share = NULL;
}
GNVDESlirpConfig* gn_vde_slirp_config_dup(const GNVDESlirpConfig* config)
{
	GNVDESlirpConfig* new_config = g_slice_new(GNVDESlirpConfig);
	new_config->tftp_share = NULL;
	gn_vde_slirp_config_copy(config,new_config);
	return new_config;
}
void gn_vde_slirp_config_free(gpointer boxed)
{
	GNVDESlirpConfig* self = (GNVDESlirpConfig*)boxed;
	g_object_unref(self->dns_server);
	free(self->tftp_share);
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
	PROP_DNS_ADDRESS,
	PROP_ENABLE_DHCP,
	PROP_TFTP_SHARE,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

void gn_vde_slirp_config_set_defaults(GNVDESlirpConfig* config)
{
	g_clear_object(&config->dns_server);
	config->dns_server = g_inet_address_new_from_string(G_PARAM_SPEC_STRING(obj_properties[PROP_DNS_ADDRESS])->default_value);
	
	config->enable_dhcp = G_PARAM_SPEC_BOOLEAN(obj_properties[PROP_ENABLE_DHCP])->default_value;
	config->tftp_share = G_PARAM_SPEC_STRING(obj_properties[PROP_TFTP_SHARE])->default_value;
}

gboolean gn_vde_slirp_set_dns_address(GNVDESlirp *slirp, const char* address)
{
	GInetAddress *new_address = g_inet_address_new_from_string(address);
	if (!new_address)
		return FALSE;
	if (g_inet_address_get_family(new_address) != G_SOCKET_FAMILY_IPV4)
		return FALSE;
	// Test passed, apply change
	g_object_unref(slirp->config.dns_server);
	slirp->config.dns_server = new_address;
	g_object_notify_by_pspec(G_OBJECT(slirp),obj_properties[PROP_DNS_ADDRESS]);
	g_object_notify_by_pspec(G_OBJECT(slirp),obj_properties[PROP_CONFIG]);
	return TRUE;
}
void gn_vde_slirp_set_tftp_share(GNVDESlirp *slirp, const char* path)
{
	if (g_strcmp0(slirp->config.tftp_share,path)) {
		free(slirp->config.tftp_share);
		slirp->config.tftp_share = g_strdup(path);
		g_object_notify_by_pspec(G_OBJECT(slirp),obj_properties[PROP_TFTP_SHARE]);
		g_object_notify_by_pspec(G_OBJECT(slirp),obj_properties[PROP_CONFIG]);
	}
}

static void gn_vde_slirp_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVDESlirp *self = GN_VDE_SLIRP(object);
	switch (property_id) {
		case PROP_CONFIG: {
			gn_vde_slirp_config_copy(g_value_get_boxed(value),&self->config);
		} break;
		case PROP_DNS_ADDRESS: {
			gn_vde_slirp_set_dns_address(self,g_value_get_string(value));
		} break;
		case PROP_ENABLE_DHCP: {
			self->config.enable_dhcp = g_value_get_boolean(value);
			g_object_notify_by_pspec(G_OBJECT(self),obj_properties[PROP_CONFIG]);
		} break;
		case PROP_TFTP_SHARE: {
			gn_vde_slirp_set_tftp_share(self,g_value_get_string(value));
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
		case PROP_DNS_ADDRESS: {
			g_value_take_string(value,g_inet_address_to_string(self->config.dns_server));
		} break;
		case PROP_ENABLE_DHCP: {
			g_value_set_boolean(value,self->config.enable_dhcp);
		} break;
		case PROP_TFTP_SHARE: {
			g_value_set_string(value,self->config.tftp_share);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static char *gn_vde_slirp_port_get_name(GNPort *port)
{
	return g_strdup("LAN");
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

static const char* gn_vde_slirp_get_label(GNNode *node)
{
	// TODO Make this dynamic
	static const char* result = "10.0.2.2/24";
	return result;
}
static gboolean gn_vde_slirp_start(GNNode *node, GError **error)
{
	GNVDESlirp *self = GN_VDE_SLIRP(node);
	// Update current_config
	gn_vde_slirp_config_copy(&self->config,&self->current_config);
	g_object_notify_by_pspec(G_OBJECT(self),obj_properties[PROP_CURRENT_CONFIG]);
	// Build argv
	static char* base_argv[] = {"slirpvde","-q","-s"};
	static char* option_dhcp = "-D";
	static char* option_dns = "-N";
	static char* option_tftp = "-t";
	GPtrArray *argv = g_ptr_array_sized_new(4);
	for (int i = 0; i < sizeof(base_argv)/sizeof(base_argv[0]); i++)
		g_ptr_array_add(argv,base_argv[i]);
	g_ptr_array_add(argv,(char*)gn_port_get_hub_sock(self->port));
	
	g_ptr_array_add(argv,option_dns);
	char* dns_address = g_inet_address_to_string(self->current_config.dns_server);
	g_ptr_array_add(argv,dns_address);
	
	if (self->current_config.enable_dhcp)
		g_ptr_array_add(argv,option_dhcp);
		
	if (self->current_config.tftp_share) {
		g_ptr_array_add(argv,option_tftp);
		g_ptr_array_add(argv,self->current_config.tftp_share);
	}
	
	// Start
	g_ptr_array_add(argv,NULL);
	self->slirp_process = g_subprocess_newv((const gchar* const*)argv->pdata,0,error);
	
	// Cleanup
	g_free(dns_address);
	g_ptr_array_unref(argv);
	gn_node_notify_state_change(node);
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
	gn_node_notify_state_change(node);
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

static void gn_vde_slirp_render(GNNode* node, cairo_t *cr)
{
	// Get real pixel size
	double picx = 1;
	double picy = 0;
	cairo_user_to_device_distance(cr,&picx,&picy);
	// Render computer screen
	// TODO Cache icons
	GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),"network-nat",picx,0);
	GdkPixbuf *pixbuf = gtk_icon_info_load_icon(icon_info,NULL);
	g_object_unref(icon_info);
	gdk_cairo_set_source_pixbuf(cr,pixbuf,0,0);
	g_object_unref(pixbuf);
	cairo_matrix_t matrix;
	cairo_matrix_init_scale(&matrix,picx,picx);
	cairo_matrix_translate(&matrix,.5,.5);
	cairo_pattern_set_matrix(cairo_get_source(cr),&matrix);
	cairo_rectangle(cr,-.5,-.5,1,1);
	cairo_fill(cr);
}

static void gn_vde_slirp_init(GNVDESlirp *self)
{
	self->ports = g_list_store_new(GN_TYPE_PORT);
	self->port = GN_PORT(g_object_new(gn_vde_slirp_port_get_type(),"node",self,NULL));
	g_list_store_append(self->ports,self->port);
	g_object_unref(self->port);
	gn_vde_slirp_config_set_defaults(&self->config);
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
	return G_LIST_MODEL(GN_VDE_SLIRP(node)->ports);
}
static void gn_vde_slirp_class_init(GNVDESlirpClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GNNodeClass* nodeclass = GN_NODE_CLASS(klass);
	
	nodeclass->get_label = gn_vde_slirp_get_label;
	nodeclass->start = gn_vde_slirp_start;
	nodeclass->stop = gn_vde_slirp_stop;
	nodeclass->get_state = gn_vde_slirp_get_state;
	nodeclass->render = gn_vde_slirp_render;
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
	obj_properties[PROP_DNS_ADDRESS] = g_param_spec_string("dns-address", "DNS server", "DNS server address",
	"10.0.2.3",G_PARAM_READWRITE);
	obj_properties[PROP_ENABLE_DHCP] = g_param_spec_boolean("enable-dhcp", "Enable DHCP", "Enable DHCP server",
	TRUE,G_PARAM_READWRITE);
	obj_properties[PROP_TFTP_SHARE] = g_param_spec_string("tftp-share", "TFTP share", "TFTP host share path",
	NULL,G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	nodeclass->file_properties = g_ptr_array_copy(nodeclass->file_properties,(GCopyFunc)g_param_spec_ref,NULL);
	g_ptr_array_add(nodeclass->file_properties,g_param_spec_ref(obj_properties[PROP_DNS_ADDRESS]));
	g_ptr_array_add(nodeclass->file_properties,g_param_spec_ref(obj_properties[PROP_ENABLE_DHCP]));
}
