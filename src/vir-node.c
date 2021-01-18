#include "vir-node.h"

G_DECLARE_FINAL_TYPE(GNVirNodePort,gn_vir_node_port,GN,VIR_NODE_PORT,GNPlug)
/* A port is QEMU device = -netdev vde = this->hub = vde_plug =>...
 * This allow live swapping of vde_cables.
 */
struct _GNVirNodePort {
	GNPlug parent_instance;
	char        *hub_sock;
	GSubprocess *hub;
	GSubprocess *plug;
	
	char *qemu_driver;
	char *qemu_id;
	guint8 mac[8];
};
G_DEFINE_TYPE (GNVirNodePort,gn_vir_node_port,GN_TYPE_PLUG)

static gboolean gn_vir_node_perform_qemu(GNVirNodePort *self, const char* cmd, char** result, GError** error)
{
	#define gn_vir_node_perform_qemu_debug_fmtnargs "%s {%s} (qemu) %s\n%s",gvir_domain_get_name(vir_node->domain),gvir_domain_get_uuid(vir_node->domain),cmd,*result
	GNNode *node = gn_port_get_node(GN_PORT(self));
	GNVirNode *vir_node = GN_VIR_NODE(node);
	// Put a dummy_out if none is provided
	char* dummy_out = NULL;
	if (!result)
		result = &dummy_out;
	gboolean warning = FALSE;
	gboolean success = virDomainQemuMonitorCommand(vir_node->domain_handle,cmd,result,VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP/* FIXME */) == 0;
	// Check if there's another error
	if (success && *result) {
		const char qemu_error_prefix[] = "Error: ";
		success = strncmp(qemu_error_prefix,*result,sizeof(qemu_error_prefix)) != 0;
		warning = TRUE; // If there's a message there's a problem
	}
	if (!success) {
		g_set_error(error,g_quark_from_string("dummy-qemu-error-domain-quark"),-1,gn_vir_node_perform_qemu_debug_fmtnargs);
		warning = TRUE;
	}
	if (warning/*FIXME*/ || 1)
		g_warning(gn_vir_node_perform_qemu_debug_fmtnargs);
	else g_debug(gn_vir_node_perform_qemu_debug_fmtnargs);
	g_free(dummy_out);
	return TRUE;
}

static gboolean gn_vir_node_port_qemu_set_carrier(GNVirNodePort *self, gboolean active, GError** error)
{
	static const char* link_level[2] = {"off","on"};
	active = active != FALSE; // Avoid buffer overflow
	
	char* cmd = g_strdup_printf("set_link %s %s",self->qemu_id,link_level[active]);
	gboolean result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	return result;
}
static gboolean gn_vir_node_port_qemu_init(GNVirNodePort *self, GError** error)
{
	char* cmd;
	gboolean result;
	// Create netdev
	cmd = g_strdup_printf("netdev_add vde,id=%s,sock=%s",self->qemu_id,self->hub_sock);
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	if (!result)
		return FALSE;
	// Create device
	cmd = g_strdup_printf("device_add %s,id=%s,netdev=%s,mac=%02x:%02x:%02x:%02x:%02x:%02x",self->qemu_driver,self->qemu_id,self->qemu_id,self->mac[0],self->mac[1],self->mac[2],self->mac[3],self->mac[4],self->mac[5]);
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	if (!result)
		return FALSE;
	// Remove carrier - TODO Do that in a atomic way
	return gn_vir_node_port_qemu_set_carrier(self,FALSE,error);
}

static void gn_vir_node_port_qemu_started(GVirDomain *gvirdomain, GNVirNodePort *self)
{
	g_warning("VM start");
	gn_vir_node_port_qemu_init(self,/* TODO GError** error */NULL);
}

