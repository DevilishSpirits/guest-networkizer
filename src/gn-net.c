#include "gn-net.h"
#include <math.h>

G_DEFINE_TYPE (GNNet,gn_net,G_TYPE_OBJECT)

static void gn_net_init(GNNet *self)
{
	self->nodes = g_ptr_array_new_with_free_func(g_object_unref);
	self->links = g_array_new(FALSE,FALSE,sizeof(GNLink));
}

static void gn_net_draw_node(GNNode *node, cairo_t *cr)
{
	GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
	GdkPoint *position = gn_node_position(node);
	// Go in node coordinates
	cairo_save(cr);
	cairo_translate(cr,position->x,position->y);
	cairo_scale(cr,.8,.8); // Gap between components
	// Draw the node
	cairo_save(cr);
	node_class->render(node,cr);
	cairo_restore(cr);
	// Draw label
	cairo_text_extents_t extents;
	const char *label = node_class->get_label(node);
	cairo_set_font_size(cr,.2);
	cairo_text_extents(cr,label,&extents);
	cairo_translate(cr,-extents.width/2,.8);
	// Set background depending of the VM state
	switch (node_class->get_state(node)) {
		case GVIR_DOMAIN_STATE_RUNNING    :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_RUNNING    ,.8);break;
		case GVIR_DOMAIN_STATE_BLOCKED    :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_BLOCKED    ,.8);break;
		case GVIR_DOMAIN_STATE_PAUSED     :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_PAUSED     ,.8);break;
		case GVIR_DOMAIN_STATE_SHUTDOWN   :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_SHUTDOWN   ,.8);break;
		case GVIR_DOMAIN_STATE_SHUTOFF    :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_SHUTOFF    ,.8);break;
		case GVIR_DOMAIN_STATE_CRASHED    :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_CRASHED    ,.8);break;
		case GVIR_DOMAIN_STATE_PMSUSPENDED:cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_PMSUSPENDED,.8);break; 
		default                           :cairo_set_source_rgba(cr,GN_NODE_DARK_COLOR_DEFAULT    ,.8);break;
	}
	cairo_rectangle(cr,-.1+extents.x_bearing,-.1+extents.y_bearing,extents.width+.2,extents.height+.2);
	cairo_fill(cr);
	cairo_set_source_rgb(cr,1,1,1);
	cairo_show_text(cr,label);
	cairo_restore(cr);
}

static void gn_net_draw_link(GNLink *link, cairo_t *cr)
{
	GdkPoint *point_a = gn_node_position(gn_port_get_node(link->port_a));
	GdkPoint *point_b = gn_node_position(gn_port_get_node(link->port_b));
	cairo_move_to(cr,point_a->x,point_a->y);
	cairo_line_to(cr,point_b->x,point_b->y);
}
void gn_net_render(GNNet *self, cairo_t *cr, GtkStyleContext *style_context)
{
	// Draw links
	GdkRGBA link_color;
	if (style_context)
		gtk_style_context_get_color(style_context,GTK_STATE_FLAG_LINK,&link_color);
	else {
		link_color.red   = .5;
		link_color.green = .5;
		link_color.blue  = 1;
		link_color.alpha = 1;
	}
	cairo_set_source_rgba(cr,link_color.red,link_color.green,link_color.blue,link_color.alpha);
	cairo_set_line_width(cr,.1);
	for (int i = 0; i < self->links->len; i++)
		gn_net_draw_link(&g_array_index(self->links,GNLink,i),cr);
	cairo_stroke(cr);
	// Draw objects
	g_ptr_array_foreach(self->nodes,(GFunc)gn_net_draw_node,cr);
}

GNNetObjectType gn_net_whats_here(GNNet *self, GNNetObject* results, gdouble x, gdouble y)
{
	GNNetObjectType fuzzy_match = GN_NET_NONE;
	for (int i = 0; i < self->nodes->len; i++) {
		// Get distance with the center
		GNNode *node = GN_NODE(self->nodes->pdata[i]);
		GdkPoint *position = gn_node_position(node);
		double dx = fabs(position->x - x)*2;
		double dy = fabs(position->y - y)*2;
		// Check if outside
		if ((dx > 1)||(dy > 1))
			continue;
		// Found a match
		fuzzy_match = GN_NET_NODE;
		results->node = node;
		// Fuzzy check
		if ((dx < .8)||(dy < .8))
			return GN_NET_NODE;
		else break;
	}
	// TODO GN_NET_LINK
	return fuzzy_match;
}

static void gn_net_dispose(GObject *gobject)
{
	GNNet *self = GN_NET(gobject);
	g_array_remove_range(self->links,0,self->links->len);
	g_ptr_array_remove_range(self->nodes,0,self->nodes->len);
	G_OBJECT_CLASS(gn_net_parent_class)->dispose(gobject);
}

static void gn_net_finalize(GObject *gobject)
{
	GNNet *self = GN_NET(gobject);
	g_array_unref(self->links);
	g_ptr_array_unref(self->nodes);
	G_OBJECT_CLASS(gn_net_parent_class)->finalize(gobject);
}

static void gn_net_class_init(GNNetClass *klass)
{
	GObjectClass* objclass = G_OBJECT_CLASS(klass);
	
	objclass->dispose = gn_net_dispose;
	objclass->finalize = gn_net_finalize;
}
