#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_VDE_SLIRP gn_vde_slirp_get_type()
G_DECLARE_FINAL_TYPE(GNVDESlirp,gn_vde_slirp,GN,VDE_SLIRP,GNNode)

struct _GNVDESlirp {
	GNNode parent_instance;
	
	GSubprocess *slirp_process;
	GListStore  *ports;
};

G_END_DECLS
