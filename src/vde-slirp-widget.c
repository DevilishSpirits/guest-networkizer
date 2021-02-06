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

static void gn_vde_slirp_widget_set_node(GNVDESlirpWidget *self, GNVDESlirp *node)
{
	self->node = node;
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
}
