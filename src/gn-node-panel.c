#include "gn-node-panel.h"
#include "vde-ns.h"

G_DEFINE_TYPE(GNNodePanel,gn_node_panel,GTK_TYPE_BOX)

enum {
	PROP_NONE,
	PROP_NODE,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

static void gn_node_panel_node_state_changed(GNNodePanel *self)
{
	GVirDomainState node_state = self->node_class->get_state(self->node);
	switch (node_state) {
		case GVIR_DOMAIN_STATE_RUNNING:case GVIR_DOMAIN_STATE_BLOCKED :
		case GVIR_DOMAIN_STATE_PAUSED :case GVIR_DOMAIN_STATE_SHUTDOWN:
		case GVIR_DOMAIN_STATE_PMSUSPENDED:
			gtk_switch_set_state(self->onoff_switch,TRUE);break; 
		
		default:
			gtk_switch_set_state(self->onoff_switch,FALSE);break; 
	}
}

static gboolean gn_node_panel_onoff_switch_active_to_state(GBinding *binding, const GValue *from_value, GValue *to_value, gpointer user_data)
{
	if (g_value_get_boolean(from_value))
		g_value_set_enum(to_value,GVIR_DOMAIN_STATE_RUNNING);
	else g_value_set_enum(to_value,GVIR_DOMAIN_STATE_SHUTOFF);
}
static gboolean gn_node_panel_onoff_switch_active_from_state(GBinding *binding, const GValue *from_value, GValue *to_value, gpointer user_data)
{
	switch (g_value_get_enum(from_value)) {
		case GVIR_DOMAIN_STATE_RUNNING:case GVIR_DOMAIN_STATE_BLOCKED :
			g_value_set_boolean(to_value,TRUE);break; 
		
		default:
			g_value_set_boolean(to_value,FALSE);break; 
	}
}

G_MODULE_EXPORT void gn_node_panel_restore(GtkWidget *self)
{
	GtkContainer *parent = GTK_CONTAINER(gtk_widget_get_parent(self));
	// Unparent myself
	g_object_ref(self); // Ensure I'm not killed
	gtk_container_remove(parent,self);
	
	// Put me in a window
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_object_bind_property(GN_NODE_PANEL(self)->node,"label",window,"title",G_BINDING_SYNC_CREATE);
	gtk_container_add(GTK_CONTAINER(window),self);
	g_object_unref(self);
	gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
	gtk_widget_show(window);
	
	// Destroy this button
	gtk_widget_destroy(GN_NODE_PANEL(self)->restore_button);
	
	// Popdown parent
	gtk_popover_popdown(GTK_POPOVER(parent));
}
G_MODULE_EXPORT gboolean gn_node_panel_wireshark(GtkWidget *button, GNNodePanel *self)
{
	 const char *argv[] = {"sh","-c","for i in $(ip l | grep -E '^[0-9]:' | cut -f2 -d':'); do ip l set $i up; done && exec wireshark",NULL};
	GSubprocess *subprocess = gn_vde_ns_subprocess_node(self->node,argv,0,NULL/* TODO GError **error */);
	g_object_unref(subprocess);
}

void gn_node_panel_set_node(GNNodePanel *panel, GNNode *node)
{
	// Cleanups
	if (panel->node) {
		g_object_unref(panel->node);
		g_clear_object(&panel->node_widget);
	}
	
	// Set node
	panel->node = node;
	panel->node_class = GN_NODE_GET_CLASS(node);
	
	// Set the UI
	if (panel->node_class->start || panel->node_class->stop) {
		gtk_widget_set_visible(GTK_WIDGET(panel->onoff_switch),TRUE);
		g_object_bind_property_full(node,"state",panel->onoff_switch,"active",G_BINDING_SYNC_CREATE|G_BINDING_BIDIRECTIONAL,
			gn_node_panel_onoff_switch_active_from_state,gn_node_panel_onoff_switch_active_to_state,NULL,NULL);
	} else
		gtk_widget_set_visible(GTK_WIDGET(panel->onoff_switch),FALSE);
	
	g_signal_connect_object(node,"notify::state",G_CALLBACK(gn_node_panel_node_state_changed),panel,G_CONNECT_SWAPPED);
	gn_node_panel_node_state_changed(panel);
	
	if (panel->node_class->widget_control_type) {
		panel->node_widget = gtk_widget_new(panel->node_class->widget_control_type,"node",node,NULL);
		gtk_container_add(GTK_CONTAINER(panel),panel->node_widget);
	}
}

static void gn_node_panel_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNNodePanel *self = GN_NODE_PANEL(object);
	switch (property_id) {
		case PROP_NODE: {
			gn_node_panel_set_node(self,GN_NODE(g_value_dup_object(value)));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_node_panel_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNNodePanel *self = GN_NODE_PANEL(object);
	switch (property_id) {
		case PROP_NODE: {
			g_value_set_object(value,self->node);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}

static void gn_node_panel_init(GNNodePanel *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));
	
	// Check for vdens debug tools would works
	if (gn_vde_ns_can(NULL)) {
		// TODO Check for Wireshark
	} else {
		gtk_widget_destroy(self->wireshark_button);
	}
}
static void gn_node_panel_dispose(GNNodePanel *self)
{
	if (self->node)
		g_object_unref(self->node);
}
static void gn_node_panel_class_init(GNNodePanelClass *klass)
{
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	objclass->get_property = gn_node_panel_get_property;
	objclass->set_property = gn_node_panel_set_property;
	objclass->dispose = gn_node_panel_dispose;
	
	obj_properties[PROP_NODE] = g_param_spec_object("node", "Node", "Attached node",
	GN_TYPE_NODE,G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNNodePanel");
	gtk_widget_class_bind_template_child(widget_class,GNNodePanel,wireshark_button);
	gtk_widget_class_bind_template_child(widget_class,GNNodePanel,onoff_switch);
	gtk_widget_class_bind_template_child(widget_class,GNNodePanel,restore_button);
}
