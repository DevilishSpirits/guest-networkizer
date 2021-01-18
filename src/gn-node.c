#include "gn-node.h"
#include "gn-net.h"

typedef struct _GNNodePrivate {
	GdkPoint position;
	GNNet   *net;
} GNNodePrivate;
G_DEFINE_TYPE_WITH_PRIVATE(GNNode,gn_node,G_TYPE_OBJECT)

GdkPoint *gn_node_position(GNNode* node)
{
	GNNodePrivate *priv = gn_node_get_instance_private(node);
	return &priv->position;
}
GNNet *gn_node_get_net(GNNode* node)
{
	GNNodePrivate *priv = gn_node_get_instance_private(node);
	return priv->net;
}

enum {
	PROP_NONE,
	PROP_NET,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

static void gn_node_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNNode *self = GN_NODE(object);
	GNNodePrivate *priv = gn_node_get_instance_private(self);
	switch (property_id) {
		case PROP_NET: {
			priv->net = GN_NET(g_value_get_object(value));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_node_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNNode *self = GN_NODE(object);
	GNNodePrivate *priv = gn_node_get_instance_private(self);
	switch (property_id) {
		case PROP_NET: {
			g_value_set_object(value,priv->net);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_node_init(GNNode *self)
{
}

static void gn_node_dispose(GObject *gobject)
{
	G_OBJECT_CLASS(gn_node_parent_class)->dispose(gobject);
}

static void gn_node_finalize(GObject *gobject)
{
	G_OBJECT_CLASS(gn_node_parent_class)->finalize(gobject);
}

static GVirDomainState gn_node_default_get_state(GNNode* node, char* text)
{
	return GVIR_DOMAIN_STATE_NONE;
}
static void gn_node_default_render(GNNode* node, cairo_t *cr)
{
	// FIXME please just fixme...
	cairo_translate(cr,-.5,.5);
	cairo_set_source_rgb(cr,1,.5,.5);
	cairo_rectangle(cr,0,-1,1,1);
	cairo_fill(cr);
	cairo_set_source_rgb(cr,1,0,0);
	cairo_set_font_size(cr,.25);
	cairo_show_text(cr,G_OBJECT_TYPE_NAME(node));
}

static void gn_node_class_init(GNNodeClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	klass->query_tooltip = gtk_false;
	klass->get_state = gn_node_default_get_state;
	klass->render = gn_node_default_render;
	
	objclass->get_property = gn_node_get_property;
	objclass->set_property = gn_node_set_property;
	objclass->dispose = gn_node_dispose;
	objclass->finalize = gn_node_finalize;
	
	obj_properties[PROP_NET] = g_param_spec_object("net", "Network", "Network",
		GN_TYPE_NET,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
}
