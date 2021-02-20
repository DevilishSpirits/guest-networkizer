#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GN_TYPE_PORT gn_port_get_type()
G_DECLARE_DERIVABLE_TYPE(GNPort,gn_port,GN,PORT,GObject)

struct _GNPortClass {
	GObjectClass parent_class;
	// Query it's name
	char* (*get_name)(GNPort* port);
	// Manually set carrier status
	gboolean (*set_carrier)(GNPort* port, gboolean has_carrier, GError **error);
};

const char* gn_port_get_hub_sock(GNPort* port);
GNPort *gn_port_get_link(GNPort* port);
gboolean gn_port_set_link(GNPort* port, GNPort *new_link, GError **error);

// Helpful utility
/* Create a vde_plug using submited rx/tx file descriptors
 */
GSubprocess* gn_mk_plug(const char* hub_sock, int his_rx, int his_tx, GError **error);
/* Create a vde_plug using submited rx/tx file descriptors
 */
GSubprocess* gn_mk_plug_no(const char* hub_sock, int port_no, int his_rx, int his_tx, GError **error);
/* Like gn_mk_plug directly on a GNPort
 */
GSubprocess* gn_port_do_plug(GNPort* port, int his_rx, int his_tx, GError **error);
