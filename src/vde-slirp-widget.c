#include "gn-node-panel.h"
#include "vde-slirp.h"

G_DEFINE_TYPE (GNVDESlirpWidget,gn_vde_slirp_widget,GTK_TYPE_GRID)

static void gn_vde_slirp_widget_init(GNVDESlirpWidget *self)
{
	GtkWidget *widget = GTK_WIDGET(self);
	gtk_widget_init_template(widget);
}

enum {
	PROP_NONE,
	PROP_NODE,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

G_MODULE_EXPORT void gn_vde_slirp_widget_reboot(GNVDESlirpWidget *self)
{
	GNNode *node = GN_NODE(self->node);
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	node_class->stop(node,NULL);
	node_class->start(node,NULL);
}
static void gn_vde_slirp_widget_need_reboot_changed(GNVDESlirp* slirp, GParamSpec *pspec, GtkInfoBar *infobar)
{
	gtk_info_bar_set_revealed(infobar,gn_vde_slirp_need_reboot(slirp));
}

G_MODULE_EXPORT void gn_vde_slirp_widget_dns_entry_focus_out(GtkEntry *entry, GdkEvent *event, GNVDESlirpWidget* self)
{
	if (!gn_vde_slirp_set_dns_address(self->node,gtk_entry_get_text(entry))) {
		char* address = g_inet_address_to_string(self->node->config.dns_server);
		gtk_entry_set_text(entry,address);
		g_free(address);
	}
}
G_MODULE_EXPORT void gn_vde_slirp_widget_dns_changed(GtkEntry *entry, GNVDESlirpWidget* self)
{
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(entry));
	if (gn_vde_slirp_set_dns_address(self->node,gtk_entry_get_text(entry)))
		gtk_style_context_remove_class(context,"error");
	else gtk_style_context_add_class(context,"error");
}

static void gn_vde_slirp_widget_set_node(GNVDESlirpWidget *self, GNVDESlirp *node)
{
	self->node = node;
	// Bind properties
	g_object_bind_property(node,"enable-dhcp",self->dhcp_checkbox,"active",G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
	g_object_bind_property(node,"dns-address",self->dns_entry,"text",G_BINDING_SYNC_CREATE);
	
	// Connect signals 
	g_signal_connect(node,"notify::config",G_CALLBACK(gn_vde_slirp_widget_need_reboot_changed),self->need_reboot_infobar);
	g_signal_connect(node,"notify::current-config",G_CALLBACK(gn_vde_slirp_widget_need_reboot_changed),self->need_reboot_infobar);
	
	// Preshot update signals
	gn_vde_slirp_widget_need_reboot_changed(node,NULL,self->need_reboot_infobar);
}
static void gn_vde_slirp_widget_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVDESlirpWidget *self = GN_VDE_SLIRP_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			gn_vde_slirp_widget_set_node(self,GN_VDE_SLIRP(g_value_get_object(value)));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_slirp_widget_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVDESlirpWidget *self = GN_VDE_SLIRP_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			g_value_set_object(value,self->node);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_slirp_widget_class_init(GNVDESlirpWidgetClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	
	objclass->get_property = gn_vde_slirp_widget_get_property;
	objclass->set_property = gn_vde_slirp_widget_set_property;
	
	obj_properties[PROP_NODE] = g_param_spec_object("node", "Node", "Attached node",
	GN_TYPE_VDE_SLIRP,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNVDESlirpWidget");
	gtk_widget_class_bind_template_child(widget_class,GNVDESlirpWidget,need_reboot_infobar);
	gtk_widget_class_bind_template_child(widget_class,GNVDESlirpWidget,dns_entry);
	gtk_widget_class_bind_template_child(widget_class,GNVDESlirpWidget,dhcp_checkbox);
}
