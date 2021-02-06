#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_VDE_SLIRP gn_vde_slirp_get_type()
G_DECLARE_FINAL_TYPE(GNVDESlirp,gn_vde_slirp,GN,VDE_SLIRP,GNNode)

struct _GNVDESlirp {
	GNNode parent_instance;
	
	GSubprocess *slirp_process;
	GListStore  *ports;
	GNPort *port;
};

#define GN_TYPE_VDE_SLIRP_WIDGET gn_vde_slirp_widget_get_type()
G_DECLARE_FINAL_TYPE(GNVDESlirpWidget,gn_vde_slirp_widget,GN,VDE_SLIRP_WIDGET,GtkGrid)

struct _GNVDESlirpWidget {
	GtkGrid parent_instance;
	
	GNVDESlirp *node;
};

G_END_DECLS
