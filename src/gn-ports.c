#include "gn-node.h"
#include "gn-net.h"

typedef struct _GNPortPrivate {
	GNPort *link;
	GNNode *node;
	
	char*        hub_sock;
	GSubprocess* hub;
	GSubprocess* plug;
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

const char* gn_port_get_hub_sock(GNPort* port)
{
	GNPortPrivate *priv = gn_port_get_instance_private(port);
	return priv->hub_sock;
}
static void gn_port_init(GNPort *self)
{
	GNPortPrivate *priv = gn_port_get_instance_private(self);
}

static void gn_port_dispose(GObject *gobject)
{
	GNPort *self = GN_PORT(gobject);
	gn_port_set_link(self,NULL,NULL);
	G_OBJECT_CLASS(gn_port_parent_class)->dispose(gobject);
}

static void gn_port_finalize(GObject *gobject)
{
	GNPort *self = GN_PORT(gobject);
	GNPortPrivate *priv = gn_port_get_instance_private(self);
	// Destroy the hub
	if (priv->hub) {
		g_subprocess_force_exit(priv->hub);
		g_subprocess_wait(priv->hub,NULL,NULL);
		g_object_unref(priv->hub);
		g_rmdir(priv->hub_sock);
	}
	g_free(priv->hub_sock);
	G_OBJECT_CLASS(gn_port_parent_class)->finalize(gobject);
}

static void gn_port_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNPort *self = GN_PORT(object);
	GNPortPrivate *priv = gn_port_get_instance_private(self);
	switch (property_id) {
		case PROP_NODE: {
			priv->node = GN_NODE(g_value_get_object(value));
			// Create hub
			priv->hub_sock = gn_node_mkdtemp(priv->node,"port",/* TODO GError **error */NULL);
			priv->hub = g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE,/* TODO GError **error */NULL,"vde_switch","-x","-s",priv->hub_sock,NULL);
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
	klass->set_carrier = (gboolean(*)(GNPort*,GNPort*,gboolean,GError**))gtk_true;
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
	GNPortClass   *port_class = GN_PORT_GET_CLASS(port);
	GNPort        *old_link = priv->link;
	if (priv->link == new_link)
		return TRUE;
	GNPortPrivate *old_priv = old_link ? gn_port_get_instance_private(old_link) : NULL;
	GNPortPrivate *new_priv = new_link ? gn_port_get_instance_private(new_link) : NULL;
	GNPortClass   *old_class = old_link ? GN_PORT_GET_CLASS(old_link) : NULL;
	GNPortClass   *new_class = new_link ? GN_PORT_GET_CLASS(new_link) : NULL;
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
	
	gboolean result = TRUE;
	// Unplug
	if (old_link) {
		old_class->set_carrier(old_link,FALSE,NULL);
		if (old_priv->plug) {
			g_subprocess_force_exit(old_priv->plug);
			g_object_unref(old_priv->plug);
			old_priv->plug = NULL;
		}
		port_class->set_carrier(port,FALSE,NULL);
	}
	if (priv->plug) {
		g_subprocess_force_exit(priv->plug);
		g_object_unref(priv->plug);
		priv->plug = NULL;
	}
	// Replug
	if (new_link) {
		int fds_p2n[2] = {-1,-1};
		int fds_n2p[2] = {-1,-1};
		result = g_unix_open_pipe(fds_p2n,0,error) && g_unix_open_pipe(fds_n2p,0,error)
		&& (priv->plug = gn_port_do_plug(port,fds_n2p[0],fds_p2n[1],error))
		&& (new_priv->plug = gn_port_do_plug(new_link,fds_p2n[0],fds_n2p[1],error))
		;
		close(fds_p2n[0]);
		close(fds_p2n[1]);
		close(fds_n2p[0]);
		close(fds_n2p[1]);
		result &= port_class->set_carrier(port,TRUE,error);
		result &= new_class->set_carrier(new_link,TRUE,error);
	}
	// Fire property changes
	if (old_link) g_object_notify_by_pspec(G_OBJECT(old_link),obj_properties[PROP_LINK]);
	g_object_notify_by_pspec(G_OBJECT(port),obj_properties[PROP_LINK]);
	if (new_link) g_object_notify_by_pspec(G_OBJECT(new_link),obj_properties[PROP_LINK]);
	return TRUE /* TODO result */;
}

GSubprocess* gn_mk_plug(const char* hub_sock, int his_rx, int his_tx, GError **error)
{
	GSubprocessLauncher *launcher = g_subprocess_launcher_new(0);
	g_subprocess_launcher_take_stdin_fd(launcher,his_rx);
	g_subprocess_launcher_take_stdout_fd(launcher,his_tx);
	GSubprocess* process = g_subprocess_launcher_spawn(launcher,error,"vde_plug",hub_sock,NULL);
	g_object_unref(launcher);
	return process;
}
GSubprocess* gn_mk_plug_no(const char* hub_sock, int port_no, int his_rx, int his_tx, GError **error)
{
	GSubprocessLauncher *launcher = g_subprocess_launcher_new(0);
	g_subprocess_launcher_take_stdin_fd(launcher,his_rx);
	g_subprocess_launcher_take_stdout_fd(launcher,his_tx);
	char* port_no_str = g_strdup_printf("%d",port_no);
	GSubprocess* process = g_subprocess_launcher_spawn(launcher,error,"vde_plug","--port",port_no_str,hub_sock,NULL);
	g_free(port_no_str);
	g_object_unref(launcher);
	return process;
}
GSubprocess* gn_port_do_plug(GNPort* port, int his_rx, int his_tx, GError **error)
{
	gn_mk_plug(gn_port_get_hub_sock(port),his_rx,his_tx,error);
}
