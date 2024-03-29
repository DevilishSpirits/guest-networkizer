#include "gn-main-window.h"
#include "gn-node-panel.h"
#include "vde-plug-url.h"
#include "vde-slirp.h"
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
static double gn_main_window_get_screen_resolution(GNMainWindow *self)
{
	return gdk_screen_get_resolution(gtk_widget_get_screen(GTK_WIDGET(self)));
}
static double gn_main_window_view_scale(GNMainWindow *self)
{
	// Scaling depend of the font-size, big font for big nodes
	return exp(self->view_z) * gn_main_window_get_screen_resolution(self);
}

static void gn_main_window_reset_new_object(GNMainWindow *self)
{
	self->new_node_type = G_TYPE_INVALID;
	g_array_set_size(self->new_node_properties_values,1);
}

G_MODULE_EXPORT void gn_main_window_open(GNMainWindow *self)
{
	
	GtkFileChooserNative *dialog = gtk_file_chooser_native_new(NULL,GTK_WINDOW(self),GTK_FILE_CHOOSER_ACTION_OPEN,NULL,NULL);
	if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GError *error = NULL;
		GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		GFileInputStream *stream = g_file_read(file,NULL,&error);
		if (stream) {
			GNNet *new_net = gn_net_load(G_INPUT_STREAM(stream),NULL,&error);
			if (new_net) {
				g_object_unref(self->net);
				self->net = new_net;
				self->save_file = g_object_ref(file);
				// Redraw upon state change
				for (int i = 0; i < self->net->nodes->len; i++)
					g_signal_connect_object(GN_NODE(g_ptr_array_index(self->net->nodes,i)),"notify::state",G_CALLBACK(gtk_widget_queue_draw),self->workspace_drawingarea,G_CONNECT_SWAPPED);
			}
			g_object_unref(stream);
		}
		g_object_unref(file);
		// Signal error if there one
		if (error) {
			GtkWidget *err_dialog = gtk_message_dialog_new(GTK_WINDOW(self),GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"Open failed");
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(err_dialog),"%s",error->message);
			g_signal_connect(err_dialog,"response",G_CALLBACK(gtk_widget_hide),NULL);
			g_error_free(error);
			gtk_dialog_run(GTK_DIALOG(err_dialog));
		}
	}
	g_object_unref(dialog);
}
G_MODULE_EXPORT void gn_main_window_save_as(GNMainWindow *self);
G_MODULE_EXPORT void gn_main_window_save(GNMainWindow *self)
{
	if (self->save_file) {
		GCancellable *cancellable = g_cancellable_new();
		GOutputStream *stream = G_OUTPUT_STREAM(g_file_replace(self->save_file,NULL,FALSE,0,cancellable,NULL));
		gn_net_save(self->net,stream,cancellable,/* TODO GError **error */NULL);
		g_output_stream_close(stream,cancellable,/* TODO GError **error */NULL);
		g_object_unref(cancellable);
		g_object_unref(stream);
	} else return gn_main_window_save_as(self);
}
G_MODULE_EXPORT void gn_main_window_save_as(GNMainWindow *self)
{
	GtkFileChooserNative *dialog = gtk_file_chooser_native_new(NULL,GTK_WINDOW(self),GTK_FILE_CHOOSER_ACTION_SAVE,NULL,NULL);
	if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_clear_object(&self->save_file);
		self->save_file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		g_object_unref(dialog);
		return gn_main_window_save(self);
	} else g_object_unref(dialog);
}

