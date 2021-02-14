#include "gn-main-window.h"
#include "vir-node.h"
#include <math.h>

static gboolean gn_main_window_add_link_dialog_draw_logo(GtkWidget *widget, cairo_t *cr, GNNode *node)
{
	GtkAllocation allocation;
	gtk_widget_get_allocated_size(widget,&allocation,NULL);
	double scale = allocation.width < allocation.height ? allocation.width : allocation.height;
	cairo_scale(cr,scale,scale);
	cairo_translate(cr,.5,.5);
	GN_NODE_GET_CLASS(node)->render(node,cr);
	return TRUE;
}
static GtkWidget *gn_main_window_add_link_bind_list_box_create_widget(gpointer item, gpointer user_data)
{
	GNPort *port = GN_PORT(item);
	GNPortClass *port_class = GN_PORT_GET_CLASS(port);
	GtkWidget *row = gtk_list_box_row_new();
	// Create a label
	gtk_container_add(GTK_CONTAINER(row),gtk_label_new(port_class->get_name(port)));
	// Disable if the port is already linked
	if (gn_port_get_link(port)) {
		gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row),FALSE);
		gtk_widget_set_sensitive(row,FALSE);
	}
	// Finish
	g_object_set_data(G_OBJECT(row),"port",port);
	gtk_widget_show_all(row);
	return row;
}

G_MODULE_EXPORT void gn_main_window_add_link_dialog_selection_changed(gpointer item, GNMainWindow *self)
{
	gtk_widget_set_sensitive(self->add_link_ok_button,gtk_list_box_get_selected_row(self->add_link_ports_a_listbox)&&gtk_list_box_get_selected_row(self->add_link_ports_b_listbox));
}
void gn_main_window_add_link_dialog_run(GNMainWindow *self, GNNode* node_a, GNNode* node_b)
{
	GNNodeClass *node_a_class = GN_NODE_GET_CLASS(node_a);
	GNNodeClass *node_b_class = GN_NODE_GET_CLASS(node_b);
	// Configure the dialog
	gtk_widget_set_sensitive(self->add_link_ok_button,FALSE);
	
	gulong sig_draw_a = g_signal_connect(self->add_link_logo_a_drawarea,"draw",G_CALLBACK(gn_main_window_add_link_dialog_draw_logo),node_a);
	gulong sig_draw_b = g_signal_connect(self->add_link_logo_b_drawarea,"draw",G_CALLBACK(gn_main_window_add_link_dialog_draw_logo),node_b);
	
	gulong sig_tooltip_a = g_signal_connect_swapped(self->add_link_logo_a_drawarea,"query-tooltip",G_CALLBACK(node_a_class->query_tooltip),node_a);
	gulong sig_tooltip_b = g_signal_connect_swapped(self->add_link_logo_b_drawarea,"query-tooltip",G_CALLBACK(node_b_class->query_tooltip),node_b);
	
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
	g_signal_handler_disconnect(self->add_link_logo_a_drawarea,sig_tooltip_a);
	g_signal_handler_disconnect(self->add_link_logo_b_drawarea,sig_tooltip_b);
	g_signal_handler_disconnect(self->add_link_logo_a_drawarea,sig_draw_a);
	g_signal_handler_disconnect(self->add_link_logo_b_drawarea,sig_draw_b);
}
