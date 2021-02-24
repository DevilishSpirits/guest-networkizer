/* This test check wheather the following things works.
 * 
 * 1. Rawly add a node (we test with a switch)
 * 2. Create and detach GNNodePanel
 * 3. Delete the node
 *
 * This used to generate g_object_weak_unref() warnings and criticals.
 */
#include "common.h"
#include <gn-node-panel.h>
#include <vde-switch.h>

G_MODULE_EXPORT void gn_node_panel_restore(GtkWidget *self);

void main_test(int argc, char** argv)
{
	puts("1..5");
	
	GNNode *new_node = GN_NODE(g_object_new(GN_TYPE_VDE_SWITCH,"net",mainwindow->net,NULL));
	if (new_node)
		printf("ok 1 - GNNode *new_node == %p\n",new_node);
	else {
		printf("not ok 1 - GNNode *new_node == %p\nBail out! Node creation failed",new_node);
		exit(1);
	}
	g_ptr_array_add(mainwindow->net->nodes,new_node);
	
	GtkWidget *panel = gtk_widget_new(GN_TYPE_NODE_PANEL,"node",new_node,"visible",TRUE,NULL);
	if (panel)
		printf("ok 2 - GNNodePanel *panel == %p\n",panel);
	else {
		printf("not ok 2 - GNNodePanel *panel == %p\nBail out! Panel creation failed",panel);
		exit(1);
	}
	
	gtk_container_add(GTK_CONTAINER(mainwindow->context_node_popover),panel);
	printf("ok 3 - gtk_container_add(container=%p,panel=%p)\n",mainwindow->context_node_popover,panel);
	
	gn_node_panel_restore(panel);
	printf("ok 4 - gn_node_panel_restore(panel=%p)\n",panel);
	
	g_ptr_array_remove_fast(mainwindow->net->nodes,new_node);
	printf("ok 5 - g_ptr_array_remove_fast(net->nodes=%p,node=%p)\n",mainwindow->net->nodes,new_node);
	
	gtk_window_close(GTK_WINDOW(mainwindow));
}


