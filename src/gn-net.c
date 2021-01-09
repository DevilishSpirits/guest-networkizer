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
	GdkPoint *position = gn_node_position(node);
	cairo_save(cr);
	cairo_translate(cr,position->x,position->y);
	cairo_scale(cr,.8,.8); // Gap between components
	GN_NODE_GET_CLASS(node)->render(node,cr);
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
