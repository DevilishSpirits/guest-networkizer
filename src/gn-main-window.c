#include "gn-main-window.h"
#include "vde-switch.h"
#include "vir-node.h"
#include <math.h>

G_DEFINE_TYPE (GNMainWindow,gn_main_window,GTK_TYPE_APPLICATION_WINDOW)

#define WORKSPACE_MOUSE(axis) (event->axis / view_scale)

static void gn_main_window_init(GNMainWindow *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));
	
	self->net = GN_NET(g_object_new(GN_TYPE_NET,NULL));
	
	self->new_node_properties_names  = g_ptr_array_new();
	self->new_node_properties_values = g_array_new(FALSE,TRUE,sizeof(GValue));
	g_array_set_clear_func(self->new_node_properties_values,(GDestroyNotify)g_value_unset);
	// Bootstrap with the net
	static const char* net_prop = "net";
	g_ptr_array_add(self->new_node_properties_names,(gpointer)net_prop);
	g_array_set_size(self->new_node_properties_values,1);
	g_value_set_object(g_value_init((GValue*)self->new_node_properties_values->data,GN_TYPE_NET),self->net);
}
static double gn_main_window_view_scale(GNMainWindow *self)
{
	// Scaling depend of the font-size, big font for big nodes
	return exp(self->view_z) * gdk_screen_get_resolution(gtk_widget_get_screen(GTK_WIDGET(self)));
}

static gboolean gn_main_window_add_link_dialog_draw_logo(GtkWidget *widget, cairo_t *cr, GNNode *node)
{
	GtkAllocation allocation;
	gtk_widget_get_allocated_size(widget,&allocation,NULL);
	double scale = allocation.width < allocation.height ? allocation.width : allocation.height;
	cairo_scale(cr,scale,scale);
	GN_NODE_GET_CLASS(node)->render(node,cr);
	return TRUE;
}
static GtkWidget *gn_main_window_add_link_bind_list_box_create_widget(gpointer item, gpointer user_data)
{
	GNPort *port = GN_PORT(item);
	GNPortClass *port_class = GN_PORT_GET_CLASS(port);
	GtkWidget *row = gtk_list_box_row_new();
	gtk_container_add(GTK_CONTAINER(row),gtk_label_new(port_class->get_name(port)));
	g_object_set_data(G_OBJECT(row),"port",port);
	gtk_widget_show_all(row);
	return row;
}
static void gn_main_window_add_link_dialog_run(GNMainWindow *self, GNNode* node_a, GNNode* node_b)
{
	GNNodeClass *node_a_class = GN_NODE_GET_CLASS(node_a);
	GNNodeClass *node_b_class = GN_NODE_GET_CLASS(node_b);
	// Configure the dialog
	gulong sig_draw_a = g_signal_connect(self->add_link_logo_a_drawarea,"draw",G_CALLBACK(gn_main_window_add_link_dialog_draw_logo),node_a);
	gulong sig_draw_b = g_signal_connect(self->add_link_logo_b_drawarea,"draw",G_CALLBACK(gn_main_window_add_link_dialog_draw_logo),node_b);
	
	gtk_list_box_bind_model(self->add_link_ports_a_listbox,node_a_class->query_portlist_model(node_a),gn_main_window_add_link_bind_list_box_create_widget,NULL,NULL);
	gtk_list_box_bind_model(self->add_link_ports_b_listbox,node_b_class->query_portlist_model(node_b),gn_main_window_add_link_bind_list_box_create_widget,NULL,NULL);
	
	// Run the dialog
	if (gtk_dialog_run(self->add_link_dialog) == GTK_RESPONSE_OK) {
		GNPort *port_a = GN_PORT(g_object_get_data(G_OBJECT(gtk_list_box_get_selected_row(self->add_link_ports_a_listbox)),"port"));
		GNPort *port_b = GN_PORT(g_object_get_data(G_OBJECT(gtk_list_box_get_selected_row(self->add_link_ports_b_listbox)),"port"));
		gn_port_set_link(port_a,port_b,NULL);
	}
	gtk_widget_hide(GTK_WIDGET(self->add_link_dialog));
	
	// Unbind the dialog
	gtk_list_box_bind_model(self->add_link_ports_a_listbox,NULL,gn_main_window_add_link_bind_list_box_create_widget,NULL,NULL);
	gtk_list_box_bind_model(self->add_link_ports_b_listbox,NULL,gn_main_window_add_link_bind_list_box_create_widget,NULL,NULL);
	g_signal_handler_disconnect(self->add_link_logo_a_drawarea,sig_draw_a);
	g_signal_handler_disconnect(self->add_link_logo_b_drawarea,sig_draw_b);
}

