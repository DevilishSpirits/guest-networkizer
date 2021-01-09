#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_VDE_SWITCH gn_vde_switch_get_type()
G_DECLARE_FINAL_TYPE(GNVDESwitch,gn_vde_switch,GN,VDE_SWITCH,GNNode)

struct _GNVDESwitch {
	GNNode parent_instance;
	
	GSubprocess *switch_process;
	gboolean     switch_process_dead;
	char*        sock_path;
	GListStore  *ports;
	/*
	// Query a tooltip
	gboolean (query_tooltip*)(GNNode* node, int x, int y, gboolean keyboard_mode, GtkTooltip *tooltip, GtkWidget *widget);
	// Query list of GNPort interface GListModel (must return the same result for the same object)
	GListModel *(query_portlist_model*)(GNNode* node);
	*/
};

G_END_DECLS