G_MODULE_EXPORT void gn_main_window_virt_listbox_row_activated(GtkListBox *box, GtkListBoxRow *row, GNMainWindow *self)
{
	gn_main_window_reset_new_object(self);
	static const char* domain_property = "domain";
	g_ptr_array_set_size(self->new_node_properties_names,2);
	self->new_node_properties_names->pdata[1] = (char*)domain_property;
	GValue domain_value = G_VALUE_INIT;
	g_value_init(&domain_value,GVIR_TYPE_DOMAIN);
	g_value_set_object(&domain_value,g_object_get_data(G_OBJECT(row),"vir-domain"));
	g_array_append_val(self->new_node_properties_values,domain_value);
	
	self->new_node_type = GN_TYPE_VIR_NODE;
	self->new_node_once = TRUE;
	gtk_popover_popdown(self->add_vm_popover);
}
G_MODULE_EXPORT void gn_main_window_add_vm(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	if (gtk_toggle_button_get_active(toggle_button))
		gtk_popover_popup(self->add_vm_popover);
}
G_MODULE_EXPORT void gn_main_window_add_switch(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	if (gtk_toggle_button_get_active(toggle_button)) {
		g_array_set_size(self->new_node_properties_values,1);
		self->new_node_type = GN_TYPE_VDE_SWITCH;
	} else if (self->new_node_type == GN_TYPE_VDE_SWITCH)
		gn_main_window_reset_new_object(self);
}
G_MODULE_EXPORT void gn_main_window_add_nat(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	if (gtk_toggle_button_get_active(toggle_button)) {
		g_array_set_size(self->new_node_properties_values,1);
		self->new_node_type = GN_TYPE_VDE_SLIRP;
	} else if (self->new_node_type == GN_TYPE_VDE_SLIRP)
		gn_main_window_reset_new_object(self);
}
G_MODULE_EXPORT void gn_main_window_add_plug_url(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	if (gtk_toggle_button_get_active(toggle_button)) {
		g_array_set_size(self->new_node_properties_values,1);
		self->new_node_type = GN_TYPE_VDE_PLUG_URL;
	} else if (self->new_node_type == GN_TYPE_VDE_PLUG_URL)
		gn_main_window_reset_new_object(self);
}

G_MODULE_EXPORT void gn_main_window_move_mode(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	gn_main_window_reset_new_object(self);
	self->new_node_type = GN_WINDOW_MODE_MOVE;
}
G_MODULE_EXPORT void gn_main_window_link_mode(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	gn_main_window_reset_new_object(self);
	self->new_node_type = GN_WINDOW_MODE_LINK;
}

G_MODULE_EXPORT void gn_main_window_delete_mode(GtkToggleButton *toggle_button, GNMainWindow *self)
{
	gn_main_window_reset_new_object(self);
	self->new_node_type = GN_WINDOW_MODE_DELETE;
}

G_MODULE_EXPORT void gn_main_window_start_all(GtkWidget *widget, GNMainWindow *self)
{
	gn_net_state_all(self->net,GVIR_DOMAIN_STATE_RUNNING);
}
G_MODULE_EXPORT void gn_main_window_shutdown_all(GtkWidget *widget, GNMainWindow *self)
{
	gn_net_state_all(self->net,GVIR_DOMAIN_STATE_SHUTOFF);
}

