#include "gn-node-panel.h"
#include "vde-plug-url.h"

G_DEFINE_TYPE (GNVDEPlugURLWidget,gn_vde_plug_url_widget,GTK_TYPE_GRID)

static void gn_vde_plug_url_widget_init(GNVDEPlugURLWidget *self)
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

static void gn_vde_plug_url_widget_reveal_changed(GNVDEPlugURL* plug, GParamSpec *pspec, GNVDEPlugURLWidget *self)
{
	gtk_info_bar_set_revealed(self->need_reboot_infobar,gn_vde_plug_url_need_reboot(plug));
	gtk_info_bar_set_revealed(self->crashed_infobar,plug->crash_message != NULL);
}

G_MODULE_EXPORT void gn_vde_plug_url_widget_reboot(GNVDEPlugURLWidget *self)
{
	GNNode *node = GN_NODE(self->node);
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	node_class->stop(node,NULL);
	node_class->start(node,NULL);
	gtk_info_bar_set_revealed(self->need_reboot_infobar,false);
}

static void gn_vde_plug_url_widget_set_node(GNVDEPlugURLWidget *self, GNVDEPlugURL *node)
{
	self->node = node;
	// Bind properties
	g_object_bind_property(node,"url",self->url_entry,"text",G_BINDING_SYNC_CREATE|G_BINDING_BIDIRECTIONAL);
	g_object_bind_property(node,"crashed-message",self->crashed_label,"label",G_BINDING_SYNC_CREATE);
	
	// Connect signals 
	g_signal_connect_object(node,"notify",G_CALLBACK(gn_vde_plug_url_widget_reveal_changed),self,0);
	
	// Preshot update signals
	gn_vde_plug_url_widget_reveal_changed(node,NULL,self);
}
static void gn_vde_plug_url_widget_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVDEPlugURLWidget *self = GN_VDE_PLUG_URL_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			gn_vde_plug_url_widget_set_node(self,GN_VDE_PLUG_URL(g_value_get_object(value)));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_plug_url_widget_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVDEPlugURLWidget *self = GN_VDE_PLUG_URL_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			g_value_set_object(value,self->node);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vde_plug_url_widget_class_init(GNVDEPlugURLWidgetClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	
	objclass->get_property = gn_vde_plug_url_widget_get_property;
	objclass->set_property = gn_vde_plug_url_widget_set_property;
	
	obj_properties[PROP_NODE] = g_param_spec_object("node", "Node", "Attached node",
	GN_TYPE_VDE_PLUG_URL,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNVDEPlugURLWidget");
	gtk_widget_class_bind_template_child(widget_class,GNVDEPlugURLWidget,crashed_label);
	gtk_widget_class_bind_template_child(widget_class,GNVDEPlugURLWidget,crashed_infobar);
	gtk_widget_class_bind_template_child(widget_class,GNVDEPlugURLWidget,need_reboot_infobar);
	gtk_widget_class_bind_template_child(widget_class,GNVDEPlugURLWidget,url_entry);
}
