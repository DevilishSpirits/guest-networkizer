#include "gn-net.h"

static const char gn_file_save_start[] = "<gn-net>\n";
static const char gn_file_save_end[] = "</gn-net>";

static const char gn_file_save_char_kit[] = "<node type=\"/>\n";
// Magic macros
#define GN_FILE_SAVE_NODE_START       &gn_file_save_char_kit[ 0],12 // <node type=\"
#define GN_FILE_SAVE_NODE_PARAM_LEFT  &gn_file_save_char_kit[ 5], 1 //   (a space)
#define GN_FILE_SAVE_NODE_PARAM_MID   &gn_file_save_char_kit[10], 2 // =\"
#define GN_FILE_SAVE_NODE_PARAM_RIGHT &gn_file_save_char_kit[11], 1 // \"
#define GN_FILE_SAVE_NODE_END_SIMPLE  &gn_file_save_char_kit[12], 3 // />\n
#define GN_FILE_SAVE_NODE_END_COMPLEX &gn_file_save_char_kit[13], 2 // >\n
#define GN_FILE_SAVE_NODE_ENDING_TAG  &gn_file_save_end     [ 0], 2 // </
#define GN_FILE_SAVE_NODE_WORD        &gn_file_save_char_kit[ 1], 4 // node

// Some magic macros
gboolean gn_net_save_context_write(struct gn_net_save_context* ctx, const void* data, gsize size)
{
	if (!g_output_stream_write_all(ctx->stream,data,size,NULL,ctx->cancellable,ctx->error)) {
		g_cancellable_cancel(ctx->cancellable);
		return FALSE;
	} else return TRUE;
}
gboolean gn_net_save_context_writev(struct gn_net_save_context* ctx, GOutputVector *vectors, gsize n_vectors)
{
	if (!g_output_stream_writev_all(ctx->stream,vectors,n_vectors,NULL,ctx->cancellable,ctx->error)) {
		g_cancellable_cancel(ctx->cancellable);
		return FALSE;
	} else return TRUE;
}

gboolean gn_net_save_context_dump_object_properties(struct gn_net_save_context* ctx, GObject* object, GParamSpec **param_specs, guint n_properties)
{
	bool result = TRUE;
	GValue value = G_VALUE_INIT;
	for (guint i = 0; i < n_properties; i++) {
		const GParamSpec *param_spec = param_specs[i];
		const char* value_string = NULL; // <- Won't be freed (static or GValue backed string)
		gsize value_string_len;
		g_object_get_property(object,param_spec->name,&value);
		// Serialize property
		switch (G_VALUE_TYPE(&value)) {
			case G_TYPE_BOOLEAN: {
				const char xml_boolean_true[] = "true";
				const char xml_boolean_false[] = "false";
				if (g_value_get_boolean(&value)) {
					value_string = xml_boolean_true;
					value_string_len = sizeof(xml_boolean_true)-1;
				} else {
					value_string = xml_boolean_false;
					value_string_len = sizeof(xml_boolean_false)-1;
				}
			} break;
			case G_TYPE_INT: {
				value_string = g_strdup_printf("%d",g_value_get_int(&value));
				g_value_unset(&value);
				g_value_init(&value,G_TYPE_STRING);
				g_value_take_string(&value,(char*)value_string);
				value_string_len = strlen(value_string);
			} break;
			case G_TYPE_STRING: {
				value_string = g_value_get_string(&value);
				value_string_len = strlen(value_string);
			} break;
			default: {
				g_warning("Cannot serialize property \"%s\" of %s, unsupported param type %s.",param_spec->name,G_OBJECT_TYPE_NAME(object),G_VALUE_TYPE_NAME(&value));
			} break;
		}
		// Write datas
		if (value_string) {
			const gsize name_len = strlen(param_spec->name);
			GOutputVector vector[] = {
				{GN_FILE_SAVE_NODE_PARAM_LEFT},  // Put a leading space  ;_            ;
				{param_spec->name,name_len},     // Property name        ; prop        ;
				{GN_FILE_SAVE_NODE_PARAM_MID},   // prop/value separator ;     ="      ;
				{value_string,value_string_len}, // The value itself     ;       value ;
				{GN_FILE_SAVE_NODE_PARAM_RIGHT}, // Value ending         ;            ";
			};
			result = gn_net_save_context_writev_static(ctx,vector);
		}
		// Free datas
		g_value_unset(&value);
		// Break on error
		if (!result)
			break;
	}
	return result;
}