G_MODULE_EXPORT gboolean gn_main_window_button_press(GtkWidget *widget, GdkEventButton *event, GNMainWindow *self)
{
	const double view_scale = gn_main_window_view_scale(self);
	const double workspace_x = WORKSPACE_MOUSE(x);
	const double workspace_y = WORKSPACE_MOUSE(y);
	GdkWindow *widget_window = gtk_widget_get_window(widget);
	GdkDisplay *widget_display = gdk_window_get_display(widget_window);
	
	self->button_press_mouse_location.x = event->x;
	self->button_press_mouse_location.y = event->y;
	
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
	
	const char* cursor_name = NULL;
	const char* cursor_link = "alias";
	const char* cursor_grabbing = "grabbing";
	const char* cursor_no_drop = "no-drop";
	
	
	GNNetObject target_object;
	GNNetObjectType target_object_type = gn_net_whats_here(self->net,&target_object,workspace_x,workspace_y);
	GdkWindow *widget_window = gtk_widget_get_window(widget);
	GdkDisplay *widget_display = gdk_window_get_display(widget_window);
	gboolean target_is_source = self->grab_object.node == target_object.node;
	
	if (self->new_node_type == GN_WINDOW_MODE_DELETE) {
		if (self->grab_object_type) {
			cursor_name = target_is_source ? cursor_grabbing : cursor_no_drop;
		}
	} else switch (self->grab_object_type) {
		case GN_NET_NONE:break; // Nothing to do
		case GN_NET_NODE: {
			switch (self->new_node_type) {
				case GN_WINDOW_MODE_LINK: {
					// Say weather this will drop a new link
					// TODO Link to myself
					gboolean no_link = (target_object_type != GN_NET_NODE) || target_is_source;
					cursor_name = no_link ? cursor_no_drop : cursor_link;
				} break;
				default: {
					// Put a no drop cursor if dropping is not possible
					gboolean no_drop = target_object_type && !target_is_source;
					cursor_name = no_drop ? cursor_no_drop : cursor_grabbing;
					// Move the object if possible
					if (!no_drop) {
						GdkPoint *position = gn_node_position(self->grab_object.node);
						if ((position->x != workspace_x)||(position->y != workspace_y)) {
							position->x = round(workspace_x);
							position->y = round(workspace_y);
							gtk_widget_queue_draw(widget);
						}
					}
				} break;
			}
		} break;
		case GN_NET_LINK: {
			g_critical("In gn_main_window_mouse_motion(), unimplemented self->grab_object_type == GN_NET_LINK");
		} break;
		default: {
			g_critical("In gn_main_window_mouse_motion(), invalid self->grab_object_type == %d",self->grab_object_type);
		} break;
	}
	
	if (cursor_name) {
		GdkCursor *cursor = gdk_cursor_new_from_name(widget_display,cursor_name);
		gdk_window_set_cursor(widget_window,cursor);
		g_object_unref(cursor);
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
	
	const int is_click_max_delta = gn_main_window_get_screen_resolution(self)/10 + 1;
	gboolean is_click = (abs(self->button_press_mouse_location.x - event->x) <= is_click_max_delta) && (abs(self->button_press_mouse_location.y - event->y) <= is_click_max_delta);
	// Check for context menu
	if ((event->button == 3) && (target_object_type == GN_NET_NODE) && is_click) {
		GdkRectangle rect = {
			(round(workspace_x)-.5)*view_scale + self->view_x,
			(round(workspace_y)-.5)*view_scale + self->view_y,
			view_scale,view_scale
		};
		GtkWidget *previous_child = gtk_bin_get_child(GTK_BIN(self->context_node_popover));
		if (previous_child)
			gtk_container_remove(GTK_CONTAINER(self->context_node_popover),previous_child);
		gtk_container_add(GTK_CONTAINER(self->context_node_popover),gtk_widget_new(GN_TYPE_NODE_PANEL,"node",target_object.node,"visible",TRUE,NULL));
		gtk_popover_set_pointing_to(self->context_node_popover,&rect);
		gtk_popover_popup(self->context_node_popover);
	} else
	// Check for insertion
	switch (self->new_node_type) {
		case GN_WINDOW_MODE_MOVE: break;
		case GN_WINDOW_MODE_LINK: {
			if ((self->grab_object_type == GN_NET_NODE) && (target_object_type == GN_NET_NODE))
				gn_main_window_add_link_dialog_run(self,self->grab_object.node,target_object.node);
		} break;
		case GN_WINDOW_MODE_DELETE: {
			if (self->grab_object.node == target_object.node)
				switch (target_object_type) {
					case GN_NET_NONE:break; // Nothing to do
					case GN_NET_NODE: {
						g_ptr_array_remove_fast(self->net->nodes,target_object.node);
					} break;
					case GN_NET_LINK: {
						gn_port_set_link(target_object.link->port_a,NULL,NULL);
					} break;
					default: {
						g_critical("In gn_main_window_mouse_motion(), invalid target_object_type = %d",target_object_type);
					} break;
				}
		} break;
		default: {
			if (!self->grab_object_type && !target_object_type) {
			// Create node
			GNNode *new_node = GN_NODE(g_object_new_with_properties(self->new_node_type,self->new_node_properties_values->len,(const char**)self->new_node_properties_names->pdata,(GValue*)self->new_node_properties_values->data));
			GdkPoint *position = gn_node_position(new_node);
			position->x = round(workspace_x);
			position->y = round(workspace_y);
			g_ptr_array_add(self->net->nodes,new_node);
			if (self->new_node_once)
				gn_main_window_reset_new_object(self);
			// Redraw upon state change
			g_signal_connect_object(new_node,"notify::state",G_CALLBACK(gtk_widget_queue_draw),widget,G_CONNECT_SWAPPED);
			}
		} break;
	}
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
	G_OBJECT_CLASS(gn_main_window_parent_class)->dispose(gobject);
}

static void gn_main_window_finalize(GObject *gobject)
{
	GNMainWindow *self = GN_MAIN_WINDOW(gobject);
	g_clear_object(&self->save_file);
	g_object_unref(self->net);
	G_OBJECT_CLASS(gn_main_window_parent_class)->finalize(gobject);
}

static void gn_main_window_class_init(GNMainWindowClass *klass)
{
	GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	gtk_widget_class_set_template_from_resource(widget_class,"/me/d_spirits/guest_networkizer/ui/GNMainWindow");
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,virt_listbox);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_vm_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_vm_popover);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_switch_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_nat_button);
	
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_dialog);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_ok_button);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_logo_a_drawarea);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_logo_b_drawarea);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_ports_a_listbox);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,add_link_ports_b_listbox);
	
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,context_node_popover);
	gtk_widget_class_bind_template_child(widget_class,GNMainWindow,workspace_drawingarea);
	
	objclass->dispose = gn_main_window_dispose;
	objclass->finalize = gn_main_window_finalize;
}
