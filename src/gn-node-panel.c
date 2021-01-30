#include "gn-node-panel.h"

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
		case GVIR_DOMAIN_STATE_NONE: {
		} break;
		case GVIR_DOMAIN_STATE_RUNNING:
		case GVIR_DOMAIN_STATE_BLOCKED:
		case GVIR_DOMAIN_STATE_PAUSED:
		case GVIR_DOMAIN_STATE_PMSUSPENDED:
		case GVIR_DOMAIN_STATE_SHUTDOWN: {
			// Machine is on
			gtk_switch_set_active(self->onoff_switch,TRUE);
		} break;
		case GVIR_DOMAIN_STATE_SHUTOFF:
		case GVIR_DOMAIN_STATE_CRASHED: {
			// Machine is off
			gtk_switch_set_active(self->onoff_switch,FALSE);
		} break;
	}
}
// FIXME This is drafty and temporary
G_MODULE_EXPORT gboolean gn_node_panel_onoff_switch_state_set(GtkToggleButton *toggle_button, gboolean state, GNNodePanel *self)
{
	GNNode *node = self->node;
	GNNodeClass *node_class = self->node_class;
	GVirDomainState node_state = node_class->get_state(node);
	if (state)
		switch (node_state) {
			case GVIR_DOMAIN_STATE_NONE: {
			} break;
			case GVIR_DOMAIN_STATE_RUNNING:
			case GVIR_DOMAIN_STATE_BLOCKED: {
				// Do nothing. VM is already running
			} break;
			//
			//case GVIR_DOMAIN_STATE_PAUSED
			//case GVIR_DOMAIN_STATE_SHUTDOWN
			case GVIR_DOMAIN_STATE_SHUTOFF:
			case GVIR_DOMAIN_STATE_CRASHED: {
				// Do nothing. VM is already stopped
				node_class->start(node,NULL);
			} break;
			//case GVIR_DOMAIN_STATE_PMSUSPENDED
		}
	else switch (node_state) {
			case GVIR_DOMAIN_STATE_NONE: {
			} break;
			case GVIR_DOMAIN_STATE_RUNNING:
			case GVIR_DOMAIN_STATE_BLOCKED: {
				node_class->stop(node,NULL);
			} break;
			//
			//case GVIR_DOMAIN_STATE_PAUSED
			//case GVIR_DOMAIN_STATE_SHUTDOWN
			case GVIR_DOMAIN_STATE_SHUTOFF:
			case GVIR_DOMAIN_STATE_CRASHED: {
				// Do nothing. VM is already stopped
			} break;
			//case GVIR_DOMAIN_STATE_PMSUSPENDED
		}
	return FALSE;
}

void gn_node_panel_set_node(GNNodePanel *panel, GNNode *node)
{
	g_clear_object(&panel->node);
	panel->node = node;
	panel->node_class = GN_NODE_GET_CLASS(node);
	
	gtk_widget_set_visible(GTK_WIDGET(panel->onoff_switch),panel->node_class->start || panel->node_class->stop);
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
}
static void gn_node_panel_dispose(GNNodePanel *self)
{
	g_clear_object(&self->node);
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
	gtk_widget_class_bind_template_child(widget_class,GNNodePanel,onoff_switch);
}