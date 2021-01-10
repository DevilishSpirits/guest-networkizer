#include "gn-node.h"
#include "gn-net.h"

typedef struct _GNPortPrivate {
	GNPort *link;
	GNNode *node;
} GNPortPrivate;
G_DEFINE_TYPE_WITH_PRIVATE(GNPort,gn_port,G_TYPE_OBJECT)

enum {
	PROP_NONE,
	PROP_NODE,
	PROP_LINK,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

static const char* gn_port_default_get_name(GNPort* port)
{
	static const char* default_name = "Unamed";
	return default_name;
}

static void gn_port_init(GNPort *self)
{
}

static void gn_port_dispose(GObject *gobject)
{
	GNPort *self = GN_PORT(gobject);
	gn_port_set_link(self,NULL,NULL);
	G_OBJECT_CLASS(gn_port_parent_class)->dispose(gobject);
}

static void gn_port_finalize(GObject *gobject)
{
	G_OBJECT_CLASS(gn_port_parent_class)->finalize(gobject);
}

static void gn_port_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNPort *self = GN_PORT(object);
	GNPortPrivate *priv = gn_port_get_instance_private(self);
	switch (property_id) {
		case PROP_NODE: {
			priv->node = GN_NODE(g_value_get_object(value));
		} break;
		case PROP_LINK: {
			gn_port_set_link(self,g_value_get_object(value),NULL);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_port_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNPort *self = GN_PORT(object);
	GNPortPrivate *priv = gn_port_get_instance_private(self);
	switch (property_id) {
		case PROP_NODE: {
			g_value_set_object(value,priv->node);
		} break;
		case PROP_LINK: {
			g_value_set_object(value,priv->link);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_port_class_init(GNPortClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	klass->get_name = gn_port_default_get_name;
	klass->link_change = (gboolean(*)(GNPort*,GNPort*,GNPort*,GError**))gtk_false;
	
	objclass->get_property = gn_port_get_property;
	objclass->set_property = gn_port_set_property;
	objclass->dispose = gn_port_dispose;
	objclass->finalize = gn_port_finalize;
	
	obj_properties[PROP_NODE] = g_param_spec_object("node", "Node", "Port owner",
		GN_TYPE_NODE,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	obj_properties[PROP_LINK] = g_param_spec_object("link", "Link", "Linked port",
		GN_TYPE_PORT,G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
}

static int gn_port_get_index(GNPort *self, GArray *net_link)
{
	int i;
	for (i = net_link->len - 1; i >= 0; i--)
		if ((g_array_index(net_link,GNLink,i).port_a == self) || (g_array_index(net_link,GNLink,i).port_b == self))
			break;
	return i;
}

GNNode *gn_port_get_node(GNPort* port)
{
	GNPortPrivate *priv = gn_port_get_instance_private(port);
	return priv->node;
}
GNPort *gn_port_get_link(GNPort* port)
{
	GNPortPrivate *priv = gn_port_get_instance_private(port);
	return priv->link;
}
gboolean gn_port_set_link(GNPort* port, GNPort *new_link, GError **error)
{
	GNPortPrivate *priv     = gn_port_get_instance_private(port);
	GNPort        *old_link = priv->link;
	if (priv->link == new_link)
		return TRUE;
	GNPortPrivate *old_priv = old_link ? gn_port_get_instance_private(old_link) : NULL;
	GNPortPrivate *new_priv = new_link ? gn_port_get_instance_private(new_link) : NULL;
	GNPort        *new_link_old = new_priv ? new_priv->link : NULL;
	GArray        *net_link = gn_node_get_net(priv->node)->links;
	
	// Update links
	priv->link = new_link;
	if (old_link && !new_link) {
		// Link deleted
		int index = gn_port_get_index(port,net_link);
		g_array_remove_index_fast(net_link,index);
		
		old_priv->link = NULL;
	} else if (!old_link && new_link) {
		// Link created
		GNLink link = {port,new_link};
		g_array_append_val(net_link,link);
		
		new_priv->link = port;
	} else {
		// Link moved
		GNLink *link = &g_array_index(net_link,GNLink,gn_port_get_index(port,net_link));
		if (link->port_a == port)
			link->port_b = new_link;
		else link->port_a = new_link;
		
		old_priv->link = NULL;
		new_priv->link = port;
	}
	
	// Trigger "link_change"
	gboolean result = TRUE;
	if (old_link) result &= GN_PORT_GET_CLASS(old_link)->link_change(old_link,port,NULL,error);
	result &= GN_PORT_GET_CLASS(port)->link_change(port,old_link,new_link,error);
	if (new_link) result &= GN_PORT_GET_CLASS(new_link)->link_change(new_link,new_link_old,port,error);
	// Fire property changes
	if (old_link) g_object_notify_by_pspec(G_OBJECT(old_link),obj_properties[PROP_LINK]);
	g_object_notify_by_pspec(G_OBJECT(port),obj_properties[PROP_LINK]);
	if (new_link) g_object_notify_by_pspec(G_OBJECT(new_link),obj_properties[PROP_LINK]);
	return TRUE /* TODO result */;
}

G_DEFINE_TYPE(GNPlug,gn_plug,GN_TYPE_PORT)

gboolean gn_plug_link_change(GNPort* port, GNPort* old_link, GNPort* new_link, GError **error)
{
	GNPlug *self = GN_PLUG(port);
	GNPlugClass *klass = GN_PLUG_GET_CLASS(self);
	if (old_link)
		klass->disconnect(self);
	if (new_link) {
		GNReceptacle *receptacle = GN_RECEPTACLE(new_link);
		GNReceptacleClass *receptacle_class = GN_RECEPTACLE_GET_CLASS(receptacle);
		int port_no;
		const char* sock_path = receptacle_class->get_path(receptacle,&port_no,error);
		klass->connect_vde2(self,sock_path,port_no,NULL);
	}
	return FALSE;
}

static void gn_plug_init(GNPlug *self)
{
}

static void gn_plug_dispose(GObject *gobject)
{
	G_OBJECT_CLASS(gn_plug_parent_class)->dispose(gobject);
}

static void gn_plug_finalize(GObject *gobject)
{
	G_OBJECT_CLASS(gn_plug_parent_class)->finalize(gobject);
}


gboolean gn_plug_default_vde2(GNPlug* port, const char* socket_path, int port_no, GError **error)
{
	GNPlugClass *klass = GN_PLUG_CLASS(G_OBJECT_GET_CLASS(port));
	if (klass->connect_vde4) {
		gchar *uri = g_strdup_printf("vde://%s[%i]",socket_path,port_no);
		gboolean result = klass->connect_vde4(port,uri,error);
		g_free(uri);
		return result;
	} else {
		g_critical("%s must implement connect_vde2 or connect_vde4.",G_OBJECT_TYPE_NAME(port));
		g_set_error(error,g_quark_from_string("gn-dummy-domain"),1,"%s must implement connect_vde2 or connect_vde4.",G_OBJECT_TYPE_NAME(port));
		return FALSE;
	}
}

static void gn_plug_class_init(GNPlugClass *klass)
{
	GNPortClass* port_class = GN_PORT_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	klass->connect_vde2 = gn_plug_default_vde2;
	
	port_class->link_change = gn_plug_link_change;
	
	objclass->dispose = gn_plug_dispose;
	objclass->finalize = gn_plug_finalize;
}


G_DEFINE_TYPE(GNReceptacle,gn_receptacle,GN_TYPE_PORT)

static void gn_receptacle_init(GNReceptacle *self)
{
}

static void gn_receptacle_dispose(GObject *gobject)
{
	G_OBJECT_CLASS(gn_receptacle_parent_class)->dispose(gobject);
}

static void gn_receptacle_finalize(GObject *gobject)
{
	G_OBJECT_CLASS(gn_receptacle_parent_class)->finalize(gobject);
}

static void gn_receptacle_class_init(GNReceptacleClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	objclass->dispose = gn_receptacle_dispose;
	objclass->finalize = gn_receptacle_finalize;
}