static void gn_main_window_reset_new_object(GNMainWindow *self)
{
	// Reset
	self->new_node_type = G_TYPE_INVALID;
	g_array_set_size(self->new_node_properties_values,1);
	
	gtk_toggle_button_set_active(self->add_vm_button,FALSE);
	gtk_toggle_button_set_active(self->add_switch_button,FALSE);
}

G_MODULE_EXPORT void gn_main_window_virt_listbox_row_activated(GtkListBox *box, GtkListBoxRow *row, GNMainWindow *self)
{
	gn_main_window_reset_new_object(self);
	gtk_toggle_button_set_active(self->add_switch_button,FALSE);
	gtk_toggle_button_set_active(self->add_link_button,FALSE);
	static const char* domain_property = "domain";
	g_ptr_array_set_size(self->new_node_properties_names,2);
	self->new_node_properties_names->pdata[1] = domain_property;
	GValue domain_value = G_VALUE_INIT;
	g_value_init(&domain_value,GVIR_TYPE_DOMAIN);
	g_value_set_object(&domain_value,g_object_get_data(G_OBJECT(row),"vir-domain"));
	g_array_append_val(self->new_node_properties_values,domain_value);
	
	self->new_node_type = GN_TYPE_VIR_NODE;
	self->new_node_once = TRUE;
	gtk_toggle_button_set_active(self->add_vm_button,TRUE);
	// FIXME There's a small glitch
}
G_MODULE_EXPORT void gn_main_window_add_switch(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	if (gtk_toggle_button_get_active(toggle_button)) {
		gtk_toggle_button_set_active(self->add_vm_button,FALSE);
		gtk_toggle_button_set_active(self->add_link_button,FALSE);
		g_array_set_size(self->new_node_properties_values,1);
		self->new_node_type = GN_TYPE_VDE_SWITCH;
	} else gn_main_window_reset_new_object(self);
}
G_MODULE_EXPORT void gn_main_window_add_link(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	gtk_toggle_button_set_active(self->add_vm_button,FALSE);
	gtk_toggle_button_set_active(self->add_switch_button,FALSE);
}

