#include "gn-node.h"
#include <libvirt-gobject/libvirt-gobject.h>
#include <libvirt/libvirt-qemu.h>

G_BEGIN_DECLS

#define GN_TYPE_VIR_NODE gn_vir_node_get_type()
G_DECLARE_FINAL_TYPE(GNVirNode,gn_vir_node,GN,VIR_NODE,GNNode)

struct _GNVirNode {
	GNNode parent_instance;
	
	GVirDomain *domain;
	virDomainPtr domain_handle;
	bool vm_online;
	GListStore *ports;
};

#define GN_TYPE_VIR_NODE_PORT gn_vir_node_port_get_type()
G_DECLARE_FINAL_TYPE(GNVirNodePort,gn_vir_node_port,GN,VIR_NODE_PORT,GNPort)
struct _GNVirNodePort {
	GNPort parent_instance;
	
	char *device;
	char *qemu_id;
	guint8 mac[6];
};

char* gn_vir_node_port_get_mac_address(GNVirNodePort *port);
gboolean gn_vir_node_port_set_mac_address(GNVirNodePort *port, const char* mac);

void gn_vir_node_port_delete(GNVirNodePort *port);
void gn_vir_node_port_add(GNVirNode *node);

#define GN_TYPE_VIR_NODE_WIDGET gn_vir_node_widget_get_type()
G_DECLARE_FINAL_TYPE(GNVirNodeWidget,gn_vir_node_widget,GN,VIR_NODE_WIDGET,GtkBox)

struct _GNVirNodeWidget {
	GtkBox parent_instance;
	
	GNVirNode *node;
	
	// Template widgets
	GtkListBox *ports_listbox;
	GtkButton *port_add_button;
};

#define GN_TYPE_VIR_NODE_WIDGET_PORT_ROW gn_vir_node_widget_get_type()
G_DECLARE_FINAL_TYPE(GNVirNodeWidgetPortRow,gn_vir_node_widget_port_row,GN,VIR_NODE_WIDGET_PORT_ROW,GtkListBoxRow)
struct _GNVirNodeWidgetPortRow {
	GtkListBoxRow parent_instance;
	
	GNVirNodePort *port;
	
	// Template widgets
};

G_END_DECLS
