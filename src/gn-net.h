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

void gn_net_state_all(GNNet *self, GVirDomainState state);

gboolean gn_net_save(GNNet *net, GOutputStream* stream, GCancellable *cancellable, GError **error);
GNNet* gn_net_load(GInputStream *stream, GCancellable *cancellable, GError **error);

void gn_net_load_skip_parser_push(GMarkupParseContext *context);

gboolean gn_net_save_context_write(struct gn_net_save_context* ctx, const void* data, gsize size);
gboolean gn_net_save_context_writev(struct gn_net_save_context* ctx, GOutputVector *vectors, gsize n_vectors);
gboolean gn_net_save_context_dump_object_properties(struct gn_net_save_context* ctx, GObject* object, GParamSpec **param_specs, guint n_properties);
// Magic macros
#define gn_net_save_context_write_static(ctx,chr_array) gn_net_save_context_write(ctx,chr_array,sizeof(chr_array)-1)
#define gn_net_save_context_writev_static(ctx,vectors) gn_net_save_context_writev(ctx,vectors,sizeof(vectors)/sizeof(vectors)[0])

G_END_DECLS
