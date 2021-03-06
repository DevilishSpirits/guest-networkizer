#include <gtk/gtk.h>
#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_NODE_PANEL gn_node_panel_get_type ()
G_DECLARE_FINAL_TYPE(GNNodePanel,gn_node_panel,GN,NODE_PANEL,GtkBox)

struct _GNNodePanel {
	GtkBox parent_instance;
	
	GNNode *node;
	GNNodeClass *node_class;
	
	GtkWidget *node_widget;
	// Template widgets
	GtkWidget *wireshark_button;
	GtkSwitch *onoff_switch;
	GtkWidget *restore_button;
};
G_END_DECLS
