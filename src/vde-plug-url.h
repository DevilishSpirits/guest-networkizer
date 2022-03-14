#include "gn-node.h"

G_BEGIN_DECLS

#define GN_TYPE_VDE_PLUG_URL gn_vde_plug_url_get_type()
G_DECLARE_FINAL_TYPE(GNVDEPlugURL,gn_vde_plug_url,GN,VDE_PLUG_URL,GNNode)

struct _GNVDEPlugURL {
	GNNode parent_instance;
	
	char* url;
	// The URL in the running instance
	char* current_url;
	
	char* crash_message;
	
	GSubprocess *plug_process;
	GListStore  *ports;
	GNPort *port;
};

// Return TRUE if we need to reboot to take changes in account
gboolean gn_vde_plug_url_need_reboot(const GNVDEPlugURL* plug);

#define GN_TYPE_VDE_PLUG_URL_WIDGET gn_vde_plug_url_widget_get_type()
G_DECLARE_FINAL_TYPE(GNVDEPlugURLWidget,gn_vde_plug_url_widget,GN,VDE_PLUG_URL_WIDGET,GtkGrid)

struct _GNVDEPlugURLWidget {
	GtkGrid parent_instance;
	
	GNVDEPlugURL *node;
	
	// Template widgets
	GtkInfoBar *crashed_infobar;
	GtkLabel   *crashed_label;
	GtkInfoBar *need_reboot_infobar;
	
	GtkEntry *url_entry;
};

G_END_DECLS
