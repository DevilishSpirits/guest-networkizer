#include "gn-node.h"
#include "gn-net.h"
#include <glib/gstdio.h>

typedef struct _GNNodePrivate {
	GdkPoint position;
	GNNet   *net;
	char*    tmp_dir;
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

const char* gn_node_tmp_dir(GNNode* node, GError **error)
{
	GNNodePrivate *priv = gn_node_get_instance_private(node);
	if (!priv->tmp_dir)
		priv->tmp_dir = g_dir_make_tmp(NULL,error);
	return priv->tmp_dir;
}
const char* gn_node_mkdtemp(GNNode* node, const char* tmpl, GError **error)
{
	const char* tmp_dir = gn_node_tmp_dir(node,error);
	if (!tmp_dir)
		return NULL;
	
	// Build path (<node_tmp>/<tmpl>-XXXXXX)
	const int tmp_dir_len = strlen(tmp_dir);
	const int tmpl_len = strlen(tmpl);
	const char suffix[] = "-XXXXXX\0";
	char* new_dir = malloc(tmp_dir_len+sizeof('/')+tmpl_len+sizeof(suffix));
	// TODO Check for new_dir != NULL
	memcpy(new_dir,tmp_dir,tmp_dir_len);
	new_dir[tmp_dir_len] = '/';
	memcpy(&new_dir[tmp_dir_len+1],tmpl,tmpl_len);
	memcpy(&new_dir[tmp_dir_len+1+tmpl_len],suffix,sizeof(suffix));
	
	char* result = g_mkdtemp(new_dir);
	if (!result) {
		g_set_error(error,G_IO_ERROR,g_io_error_from_errno(errno),"Failed to create temporary directory \"%s\": %s",new_dir,g_strerror(errno));
		g_free(new_dir);
	}
	return result;
}

enum {
	PROP_NONE,
	PROP_NET,
	PROP_LABEL,
	PROP_STATE,
	PROP_XPOS,
	PROP_YPOS,
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
		case PROP_XPOS: {
			priv->position.x = g_value_get_int(value);
		} break;
		case PROP_YPOS: {
			priv->position.y = g_value_get_int(value);
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
		case PROP_XPOS: {
			g_value_set_int(value,priv->position.x);
		} break;
		case PROP_YPOS: {
			g_value_set_int(value,priv->position.y);
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
	GNNode *self = GN_NODE(gobject);
	GNNodePrivate *priv = gn_node_get_instance_private(self);
	// Remove tmp
	if (priv->tmp_dir) {
		g_rmdir(priv->tmp_dir);
		g_free(priv->tmp_dir);
	}
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
	klass->file_load_parser.start_element = (void(*)(GMarkupParseContext*,const gchar*,const gchar**,const gchar**,gpointer,GError**))gn_net_load_skip_parser_push;
		klass->file_load_parser.end_element = (void(*)(GMarkupParseContext*,const gchar*,void*,GError**))g_markup_parse_context_pop;
	
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
	obj_properties[PROP_XPOS] = g_param_spec_int("xpos","X node cordinate","Horizontal position of the node",
		G_MININT,G_MAXINT,0,G_PARAM_READWRITE);
	obj_properties[PROP_YPOS] = g_param_spec_int("ypos","Y node cordinate","Vertical position of the node",
		G_MININT,G_MAXINT,0,G_PARAM_READWRITE);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	klass->file_properties = g_ptr_array_new_with_free_func((GDestroyNotify)g_param_spec_unref);
	g_ptr_array_add(klass->file_properties,obj_properties[PROP_XPOS]);
	g_ptr_array_add(klass->file_properties,obj_properties[PROP_YPOS]);
}
