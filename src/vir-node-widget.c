#include "gn-node-panel.h"
#include "vir-node.h"

G_DEFINE_TYPE(GNVirNodeWidget,gn_vir_node_widget,GTK_TYPE_BOX)

static void gn_vir_node_widget_init(GNVirNodeWidget *self)
{
	GtkWidget *widget = GTK_WIDGET(self);
	gtk_widget_init_template(widget);
}

enum {
	PROP_NONE,
	PROP_NODE,
	N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {NULL,};

static void gn_vir_node_widget_port_mac_address_focus_out(GtkEntry *entry, GdkEvent *event, GNVirNodePort* port)
{
	if (gn_vir_node_port_set_mac_address(port,gtk_entry_get_text(entry))) {
		char* address = gn_vir_node_port_get_mac_address(port);
		gtk_entry_set_text(entry,address);
		g_free(address);
	}
}
static void gn_vir_node_widget_port_mac_address_changed(GtkEntry *entry, GNVirNodePort* port)
{
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(entry));
	if (gn_vir_node_port_set_mac_address(port,gtk_entry_get_text(entry)))
		gtk_style_context_remove_class(context,"error");
	else gtk_style_context_add_class(context,"error");
}

GtkWidget *gn_vir_node_widget_port_list_box_new_widget(GNVirNodePort *port, GNVirNodeWidget *self)
{
	GtkBox *box = GTK_BOX(gtk_widget_new(GTK_TYPE_BOX,
		"orientation",GTK_ORIENTATION_HORIZONTAL,
		"spacing",8,
		"border-width",4,
	NULL));
	gtk_box_pack_start(box,gtk_image_new_from_icon_name("network-wired",GTK_ICON_SIZE_LARGE_TOOLBAR),TRUE,FALSE,0);
	
	// MAC address entry
	GtkWidget *mac_entry = gtk_widget_new(GTK_TYPE_ENTRY,
		"input-purpose",GTK_INPUT_PURPOSE_ALPHA,
		"input-hints",GTK_INPUT_HINT_NO_SPELLCHECK|GTK_INPUT_HINT_UPPERCASE_CHARS|GTK_INPUT_HINT_NO_EMOJI,
	NULL);
	g_object_bind_property(port,"mac-address",mac_entry,"text",G_BINDING_SYNC_CREATE);
	g_signal_connect_object(mac_entry,"focus-out",G_CALLBACK(gn_vir_node_widget_port_mac_address_focus_out),port,G_CONNECT_SWAPPED);
	g_signal_connect_object(mac_entry,"changed",G_CALLBACK(gn_vir_node_widget_port_mac_address_changed),port,0);
	gtk_box_pack_start(box,mac_entry,TRUE,TRUE,0);
	
	// Device combobox
	GtkWidget *device_combo = gtk_combo_box_text_new_with_entry();
	GtkEntry *device_entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(device_combo)));
	gtk_entry_set_input_hints(device_entry,GTK_INPUT_HINT_NO_SPELLCHECK|GTK_INPUT_HINT_LOWERCASE|GTK_INPUT_HINT_NO_EMOJI);
	g_object_bind_property(port,"device",device_entry,"text",G_BINDING_SYNC_CREATE|G_BINDING_BIDIRECTIONAL);
	gtk_box_pack_start(box,GTK_WIDGET(device_combo),TRUE,TRUE,0);
	
	// Delete button
	GtkWidget *delete_button = gtk_button_new_from_icon_name("edit-delete-symbolic",GTK_ICON_SIZE_BUTTON);
	g_signal_connect_swapped(delete_button,"clicked",G_CALLBACK(gn_vir_node_port_delete),port);
	gtk_box_pack_end(box,delete_button,TRUE,FALSE,0);
	
	// The row !
	GtkWidget *row = gtk_list_box_row_new();
	gtk_container_add(GTK_CONTAINER(row),GTK_WIDGET(box));
	gtk_widget_show_all(row);
	g_object_set_data(G_OBJECT(row),"port",port);
	return row;
}
static void gn_vir_node_widget_set_node(GNVirNodeWidget *self, GNVirNode *node)
{
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	self->node = node;
	
	gtk_list_box_bind_model(self->ports_listbox,node_class->query_portlist_model(GN_NODE(node)),(GtkListBoxCreateWidgetFunc)gn_vir_node_widget_port_list_box_new_widget,self,NULL);
	g_signal_connect_object(self->port_add_button,"clicked",G_CALLBACK(gn_vir_node_port_add),node,G_CONNECT_SWAPPED);
}
static void gn_vir_node_widget_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GNVirNodeWidget *self = GN_VIR_NODE_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			gn_vir_node_widget_set_node(self,GN_VIR_NODE(g_value_get_object(value)));
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vir_node_widget_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GNVirNodeWidget *self = GN_VIR_NODE_WIDGET(object);
	switch (property_id) {
		case PROP_NODE: {
			g_value_set_object(value,self->node);
		} break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
	}
}
static void gn_vir_node_widget_class_init(GNVirNodeWidgetClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	
	objclass->get_property = gn_vir_node_widget_get_property;
	objclass->set_property = gn_vir_node_widget_set_property;
	
	obj_properties[PROP_NODE] = g_param_spec_object("node", "Node", "Attached node",
	GN_TYPE_VIR_NODE,G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(objclass,N_PROPERTIES,obj_properties);
	
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNVirNodeWidget");
	gtk_widget_class_bind_template_child(widget_class,GNVirNodeWidget,ports_listbox);
	gtk_widget_class_bind_template_child(widget_class,GNVirNodeWidget,port_add_button);
}