G_MODULE_EXPORT gboolean gn_main_window_button_press(GtkWidget *widget, GdkEventButton *event, GNMainWindow *self)
{
	const double view_scale = gn_main_window_view_scale(self);
	const double workspace_x = WORKSPACE_MOUSE(x);
	const double workspace_y = WORKSPACE_MOUSE(y);
	GdkWindow *widget_window = gtk_widget_get_window(widget);
	GdkDisplay *widget_display = gdk_window_get_display(widget_window);
	
	self->grab_object_type = gn_net_whats_here(self->net,&self->grab_object,workspace_x,workspace_y);
	if (self->grab_object_type) {
		GdkCursor *cursor = gdk_cursor_new_from_name(widget_display,"grabbing");
		gdk_window_set_cursor(widget_window,cursor);
		g_object_unref(cursor);
	}
	return TRUE;
}
G_MODULE_EXPORT gboolean gn_main_window_mouse_motion(GtkWidget *widget, GdkEventMotion *event, GNMainWindow *self)
{
	const double view_scale = gn_main_window_view_scale(self);
	const double workspace_x = WORKSPACE_MOUSE(x);
	const double workspace_y = WORKSPACE_MOUSE(y);
	GNNetObject target_object;
	GNNetObjectType target_object_type = gn_net_whats_here(self->net,&target_object,workspace_x,workspace_y);
	GdkWindow *widget_window = gtk_widget_get_window(widget);
	GdkDisplay *widget_display = gdk_window_get_display(widget_window);
	
	switch (self->grab_object_type) {
		case GN_NET_NODE: {
			if (gtk_toggle_button_get_active(self->add_link_button)) {
				// Set cursor according to the destination
				gboolean no_link = (target_object_type != GN_NET_NODE) || (self->grab_object.node == target_object.node);
				const char *cursor_name = no_link ? "no-drop" : "alias";
				GdkCursor *cursor = gdk_cursor_new_from_name(widget_display,cursor_name);
				gdk_window_set_cursor(widget_window,cursor);
				g_object_unref(cursor);
			} else {
				// Set cursor according to the destination
				gboolean no_drop = target_object_type && (self->grab_object.node != target_object.node);
				const char *cursor_name = no_drop ? "no-drop" : "grabbing";
				GdkCursor *cursor = gdk_cursor_new_from_name(widget_display,cursor_name);
				gdk_window_set_cursor(widget_window,cursor);
				g_object_unref(cursor);
				// Move the object
				if (!no_drop) {
					GdkPoint *position = gn_node_position(self->grab_object.node);
					if ((position->x != workspace_x)||(position->y != workspace_y)) {
						position->x = round(workspace_x);
						position->y = round(workspace_y);
						gtk_widget_queue_draw(widget);
					}
				}
			}
		} break;
	}
	return TRUE;
}
G_MODULE_EXPORT gboolean gn_main_window_button_release(GtkWidget *widget, GdkEventButton *event, GNMainWindow *self)
{
	const double view_scale = gn_main_window_view_scale(self);
	const double workspace_x = WORKSPACE_MOUSE(x);
	const double workspace_y = WORKSPACE_MOUSE(y);
	GNNetObject target_object;
	GNNetObjectType target_object_type = gn_net_whats_here(self->net,&target_object,workspace_x,workspace_y);
	GdkWindow *widget_window = gtk_widget_get_window(widget);
	// Check for insertion
	if (!self->grab_object_type && !target_object_type && self->new_node_type) {
		// Create node
		GNNode *new_node = GN_NODE(g_object_new_with_properties(self->new_node_type,self->new_node_properties_values->len,(const char**)self->new_node_properties_names->pdata,(GValue*)self->new_node_properties_values->data));
		GdkPoint *position = gn_node_position(new_node);
		position->x = round(workspace_x);
		position->y = round(workspace_y);
		g_ptr_array_add(self->net->nodes,new_node);
		if (self->new_node_once)
			gn_main_window_reset_new_object(self);
	}
	// Check for link
	if (gtk_toggle_button_get_active(self->add_link_button) && (self->grab_object_type == GN_NET_NODE) && (target_object_type == GN_NET_NODE))
		gn_main_window_add_link_dialog_run(self,self->grab_object.node,target_object.node);
	
	// Resets
	self->grab_object_type = GN_NET_NONE;
	gdk_window_set_cursor(widget_window,NULL);
	gtk_widget_queue_draw(widget);
	return TRUE;
}
G_MODULE_EXPORT void gn_main_window_draw_area(GtkWidget *widget, cairo_t *cr, GNMainWindow *self)
{
	GtkStyleContext *style_context = gtk_widget_get_style_context(widget);
	double scale = gn_main_window_view_scale(self);
	cairo_translate(cr,self->view_x,self->view_y);
	cairo_scale(cr,scale,scale);
	gn_net_render(self->net,cr,style_context);
}

static void gn_main_window_dispose(GObject *gobject)
{
	GNMainWindow *self = GN_MAIN_WINDOW(gobject);
	G_OBJECT_CLASS(gn_main_window_parent_class)->dispose(gobject);
}

static void gn_main_window_finalize(GObject *gobject)
{
	GNMainWindow *self = GN_MAIN_WINDOW(gobject);
	g_object_unref(self->net);
	G_OBJECT_CLASS(gn_main_window_parent_class)->finalize(gobject);
}

static void gn_main_window_class_init(GNMainWindowClass *klass)
{
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	GError *error = NULL;
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNMainWindow");
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,virt_listbox);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_vm_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_switch_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_button);
	
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_dialog);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_logo_a_drawarea);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_logo_b_drawarea);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_ports_a_listbox);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_ports_b_listbox);
	
	objclass->dispose = gn_main_window_dispose;
	objclass->finalize = gn_main_window_finalize;
}