static gboolean gn_vir_node_port_connect_vde2(GNPlug* port, const char* socket_path, int port_no, GError **error)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(port);
	// Add vde_plug
	// TODO port_no
	self->plug = g_subprocess_new(0,error,"dpipe","vde_plug",socket_path,"=","vde_plug",self->hub_sock,NULL);
	if (!self->plug)
		return FALSE;
	// Add carrier
	return gn_vir_node_port_qemu_set_carrier(self,TRUE,error);
}
static void gn_vir_node_port_disconnect(GNPlug *port)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(port);
	// Kill process
	g_subprocess_force_exit(self->hub);
	g_clear_object(&self->hub);
	// Remove carrier
	gn_vir_node_port_qemu_set_carrier(self,FALSE,NULL);
}

// Called when VM start or port is added while VM is running
static void gn_vir_node_port_init(GNVirNodePort *self)
{
	static int next_qemu_id = 0;
	
	// Setup hub
	self->hub_sock = g_dir_make_tmp (NULL,/* TODO GError **error */NULL);
	self->hub = g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE,/* TODO GError **error */NULL,"vde_switch","-x","-s",self->hub_sock,NULL);
	
	self->qemu_id = g_strdup_printf("gn-vde-%d",next_qemu_id++);
	self->qemu_driver = g_strdup("e1000");
	// Set a pseudo-random MAC
	static guint8 rand_mac[3] = {1,0,0};
	if (rand_mac[0] == 255) {
		rand_mac[0] = 0;
		if (rand_mac[1] == 255) {
			rand_mac[1] = 0;
			rand_mac[2]++;
		} else rand_mac[1]++;
	} else rand_mac[0]++;
	
	self->mac[0] = 0x52;
	self->mac[1] = 0x54;
	self->mac[2] = 0x00;
	
	self->mac[3] = rand_mac[2];
	self->mac[4] = rand_mac[1];
	self->mac[5] = rand_mac[0];
}
static void gn_vir_node_port_finalize(GObject *gobject)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(gobject);
	g_free(self->qemu_id);
	g_free(self->qemu_driver);
	G_OBJECT_CLASS(gn_vir_node_port_parent_class)->finalize(gobject);
}
static void gn_vir_node_port_class_init(GNVirNodePortClass *klass)
{
	GNPlugClass  *plugclass = GN_PLUG_CLASS(klass);
	GObjectClass *objclass = G_OBJECT_CLASS(klass);
	
	plugclass->connect_vde2 = gn_vir_node_port_connect_vde2;
	plugclass->disconnect = gn_vir_node_port_disconnect;
	
	//objclass->dispose = gn_vir_node_port_dispose;
	objclass->finalize = gn_vir_node_port_finalize;
}
G_DEFINE_TYPE (GNVirNode,gn_vir_node,GN_TYPE_NODE)

extern GVirConnection *vir_connection; // FIXME Dirty hard-coded reference

static void gn_vir_node_screenshot_to_image_at_scale_async(GNVirNode *self, int width, int height, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	// Get screen_shot
	GVirStream *stream = gvir_connection_get_stream(vir_connection,VIR_STREAM_NONBLOCK);
	char *mime_type = gvir_domain_screenshot(self->domain,stream,0,0,NULL);
	if (!mime_type) {
		g_object_unref(stream);
	}
	g_free(mime_type);
	// Read datas
	GInputStream *input_stream = g_io_stream_get_input_stream(G_IO_STREAM(stream));
	gdk_pixbuf_new_from_stream_at_scale_async(input_stream,width,height,TRUE,cancellable,callback,user_data);
}

enum {
	PROP_NONE,
	PROP_DOMAIN,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};
