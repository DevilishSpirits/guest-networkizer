#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_VDE_SLIRP gn_vde_slirp_get_type()
G_DECLARE_FINAL_TYPE(GNVDESlirp,gn_vde_slirp,GN,VDE_SLIRP,GNNode)

typedef struct _GNVDESlirpConfig {
	GInetAddress *dns_server;
	bool enable_dhcp;
} GNVDESlirpConfig;
void gn_vde_slirp_config_set_defaults(GNVDESlirpConfig* config);
gboolean gn_vde_slirp_config_equal(const GNVDESlirpConfig* a, const GNVDESlirpConfig* b);
void gn_vde_slirp_config_copy(const GNVDESlirpConfig* from, GNVDESlirpConfig* to);
GNVDESlirpConfig* gn_vde_slirp_config_dup(const GNVDESlirpConfig* config);

GType gn_vde_slirp_config_get_type(void);
#define GN_TYPE_VDE_SLIRP_CONFIG gn_vde_slirp_config_get_type()

struct _GNVDESlirp {
	GNNode parent_instance;
	
	GNVDESlirpConfig config;
	// The config in the running instance
	GNVDESlirpConfig current_config;
	
	GSubprocess *slirp_process;
	GListStore  *ports;
	GNPort *port;
};

// Return TRUE if we need to reboot to take changes in account
gboolean gn_vde_slirp_set_dns_address(GNVDESlirp *slirp, const char* address);
gboolean gn_vde_slirp_need_reboot(const GNVDESlirp* slirp);

#define GN_TYPE_VDE_SLIRP_WIDGET gn_vde_slirp_widget_get_type()
G_DECLARE_FINAL_TYPE(GNVDESlirpWidget,gn_vde_slirp_widget,GN,VDE_SLIRP_WIDGET,GtkGrid)

struct _GNVDESlirpWidget {
	GtkGrid parent_instance;
	
	GNVDESlirp *node;
	
	// Template widgets
	GtkInfoBar *need_reboot_infobar;
	
	GtkEntry *dns_entry;
	GtkToggleButton *dhcp_checkbox;
};

G_END_DECLS
