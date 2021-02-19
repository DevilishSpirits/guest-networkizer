#include <gtk/gtk.h>
#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_NET gn_net_get_type ()
G_DECLARE_FINAL_TYPE(GNNet,gn_net,GN,NET,GObject)

typedef enum _GNNetObjectType {
	GN_NET_NONE,
	GN_NET_NODE,
	GN_NET_LINK,
	GN_NET_OBJECT_TYPE_N
} GNNetObjectType;
typedef union _GNNetObject {
	GNLink *link;
	GNNode *node;
} GNNetObject;

struct _GNNet
{
	GObject parent_instance;
	
	GArray    *links;
	GPtrArray *nodes;
};

GNNet *gn_node_get_net(GNNode* node);

void gn_net_render(GNNet *self, cairo_t *cr, GtkStyleContext *style_context);
GNNetObjectType gn_net_whats_here(GNNet *self, GNNetObject *results, gdouble x, gdouble y);
gboolean gn_net_save(GNNet *net, GOutputStream* stream, GCancellable *cancellable, GError **error);

void gn_net_state_all(GNNet *self, GVirDomainState state);

G_END_DECLS
