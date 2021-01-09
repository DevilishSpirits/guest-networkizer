#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GN_TYPE_PORT gn_port_get_type()
#define GN_TYPE_PLUG gn_plug_get_type()
#define GN_TYPE_RECEPTACLE gn_receptacle_get_type()
G_DECLARE_DERIVABLE_TYPE(GNPort,gn_port,GN,PORT,GObject)
G_DECLARE_DERIVABLE_TYPE(GNPlug,gn_plug,GN,PLUG,GObject)
G_DECLARE_DERIVABLE_TYPE(GNReceptacle,gn_receptacle,GN,RECEPTACLE,GObject)

struct _GNPortClass {
	GObjectClass parent_class;
	// Query it's name
	const char* (*get_name)(GNPort* port);
	gboolean (*link_change)(GNPort* port, GNPort* old_link, GNPort* new_link, GError **error);
};

GNPort *gn_port_get_link(GNPort* port);
gboolean gn_port_set_link(GNPort* port, GNPort *new_link, GError **error);

struct _GNPlugClass {
	GNPortClass parent_class;
	// Connect a plug to a VDE2 switch. By default fallback to connect_vde4 but still recomended.
	gboolean (*connect_vde2)(GNPlug* port, const char* socket_path, int port_no, GError **error);
	// Connect a plug to a VDE4 URI
	gboolean (*connect_vde4)(GNPlug* port, const char* uri, GError **error);
	// Disconnect the port
	void (*disconnect)(GNPlug* port);
};

struct _GNReceptacleClass {
	GNPortClass parent_class;
	// Return VDE2 socket path (or NULL if none is available)
	const char* (*get_path)(GNReceptacle* port, int *port_no, GError **error);
};

G_END_DECLS