gboolean gn_net_save(GNNet *net, GOutputStream* stream, GCancellable *cancellable, GError **error)
{
	struct gn_net_save_context ctx = {
		stream,
		cancellable,
		error,
	};
	
	if (!gn_net_save_context_write_static(&ctx,gn_file_save_start))
		return FALSE;
	
	// Save objects
	for (int i = 0; i < net->nodes->len; i++) {
		GNNode *node = (GNNode*)net->nodes->pdata[i];
		GNNodeClass *node_class = GN_NODE_GET_CLASS(node);
		const char *node_name = G_OBJECT_TYPE_NAME(node);
		GOutputVector vector[] = {
			{GN_FILE_SAVE_NODE_START},       // Put the node start
			{node_name,strlen(node_name)},   // Node class
			{GN_FILE_SAVE_NODE_PARAM_RIGHT}, // End the param
		};
		GOutputVector vector_complx_close_tag[] = {
			{GN_FILE_SAVE_NODE_ENDING_TAG},
			{GN_FILE_SAVE_NODE_WORD},
			{GN_FILE_SAVE_NODE_END_COMPLEX},
		};
		if (!( // Write the node with checks
		 gn_net_save_context_writev_static(&ctx,vector) &&
		 gn_net_save_context_dump_object_properties(&ctx,G_OBJECT(node),(GParamSpec**)node_class->file_properties->pdata,node_class->file_properties->len) &&
		 node_class->file_save
		 	? gn_net_save_context_write(&ctx,GN_FILE_SAVE_NODE_END_COMPLEX) && node_class->file_save(node,&ctx) && gn_net_save_context_writev_static(&ctx,vector_complx_close_tag)
		 	: gn_net_save_context_write(&ctx,GN_FILE_SAVE_NODE_END_SIMPLE)
		 ))
			return FALSE;
	}
	// Save links
	for (int i = 0; i < net->links->len; i++) {
		// TODO FIXME This is inneficient and extremely shitty 😭️😭️😭️
		GNLink *link = &g_array_index(net->links,GNLink,i);
		GNNode *node_a = gn_port_get_node(link->port_a);
		GNNode *node_b = gn_port_get_node(link->port_b);
		GListModel *port_a_ports = GN_NODE_GET_CLASS(node_a)->query_portlist_model(node_a);
		GListModel *port_b_ports = GN_NODE_GET_CLASS(node_b)->query_portlist_model(node_b);
		
		guint node_a_index;
		guint node_b_index;
		guint port_a_index;
		guint port_b_index;
		g_ptr_array_find(net->nodes,node_a,&node_a_index);
		g_ptr_array_find(net->nodes,node_b,&node_b_index);
		for (port_a_index = 0; g_list_model_get_item(port_a_ports,port_a_index) != link->port_a; port_a_index++);
		for (port_b_index = 0; g_list_model_get_item(port_b_ports,port_b_index) != link->port_b; port_b_index++);
		
		if (!g_output_stream_printf(stream,NULL,cancellable,error,"<link node_a_index=\"%d\" node_b_index=\"%d\" port_a_index=\"%d\" port_b_index=\"%d\"/>\n",node_a_index,node_b_index,port_a_index,port_b_index))
			return FALSE;
	}
	return gn_net_save_context_write_static(&ctx,gn_file_save_end);
}
