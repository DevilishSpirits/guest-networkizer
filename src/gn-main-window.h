#include <gtk/gtk.h>
#include "gn-net.h"

G_BEGIN_DECLS

#define GN_TYPE_MAIN_WINDOW gn_main_window_get_type ()
G_DECLARE_FINAL_TYPE(GNMainWindow,gn_main_window,GN,MAIN_WINDOW,GtkApplicationWindow)

struct _GNMainWindow
{
	GtkApplicationWindow parent_instance;
	
	GNNet *net;
	double view_x;
	double view_y;
	double view_z;
	
	// The grabbed object
	GNNetObjectType grab_object_type;
	GNNetObject grab_object;
	
	/*  The object to insert on click
	 *
	 * When no object is set :
	 * - new_node_type is G_TYPE_INVALID
	 * - new_node_once is FALSE
	 * - new_node_properties_names is undefined
	 * - new_node_properties_values may have properties that must not be changed
	 * You are not obliged to downsize new_node_properties_names when setting.
	 * Always reset with gn_main_window_reset_new_object()
	 *
	 * new_node_properties_names contain static strings (they aren't freed).
	 * If new_node_once is TRUE, only one instance can be created instead of the
	 * default behavior to create as much as the user click.
	 */
	GType      new_node_type;
	gboolean   new_node_once;
	GPtrArray *new_node_properties_names;
	GArray    *new_node_properties_values;
	
	// Template widgets
	GtkListBox* virt_listbox;
	GtkToggleButton* add_vm_button;
	GtkToggleButton* add_switch_button;
	GtkToggleButton* add_link_button;
	GtkToggleButton* add_nat_button;
	
	GtkDialog *add_link_dialog;
	GtkDrawingArea *add_link_logo_a_drawarea;
	GtkDrawingArea *add_link_logo_b_drawarea;
	GtkListBox    *add_link_ports_a_listbox;
	GtkListBox    *add_link_ports_b_listbox;
};

G_END_DECLS
