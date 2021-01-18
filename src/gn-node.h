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
	// Query a tooltip
	gboolean (*query_tooltip)(GNNode* node, int x, int y, gboolean keyboard_mode, GtkTooltip *tooltip, GtkWidget *widget);
	// Query list of GNPort interface GListModel (must return the same result for the same object)
	GListModel *(*query_portlist_model)(GNNode* node);
	
	// Start the node
	gboolean (*start)(GNNode *node, GError **err);
	// Stop the node
	gboolean (*stop)(GNNode *node, GError **err);
	// Query current state. By default return GVIR_DOMAIN_STATE_NONE that mean N/A
	GVirDomainState (*get_state)(GNNode *node);
	// Render a node, (1×1 rectangle with center at 0×0)
	void (*render)(GNNode* node, cairo_t *cr);
};

GdkPoint *gn_node_position(GNNode* node);
GNNode *gn_port_get_node(GNPort* port);

G_END_DECLS

typedef struct _GNLink {
	GNPort* port_a;
	GNPort* port_b;
} GNLink;
