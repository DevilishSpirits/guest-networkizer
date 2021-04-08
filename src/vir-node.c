#include "vir-node.h"
#include "gn-net.h"

G_DEFINE_TYPE (GNVirNodePort,gn_vir_node_port,GN_TYPE_PORT)

static GVirDomainState gn_vir_node_get_state(GNNode *node);
enum {
	PORT_PROP_NONE,
	PORT_PROP_DEVICE,
	PORT_PROP_MAC_ADDRESS,
	PORT_N_PROPERTIES
};
static GParamSpec *port_obj_properties[PORT_N_PROPERTIES] = {NULL,};
char* gn_vir_node_port_get_mac_address(GNVirNodePort *port)
{
	return g_strdup_printf("%02x:%02x:%02x:%02x:%02x:%02x",port->mac[0],port->mac[1],port->mac[2],port->mac[3],port->mac[4],port->mac[5]);
}
static guint8 gn_vir_node_port_read_mac_address_digit(char digit)
{
	if ((digit >= '0')&&(digit <= '9'))
		return digit - '0';
	else if ((digit >= 'a')&&(digit <= 'f'))
		return digit - 'a' + 0xa;
	else if ((digit >= 'A')&&(digit <= 'F'))
		return digit - 'A' + 0xA;
	else return 0xFF; // Bad char
}
gboolean gn_vir_node_port_set_mac_address(GNVirNodePort *port, const char* mac)
{
	guint8 new_mac[6];
	for (int i = 0; i < 6; i++) {
		// Check and preprocess digits
		char high_digit = gn_vir_node_port_read_mac_address_digit(mac[3*i+0]);
		if (high_digit == 0xff)
			return FALSE;
		char low_digit = gn_vir_node_port_read_mac_address_digit(mac[3*i+1]);
		if (low_digit == 0xff)
			return FALSE;
		if ((mac[3*i+2] != (i == 5 ? '\0' : ':')))
			return FALSE;
		// Field is okay, write it
		new_mac[i] = (high_digit << 4) | low_digit;
	}
	memcpy(&port->mac,&new_mac,sizeof(new_mac));
	g_object_notify_by_pspec(G_OBJECT(port),port_obj_properties[PORT_PROP_MAC_ADDRESS]);
	return TRUE;
}
static void gn_vir_node_port_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(object);
	switch (property_id) {
		case PORT_PROP_DEVICE: {
			g_free(self->device);
			self->device = g_value_dup_string(value);
		} break;
		case PORT_PROP_MAC_ADDRESS: {
			gn_vir_node_port_set_mac_address(self,g_value_get_string(value));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vir_node_port_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(object);
	switch (property_id) {
		case PORT_PROP_DEVICE: {
			g_value_set_string(value,self->device);
		} break;
		case PORT_PROP_MAC_ADDRESS: {
			g_value_take_string(value,gn_vir_node_port_get_mac_address(self));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

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
	if (warning)
		g_warning(gn_vir_node_perform_qemu_debug_fmtnargs);
	else g_debug(gn_vir_node_perform_qemu_debug_fmtnargs);
	g_free(dummy_out);
	return TRUE;
}

static gboolean gn_vir_node_port_qemu_set_carrier(GNPort *port, gboolean active, GError** error)
{
	GNVirNodePort *self = GN_VIR_NODE_PORT(port);
	switch (gn_vir_node_get_state(gn_port_get_node(GN_PORT(self)))) {
		case GVIR_DOMAIN_STATE_CRASHED:
		case GVIR_DOMAIN_STATE_SHUTOFF:
			return TRUE;
		default:break;
	}
	static const char* link_level[2] = {"off","on"};
	active = active != FALSE; // Avoid buffer overflow
	
	char* cmd = g_strdup_printf("set_link %s %s",self->qemu_id,link_level[active]);
	gboolean result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	return result;
}
static gboolean gn_vir_node_port_qemu_init(GNVirNodePort *self, GError** error)
{
	switch (gn_vir_node_get_state(gn_port_get_node(GN_PORT(self)))) {
		case GVIR_DOMAIN_STATE_CRASHED:
		case GVIR_DOMAIN_STATE_SHUTOFF:
			return TRUE;
		default:break;
	}
	GNPort *port = GN_PORT(self);
	char* cmd;
	gboolean result;
	// Create netdev
	cmd = g_strdup_printf("netdev_add vde,id=%s,sock=%s",self->qemu_id,gn_port_get_hub_sock(port));
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	if (!result)
		return FALSE;
	// Create device
	cmd = g_strdup_printf("device_add %s,id=%s,netdev=%s,mac=%02x:%02x:%02x:%02x:%02x:%02x",self->device,self->qemu_id,self->qemu_id,self->mac[0],self->mac[1],self->mac[2],self->mac[3],self->mac[4],self->mac[5]);
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	if (!result)
		return FALSE;
	if (gn_port_get_link(port))
		return TRUE; // Already has link
	// Remove carrier - TODO Do that in a atomic way
	return gn_vir_node_port_qemu_set_carrier(port,FALSE,error);
}
static gboolean gn_vir_node_port_qemu_clean(GNVirNodePort *self, GError** error)
{
	char* cmd;
	gboolean result;
	// Create netdev
	cmd = g_strdup_printf("netdev_del %s",self->qemu_id);
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	if (!result)
		return FALSE;
	// Create device
	cmd = g_strdup_printf("device_del %s",self->qemu_id);
	result = gn_vir_node_perform_qemu(self,cmd,NULL,error);
	g_free(cmd);
	return result;
}

static void gn_vir_node_port_qemu_started(GVirDomain *gvirdomain, GNVirNodePort *self)
{
	gn_vir_node_port_qemu_init(self,/* TODO GError** error */NULL);
}

// Called when VM start or port is added while VM is running
static void gn_vir_node_port_init(GNVirNodePort *self)
{
	static int next_qemu_id = 0;
	static int instance_id = 0;
	// FIXME Use a better global
	if (!instance_id)
		instance_id = abs(g_get_monotonic_time());
	
	self->qemu_id = g_strdup_printf("gn-vde-%d-%d",instance_id,next_qemu_id++);
	self->device = g_value_dup_string(g_param_spec_get_default_value(port_obj_properties[PORT_PROP_DEVICE]));
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
	gn_vir_node_port_qemu_clean(self,NULL);
	g_free(self->qemu_id);
	g_free(self->device);
	G_OBJECT_CLASS(gn_vir_node_port_parent_class)->finalize(gobject);
}
static void gn_vir_node_port_class_init(GNVirNodePortClass *klass)
{
	GNPortClass  *port_class = GN_PORT_CLASS(klass);
	GObjectClass *objclass = G_OBJECT_CLASS(klass);
	
	port_class->get_name = (char*(*)(GNPort*))gn_vir_node_port_get_mac_address;
	port_class->set_carrier = gn_vir_node_port_qemu_set_carrier;
	
	objclass->get_property = gn_vir_node_port_get_property;
	objclass->set_property = gn_vir_node_port_set_property;
	//objclass->dispose = gn_vir_node_port_dispose;
	objclass->finalize = gn_vir_node_port_finalize;
	
	port_obj_properties[PORT_PROP_DEVICE] = g_param_spec_string("device", "Device model", "NIC model name",
		"virtio-net-pci",G_PARAM_READWRITE);
	port_obj_properties[PORT_PROP_MAC_ADDRESS] = g_param_spec_string("mac-address", "MAC address", "NIC MAC address",
		NULL,G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
	g_object_class_install_properties(objclass,PORT_N_PROPERTIES,port_obj_properties);
}
G_DEFINE_TYPE (GNVirNode,gn_vir_node,GN_TYPE_NODE)

extern GVirConnection *vir_connection; // FIXME Dirty hard-coded reference

void gn_vir_node_port_delete(GNVirNodePort *port)
{
	GListStore *store = GN_VIR_NODE(gn_port_get_node(GN_PORT(port)))->ports;
	guint position;
	if (g_list_store_find(store,port,&position))
		g_list_store_remove(store,position);
}
void gn_vir_node_port_add(GNVirNode *self)
{
	GNVirNodePort *port = g_object_new(GN_TYPE_VIR_NODE_PORT,"node",self,NULL);
	g_signal_connect_object(self->domain,"started",G_CALLBACK(gn_vir_node_port_qemu_started),port,0);
	gn_vir_node_port_qemu_init(port,NULL);
	g_list_store_append(self->ports,port);
	g_object_unref(port);
}

static void gn_vir_node_screenshot_to_image_at_scale_async(GNVirNode *self, int width, int height, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	// TODO Ensure the domain is running
	// Get screen_shot
	GVirStream *stream = gvir_connection_get_stream(vir_connection,VIR_STREAM_NONBLOCK);
	char *mime_type = gvir_domain_screenshot(self->domain,stream,0,0,NULL);
	if (!mime_type) {
		g_object_unref(stream);
		// TODO Load a dummy icon
		return;
	}
	g_free(mime_type);
	// Read datas
	GInputStream *input_stream = g_io_stream_get_input_stream(G_IO_STREAM(stream));
	gdk_pixbuf_new_from_stream_at_scale_async(input_stream,width,height,TRUE,cancellable,callback,user_data);
}

enum {
	PROP_NONE,
	PROP_DOMAIN,
	PROP_DOMAIN_UUID,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

void gn_vir_node_set_domain(GNVirNode *self, GVirDomain *domain)
{
	self->domain = g_object_ref(domain);
	// I have *divined* into the code
	GValue domain_handle_value = G_VALUE_INIT;
	g_object_get_property(G_OBJECT(self->domain),"handle",&domain_handle_value);
	self->domain_handle = g_value_get_boxed(&domain_handle_value);
	g_value_unset(&domain_handle_value);
	// Connect to state change notifications
	g_signal_connect_object(self->domain,"pmsuspended",G_CALLBACK(gn_node_notify_state_change),self,G_CONNECT_SWAPPED);
	g_signal_connect_object(self->domain,"resumed"    ,G_CALLBACK(gn_node_notify_state_change),self,G_CONNECT_SWAPPED);
	g_signal_connect_object(self->domain,"started"    ,G_CALLBACK(gn_node_notify_state_change),self,G_CONNECT_SWAPPED);
	g_signal_connect_object(self->domain,"stopped"    ,G_CALLBACK(gn_node_notify_state_change),self,G_CONNECT_SWAPPED);
	g_signal_connect_object(self->domain,"suspended"  ,G_CALLBACK(gn_node_notify_state_change),self,G_CONNECT_SWAPPED);
}
static void gn_vir_node_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVirNode *self = GN_VIR_NODE(object);
	switch (property_id) {
		case PROP_DOMAIN: {
			GVirDomain *domain = GVIR_DOMAIN(g_value_get_object(value));
			if (domain)
				gn_vir_node_set_domain(self,domain);
		} break;
		case PROP_DOMAIN_UUID: {
			const char* str = g_value_get_string(value);
			if (str) {
				GVirDomain *domain = gvir_connection_get_domain(vir_connection,str);
				gn_vir_node_set_domain(self,domain);
				g_object_unref(domain);
			}
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_vir_node_render(GNNode* node, cairo_t *cr)
{
	// Get real pixel size
	double picx = 1;
	double picy = 0;
	cairo_user_to_device_distance(cr,&picx,&picy);
	// Render computer screen
	// TODO Cache icons
	GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),"computer",picx,0);
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

static void gn_vir_node_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVirNode *self = GN_VIR_NODE(object);
	switch (property_id) {
		case PROP_DOMAIN: {
			g_value_set_object(value,self->domain);
		} break;
		case PROP_DOMAIN_UUID: {
			g_value_set_string(value,gvir_domain_get_uuid(self->domain));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_vir_node_init(GNVirNode *self)
{
	self->ports = g_list_store_new(GN_TYPE_VIR_NODE_PORT);
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
	return gvir_domain_shutdown(self->domain,0,err);
}
static gboolean gn_vir_node_force_stop(GNNode *node, GError **err)
{
	GNVirNode *self = GN_VIR_NODE(node);
	return gvir_domain_stop(self->domain,0,err);
}
static const char* gn_vir_node_get_label(GNNode *node)
{
	GNVirNode *self = GN_VIR_NODE(node);
	return gvir_domain_get_name(self->domain);
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
	return G_LIST_MODEL(GN_VIR_NODE(node)->ports);
}


static void gn_vir_node_file_load_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	g_warning("load start");
	GNVirNode *self = GN_VIR_NODE(user_data);
	if (g_str_equal(element_name,"nic")) {
		char* mac_str;
		char* device_str;
		if (g_markup_collect_attributes(element_name,attribute_names,attribute_values,error,
		 G_MARKUP_COLLECT_STRING,"mac",&mac_str,
		 G_MARKUP_COLLECT_STRING,"device",&device_str,
		 G_MARKUP_COLLECT_INVALID)) {
			// FIXME This is bad code duplication
			GNVirNodePort *port = g_object_new(GN_TYPE_VIR_NODE_PORT,"node",self,"mac-address",mac_str,"device",device_str,NULL);
			g_signal_connect_object(self->domain,"started",G_CALLBACK(gn_vir_node_port_qemu_started),port,0);
			gn_vir_node_port_qemu_init(port,NULL);
			g_list_store_append(self->ports,port);
			g_object_unref(port);
		}
	}
	gn_net_load_skip_parser_push(context);
}
static gboolean gn_vir_node_file_save(GNNode *node, struct gn_net_save_context* ctx)
{
	GNVirNode *self = GN_VIR_NODE(node);
	GListModel *model = G_LIST_MODEL(self->ports);
	guint n_items = g_list_model_get_n_items(model);
	for (guint i = 0; i < n_items; i++) {
		GNVirNodePort *port = GN_VIR_NODE_PORT(g_list_model_get_item(model,i));
		gchar *espaced_device = g_markup_escape_text(port->device,-1);
		g_output_stream_printf(ctx->stream,NULL,ctx->cancellable,ctx->error,"	<nic mac=\"%02x:%02x:%02x:%02x:%02x:%02x\" device=\"%s\"/>\n",port->mac[0],port->mac[1],port->mac[2],port->mac[3],port->mac[4],port->mac[5],espaced_device);
		g_free(espaced_device);
	}
	return TRUE;
}

static void gn_vir_node_constructed(GObject *gobject)
{
	G_OBJECT_CLASS(gn_vir_node_parent_class)->constructed(gobject);
}

static void gn_vir_node_dispose(GObject *gobject)
{
	GNVirNode *self = GN_VIR_NODE(gobject);
	g_list_store_remove_all(self->ports);
	G_OBJECT_CLASS(gn_vir_node_parent_class)->dispose(gobject);
}

static void gn_vir_node_finalize(GObject *gobject)
{
	GNVirNode *self = GN_VIR_NODE(gobject);
	g_clear_object(&self->ports);
	G_OBJECT_CLASS(gn_vir_node_parent_class)->finalize(gobject);
}

static void gn_vir_node_class_init(GNVirNodeClass *klass)
{
	GNNodeClass* node_class = GN_NODE_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	node_class->query_portlist_model = gn_vir_node_query_portlist_model;
	node_class->start = gn_vir_node_start;
	node_class->stop = gn_vir_node_stop;
	node_class->force_stop = gn_vir_node_force_stop;
	node_class->render = gn_vir_node_render;
	node_class->get_label = gn_vir_node_get_label;
	node_class->get_state = gn_vir_node_get_state;
	node_class->query_tooltip = gn_vir_node_query_tooltip;
	node_class->widget_control_type = GN_TYPE_VIR_NODE_WIDGET;
	node_class->file_load_parser.start_element = gn_vir_node_file_load_start_element;
	node_class->file_save = gn_vir_node_file_save;
	
	objclass->constructed = gn_vir_node_constructed;
	objclass->get_property = gn_vir_node_get_property;
	objclass->set_property = gn_vir_node_set_property;
	objclass->dispose = gn_vir_node_dispose;
	objclass->finalize = gn_vir_node_finalize;
	
	obj_properties[PROP_DOMAIN] = g_param_spec_object("domain", "libvirt's domain", "Virtual machine domain",
		GVIR_TYPE_DOMAIN,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY/* TODO Settable at runtime because we do errors */);
	obj_properties[PROP_DOMAIN_UUID] = g_param_spec_string("domain-uuid", "libvirt's domain UUID", "Virtual machine domain UUID",
		NULL,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY/* TODO Settable at runtime because we do errors */);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	node_class->file_properties = g_ptr_array_copy(node_class->file_properties,(GCopyFunc)g_param_spec_ref,NULL);
	g_ptr_array_add(node_class->file_properties,g_param_spec_ref(obj_properties[PROP_DOMAIN_UUID]));
}
