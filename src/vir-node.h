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

G_END_DECLS
