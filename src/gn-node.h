#pragma once
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libvirt-gobject/libvirt-gobject.h>
#include "gn-ports.h"

G_BEGIN_DECLS

#define GN_TYPE_NODE gn_node_get_type()
G_DECLARE_DERIVABLE_TYPE(GNNode,gn_node,GN,NODE,GObject)

struct _GNNodeClass {
	GObjectClass parent_class;
	// Get a name to display
	const char* (*get_label)(GNNode* node);
	// Query a tooltip
	gboolean (*query_tooltip)(GNNode* node, int x, int y, gboolean keyboard_mode, GtkTooltip *tooltip, GtkWidget *widget);
	// Query list of GNPort interface GListModel (must return the same result for the same object)
	GListModel *(*query_portlist_model)(GNNode* node);
	// GtkWidget class for the contextual control panel
	// The 'node' param is set to the instance to bind
	GType widget_control_type;
	
	// Start the node
	gboolean (*start)(GNNode *node, GError **err);
	// Stop the node
	gboolean (*stop)(GNNode *node, GError **err);
	// Query current state. By default return GVIR_DOMAIN_STATE_NONE that mean N/A
	GVirDomainState (*get_state)(GNNode *node);
	// Render a node, (1×1 rectangle with center at 0×0)
	void (*render)(GNNode* node, cairo_t *cr);
	
	// List of properties to read/write in files
	// Array of GParamSpec, the array own a reference to them and can be reffed
	GPtrArray *file_properties;
};

gboolean gn_node_set_state(GNNode* node, GVirDomainState state, GError **error);
const char* gn_node_tmp_dir(GNNode* node, GError **error);
const char* gn_node_mkdtemp(GNNode* node, const char* tmpl, GError **error);

GdkPoint *gn_node_position(GNNode* node);
GNNode *gn_port_get_node(GNPort* port);

void gn_node_notify_label_change(GNNode* node);
void gn_node_notify_state_change(GNNode* node);

#define GN_NODE_DARK_COLOR_RUNNING     .0,.4,.0
#define GN_NODE_DARK_COLOR_BLOCKED     .4,.0,.2
#define GN_NODE_DARK_COLOR_PAUSED      .2,.3,.0
#define GN_NODE_DARK_COLOR_SHUTDOWN    .4,.2,.0
#define GN_NODE_DARK_COLOR_SHUTOFF     .2,.1,.1
#define GN_NODE_DARK_COLOR_CRASHED     .4,.0,.0
#define GN_NODE_DARK_COLOR_PMSUSPENDED .2,.3,.0
#define GN_NODE_DARK_COLOR_DEFAULT     .0,.0,.0

G_END_DECLS

typedef struct _GNLink {
	GNPort* port_a;
	GNPort* port_b;
} GNLink;
