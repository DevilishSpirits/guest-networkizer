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
	PROP_LABEL,
	PROP_STATE,
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
		case PROP_STATE: {
			gn_node_set_state(self,g_value_get_enum(value),NULL);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_node_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNNode *self = GN_NODE(object);
	GNNodeClass *klass = GN_NODE_GET_CLASS(self);
	GNNodePrivate *priv = gn_node_get_instance_private(self);
	switch (property_id) {
		case PROP_NET: {
			g_value_set_object(value,priv->net);
		} break;
		case PROP_LABEL: {
			g_value_set_string(value,klass->get_label(self));
		} break;
		case PROP_STATE: {
			g_value_set_enum(value,klass->get_state(self));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
void gn_node_notify_label_change(GNNode* node)
{
	g_object_notify_by_pspec(G_OBJECT(node),obj_properties[PROP_LABEL]);
}
void gn_node_notify_state_change(GNNode* node)
{
	g_object_notify_by_pspec(G_OBJECT(node),obj_properties[PROP_STATE]);
}

gboolean gn_node_set_state(GNNode* node, GVirDomainState state, GError **error)
{
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	GVirDomainState current_state = node_class->get_state(node);
	switch (current_state) {
		case GVIR_DOMAIN_STATE_RUNNING:
		case GVIR_DOMAIN_STATE_BLOCKED:
		switch (state) {
			// TODO case GVIR_DOMAIN_STATE_PAUSED
			case GVIR_DOMAIN_STATE_SHUTOFF: // TODO Cleanly stop the node
			case GVIR_DOMAIN_STATE_CRASHED:
				return node_class->stop(node,error);
			// TODO case GVIR_DOMAIN_STATE_PMSUSPENDED
			default:return TRUE;
		} break;
		
		//case GVIR_DOMAIN_STATE_PAUSED
		case GVIR_DOMAIN_STATE_SHUTDOWN:
		switch (state) {
			case GVIR_DOMAIN_STATE_RUNNING: // TODO
			case GVIR_DOMAIN_STATE_PAUSED: //TODO
				return TRUE;
			case GVIR_DOMAIN_STATE_CRASHED:
				return node_class->stop(node,error);
		} break;
		case GVIR_DOMAIN_STATE_SHUTOFF:
		case GVIR_DOMAIN_STATE_CRASHED:
		switch (state) {
			case GVIR_DOMAIN_STATE_RUNNING:
			// TODO case GVIR_DOMAIN_STATE_PAUSED:
				return node_class->start(node,error);
			default:return TRUE;
		} break;
		//case GVIR_DOMAIN_STATE_PMSUSPENDED
		default:return TRUE;break;
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
static const char* gn_node_default_get_label(GNNode* node)
{
	return G_OBJECT_TYPE_NAME(node);
}
static void gn_node_default_render(GNNode* node, cairo_t *cr)
{
	cairo_translate(cr,-.5,.5);
	cairo_set_source_rgb(cr,1,.5,.5);
	cairo_rectangle(cr,0,-1,1,1);
	cairo_fill(cr);
}

static void gn_node_class_init(GNNodeClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	klass->query_tooltip = gtk_false;
	klass->get_label = gn_node_default_get_label;
	klass->get_state = gn_node_default_get_state;
	klass->render = gn_node_default_render;
	
	objclass->get_property = gn_node_get_property;
	objclass->set_property = gn_node_set_property;
	objclass->dispose = gn_node_dispose;
	objclass->finalize = gn_node_finalize;
	
	obj_properties[PROP_NET] = g_param_spec_object("net", "Network", "Network",
		GN_TYPE_NET,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	obj_properties[PROP_LABEL] = g_param_spec_string("label", "Label", "Node label",
		NULL,G_PARAM_READABLE);
	obj_properties[PROP_STATE] = g_param_spec_enum("state", "State", "Node state",
		GVIR_TYPE_DOMAIN_STATE,0,G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
}
