#include "vde-plug-url.h"

G_DECLARE_FINAL_TYPE(GNVDEPlugURLPort,gn_vde_plug_url_port,GN,VDE_SLIRP_PORT,GNPort)
struct _GNVDEPlugURLPort {
	GNPort parent_instance;
	char *name;
};
G_DEFINE_TYPE (GNVDEPlugURLPort,gn_vde_plug_url_port,GN_TYPE_PORT)


enum {
	PROP_NONE,
	PROP_URL,
	PROP_CRASHED_MESSAGE,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

gboolean gn_vde_plug_url_set_url(GNVDEPlugURL *plug, const char* url)
{
	g_free(plug->url);
	plug->url = g_strdup(url);
	return TRUE;
}

static void gn_vde_plug_url_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(object);
	switch (property_id) {
		case PROP_URL: {
			gn_vde_plug_url_set_url(self, g_value_get_string(value));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_plug_url_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(object);
	switch (property_id) {
		case PROP_URL: {
			g_value_set_string(value,self->url);
		} break;
		case PROP_CRASHED_MESSAGE: {
			g_value_set_string(value,self->crash_message);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static char *gn_vde_plug_url_port_get_name(GNPort *port)
{
	return g_strdup("Plug");
}

static void gn_vde_plug_url_port_init(GNVDEPlugURLPort *self)
{
}
static void gn_vde_plug_url_port_class_init(GNVDEPlugURLPortClass *klass)
{
	GNPortClass *port_class = GN_PORT_CLASS(klass);
	
	port_class->get_name = gn_vde_plug_url_port_get_name;
}
G_DEFINE_TYPE (GNVDEPlugURL,gn_vde_plug_url,GN_TYPE_NODE)

static const char* gn_vde_plug_url_get_label(GNNode *node)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(node);
	return self->current_url ? self->current_url : self->url;
}
static void gn_vde_plug_process_comunicate(GSubprocess *subprocess, GAsyncResult *res, GNVDEPlugURL *self)
{
	GObject *object = G_OBJECT(self);
	g_subprocess_communicate_utf8_finish(subprocess,res,&self->crash_message,NULL,NULL);
	if (self->crash_message) {
		// Trim '\n'
		int msglen = strlen(self->crash_message);
		while ((msglen>0) && (self->crash_message[msglen-1] == '\n'))
			self->crash_message[--msglen] = '\0';
		// Check for length
		if (!msglen) {
			g_free(self->crash_message);
			self->crash_message = NULL;
		}
	}
	g_object_notify_by_pspec(object,obj_properties[PROP_CRASHED_MESSAGE]);
}
static gboolean gn_vde_plug_url_start(GNNode *node, GError **error)
{
	GObject *object = G_OBJECT(node);
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(node);
	// Reset crash
	g_free(self->crash_message);
	self->crash_message = NULL;
	g_object_notify_by_pspec(object,obj_properties[PROP_CRASHED_MESSAGE]);
	// Update config
	g_free(self->current_url);
	self->current_url = g_strdup(self->url);
	// Start
	self->plug_process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDERR_MERGE|G_SUBPROCESS_FLAGS_STDOUT_PIPE,error,"vde_plug",self->current_url,gn_port_get_hub_sock(self->port),NULL);
	g_subprocess_communicate_utf8_async(self->plug_process,NULL,NULL,(GAsyncReadyCallback)gn_vde_plug_process_comunicate,self);
	// Cleanup
	gn_node_notify_state_change(node);
	return self->plug_process != NULL;
}
static gboolean gn_vde_plug_url_stop(GNNode *node, GError **error)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(node);
	if (self->plug_process) {
		g_subprocess_force_exit(self->plug_process);
		g_object_unref(self->plug_process);
		self->plug_process = NULL;
	}
	gn_node_notify_state_change(node);
	return TRUE;
}
static GVirDomainState gn_vde_plug_url_get_state(GNNode *node)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(node);
	if (self->crash_message)
		return GVIR_DOMAIN_STATE_CRASHED;
	if (self->plug_process)
		return GVIR_DOMAIN_STATE_RUNNING;
	else return GVIR_DOMAIN_STATE_SHUTOFF;
}
gboolean gn_vde_plug_url_need_reboot(const GNVDEPlugURL* plug)
{
	return plug->plug_process && !plug->crash_message && g_strcmp0(plug->url,plug->current_url);
}

static void gn_vde_plug_url_render(GNNode* node, cairo_t *cr)
{
	// Get real pixel size
	double picx = 1;
	double picy = 0;
	cairo_user_to_device_distance(cr,&picx,&picy);
	// Render computer screen
	// TODO Cache icons
	GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),"network-wired",picx,0);
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

static void gn_vde_plug_url_init(GNVDEPlugURL *self)
{
	self->url = g_strdup("vde://");
	
	self->ports = g_list_store_new(GN_TYPE_PORT);
	self->port = GN_PORT(g_object_new(gn_vde_plug_url_port_get_type(),"node",self,NULL));
	g_list_store_append(self->ports,self->port);
	g_object_unref(self->port);
}

static void gn_vde_plug_url_dispose(GObject *gobject)
{
	GNVDEPlugURL *self = GN_VDE_PLUG_URL(gobject);
	GNNode *node = GN_NODE(gobject);
	g_clear_object(&self->ports);
	gn_vde_plug_url_stop(node,NULL);
	G_OBJECT_CLASS(gn_vde_plug_url_parent_class)->dispose(gobject);
}

static void gn_vde_plug_url_finalize(GObject *gobject)
{
	//GNVDEPlugURL *self = gn_vde_plug_url(gobject);
	G_OBJECT_CLASS(gn_vde_plug_url_parent_class)->finalize(gobject);
}

static GListModel *gn_vde_plug_url_query_portlist_model(GNNode* node)
{
	return G_LIST_MODEL(GN_VDE_PLUG_URL(node)->ports);
}
static void gn_vde_plug_url_class_init(GNVDEPlugURLClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GNNodeClass* nodeclass = GN_NODE_CLASS(klass);
	
	nodeclass->get_label = gn_vde_plug_url_get_label;
	nodeclass->start = gn_vde_plug_url_start;
	nodeclass->force_stop = gn_vde_plug_url_stop;
	nodeclass->get_state = gn_vde_plug_url_get_state;
	nodeclass->render = gn_vde_plug_url_render;
	nodeclass->query_portlist_model = gn_vde_plug_url_query_portlist_model;
	nodeclass->widget_control_type = GN_TYPE_VDE_PLUG_URL_WIDGET;
	
	objclass->get_property = gn_vde_plug_url_get_property;
	objclass->set_property = gn_vde_plug_url_set_property;
	objclass->dispose = gn_vde_plug_url_dispose;
	objclass->finalize = gn_vde_plug_url_finalize;
	
	obj_properties[PROP_URL] = g_param_spec_string("url", "VDE URL", "VDE plug URL",
	"vde://",G_PARAM_READWRITE);
	obj_properties[PROP_CRASHED_MESSAGE] = g_param_spec_string("crashed-message", "Crash message", "The message of the crashed VDE instance",
	NULL,G_PARAM_READABLE);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	nodeclass->file_properties = g_ptr_array_copy(nodeclass->file_properties,(GCopyFunc)g_param_spec_ref,NULL);
	g_ptr_array_add(nodeclass->file_properties,g_param_spec_ref(obj_properties[PROP_URL]));
}