static void gn_vir_node_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVirNode *self = GN_VIR_NODE(object);
	switch (property_id) {
		case PROP_DOMAIN: {
			self->domain = GVIR_DOMAIN(g_value_dup_object(value));
			// I have *divined* into the code
			GValue domain_handle_value = G_VALUE_INIT;
			g_object_get_property(G_OBJECT(self->domain),"handle",&domain_handle_value);
			self->domain_handle = g_value_get_boxed(&domain_handle_value);
			g_value_unset(&domain_handle_value);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vir_node_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVirNode *self = GN_VIR_NODE(object);
	switch (property_id) {
		case PROP_DOMAIN: {
			g_value_set_object(value,self->domain);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_vir_node_init(GNVirNode *self)
{
	self->ports = g_list_store_new(GN_TYPE_PLUG);
}
static void gn_vir_node_screenshot_ready(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	gtk_image_set_from_pixbuf(GTK_IMAGE(user_data),gdk_pixbuf_new_from_stream_finish(res,NULL));
	g_object_unref(user_data);
}
static gboolean gn_vir_node_query_tooltip(GNNode* node, int x, int y, gboolean keyboard_mode, GtkTooltip *tooltip, GtkWidget *widget)
{
	GNVirNode *self = GN_VIR_NODE(node);
	GtkContainer *box = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_VERTICAL,0));
	gtk_container_add(box,gtk_label_new(gvir_domain_get_name(self->domain)));
	GtkWidget *image = gtk_image_new();
	g_object_ref(image); // FIXME Keep a reference for the gn_vir_node_screenshot_ready. That's dirty
	gn_vir_node_screenshot_to_image_at_scale_async(self,200,200,NULL,gn_vir_node_screenshot_ready,image);
	gtk_container_add(box,image);
	
	GtkWidget *tooltip_widget = GTK_WIDGET(box);
	gtk_widget_show_all(tooltip_widget);
	gtk_tooltip_set_custom(tooltip,tooltip_widget);
	return TRUE;
}

static gboolean gn_vir_node_start(GNNode *node, GError **err)
{
	GNVirNode *self = GN_VIR_NODE(node);
	return gvir_domain_start(self->domain,0,err);
}
static gboolean gn_vir_node_stop(GNNode *node, GError **err)
{
	GNVirNode *self = GN_VIR_NODE(node);
	return gvir_domain_stop(self->domain,0,err);
}
static GVirDomainState gn_vir_node_get_state(GNNode *node)
{
	GNVirNode *self = GN_VIR_NODE(node);
	int state;
	if (virDomainGetState(self->domain_handle,&state,NULL,0))
		return 0;
	else return state;
}

static GListModel *gn_vir_node_query_portlist_model(GNNode* node)
{
	return GN_VIR_NODE(node)->ports;
}
static void gn_vir_node_constructed(GObject *gobject)
{
	GNVirNode *self = GN_VIR_NODE(gobject);
	GNVirNodePort *port = g_object_new(gn_vir_node_port_get_type(),"node",self,NULL);
	g_signal_connect_object(self->domain,"started",G_CALLBACK(gn_vir_node_port_qemu_started),port,0);
	gn_vir_node_port_qemu_init(port,NULL);
	g_list_store_append(self->ports,port);
	G_OBJECT_CLASS(gn_vir_node_parent_class)->constructed(gobject);
}

static void gn_vir_node_dispose(GObject *gobject)
{
	GNVirNode *self = GN_VIR_NODE(gobject);
	g_clear_object(&self->ports);
	G_OBJECT_CLASS(gn_vir_node_parent_class)->dispose(gobject);
}

static void gn_vir_node_finalize(GObject *gobject)
{
	GNVirNode *self = GN_VIR_NODE(gobject);
	GNPlug *plug = GN_PLUG(gobject);
	gn_vir_node_port_disconnect(plug);
	G_OBJECT_CLASS(gn_vir_node_parent_class)->finalize(gobject);
}

static void gn_vir_node_class_init(GNVirNodeClass *klass)
{
	GNNodeClass* node_class = GN_NODE_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	node_class->query_portlist_model = gn_vir_node_query_portlist_model;
	node_class->start = gn_vir_node_start;
	node_class->stop = gn_vir_node_stop;
	node_class->get_state = gn_vir_node_get_state;
	node_class->query_tooltip = gn_vir_node_query_tooltip;
	
	objclass->constructed = gn_vir_node_constructed;
	objclass->get_property = gn_vir_node_get_property;
	objclass->set_property = gn_vir_node_set_property;
	objclass->dispose = gn_vir_node_dispose;
	objclass->finalize = gn_vir_node_finalize;
	
	obj_properties[PROP_DOMAIN] = g_param_spec_object("domain", "libvirt's domain", "Virtual machine domain",
		GVIR_TYPE_DOMAIN,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY/* TODO Settable at runtime because we do errors */);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
}
