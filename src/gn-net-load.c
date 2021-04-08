#include "gn-net.h"

static void gn_net_load_root_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static GMarkupParser gn_net_load_root_parser = {
	gn_net_load_root_start_element,
	(void(*)(GMarkupParseContext*,const gchar*,void*,GError**))g_markup_parse_context_pop,
	NULL,
	NULL,
	NULL,
};
static void gn_net_load_main_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static GMarkupParser gn_net_load_main_parser = {
	gn_net_load_main_start_element,
	NULL,
	NULL,
	NULL,
	NULL,
};
static void gn_net_load_skip_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
static void gn_net_load_skip_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
static GMarkupParser gn_net_load_skip_parser = {
	gn_net_load_skip_start_element,
	gn_net_load_skip_end_element,
	NULL,
	NULL,
	NULL,
};

static void gn_net_load_root_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (g_str_equal(element_name,"gn-net"))
		g_markup_parse_context_push(context,&gn_net_load_main_parser,user_data);
	// TODO else
}


gboolean gn_net_load_find_property(gconstpointer a, gconstpointer b)
{
	GParamSpec *param_spec = G_PARAM_SPEC(a);
	return g_str_equal(param_spec->name,b);
}
static void gn_net_load_main_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	// FIXME Oh that fucking disgusting !
	GNNet *net = GN_NET(user_data);
	
	// Prepare node additions
	static char prop_name_net[] = "net";
	GPtrArray *new_node_prop_name = g_ptr_array_new();
	GArray *new_node_prop_value = g_array_new(FALSE,TRUE,sizeof(GValue));
	g_array_set_clear_func(new_node_prop_value,(GDestroyNotify)g_value_unset);
	g_ptr_array_add(new_node_prop_name,(char*)prop_name_net);
	g_array_set_size(new_node_prop_value,1);
	g_value_init((GValue*)new_node_prop_value->data,GN_TYPE_NET);
	g_value_set_object((GValue*)new_node_prop_value->data,net);
	
	// Serious things begin
	if (g_str_equal(element_name,"node")) {
		const char* type_str = NULL;
		for (int i = 0; attribute_names[i]; i++)
			if (g_str_equal(attribute_names[i],"type")) {
				type_str = attribute_values[i];
				break;
			}
		if (type_str) {
			GType type = g_type_from_name(type_str);
			if (g_type_is_a(type,GN_TYPE_NODE)) {
				GNNodeClass *node_class = g_type_class_ref(type);
				// Deserialize properties
				for (int i = 0; attribute_names[i]; i++) {
					const char* prop_name = attribute_names[i];
					const char* prop_str = attribute_values[i];
					guint param_spec_index;
					if (g_ptr_array_find_with_equal_func(node_class->file_properties,prop_name,gn_net_load_find_property,&param_spec_index)) {
						GParamSpec *param_spec = G_PARAM_SPEC(g_ptr_array_index(node_class->file_properties,param_spec_index));
						GValue prop_value = G_VALUE_INIT;
						switch (param_spec->value_type) {
							case G_TYPE_BOOLEAN: {
								// FIXME This is weak
								g_value_init(&prop_value,G_TYPE_BOOLEAN);
								g_value_set_boolean(&prop_value,prop_str[0] == 't');
							} break;
							case G_TYPE_INT: {
								// FIXME This is weak
								g_value_init(&prop_value,G_TYPE_INT);
								g_value_set_int(&prop_value,atoi(prop_str));
							} break;
							case G_TYPE_STRING: {
								g_value_init(&prop_value,G_TYPE_STRING);
								g_value_set_string(&prop_value,prop_str);
							} break;
							default: if (strcmp(attribute_names[i],"type")) {
								g_warning("Cannot deserialize property \"%s\" of %s, unsupported param type %s.",param_spec->name,g_type_name(type),g_type_name(param_spec->value_type));
							} continue; // Skip the next
						}
						// Property has been decoded
						g_ptr_array_add(new_node_prop_name,(char*)prop_name);
						g_array_append_val(new_node_prop_value,prop_value);
						// No g_value_unset(&prop_value); because the prop_value has been memory copied in new_node_prop_value
					}
				}
				// Create the object
				GObject *new_node = g_object_new_with_properties(type,new_node_prop_name->len,(const char**)new_node_prop_name->pdata,(GValue*)new_node_prop_value->data);
				g_ptr_array_add(net->nodes,new_node);
				// Cleanups
				g_ptr_array_set_size(new_node_prop_name,1);
				g_array_set_size(new_node_prop_value,1);
				g_markup_parse_context_push(context,&node_class->file_load_parser,new_node);
				g_type_class_unref(node_class);
			} else g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,"Node type=\"%s\" is unknown",type_str);
		}
	} else if (g_str_equal(element_name,"link")) {
		char* node_a_index_str;
		char* node_b_index_str;
		char* port_a_index_str;
		char* port_b_index_str;
		if (g_markup_collect_attributes(element_name,attribute_names,attribute_values,error,
		 G_MARKUP_COLLECT_STRING,"node_a_index",&node_a_index_str,
		 G_MARKUP_COLLECT_STRING,"node_b_index",&node_b_index_str,
		 G_MARKUP_COLLECT_STRING,"port_a_index",&port_a_index_str,
		 G_MARKUP_COLLECT_STRING,"port_b_index",&port_b_index_str,
		 G_MARKUP_COLLECT_INVALID)) {
			guint node_a_index = atoi(node_a_index_str);
			guint node_b_index = atoi(node_b_index_str);
			guint port_a_index = atoi(port_a_index_str);
			guint port_b_index = atoi(port_b_index_str);
			// Sanity check
			// Shitty doesn't mean no input validation
			// TODO Shit an error on those malicious input (but changing the format is better)
			if (node_a_index >= net->nodes->len) return;
			if (node_b_index >= net->nodes->len) return;
			GNNode *node_a = GN_NODE(net->nodes->pdata[node_a_index]);
			GNNode *node_b = GN_NODE(net->nodes->pdata[node_b_index]);
			GListModel *node_a_ports = GN_NODE_GET_CLASS(node_a)->query_portlist_model(node_a);
			GListModel *node_b_ports = GN_NODE_GET_CLASS(node_b)->query_portlist_model(node_b);
			if (port_a_index >= g_list_model_get_n_items(node_a_ports)) return;
			if (port_b_index >= g_list_model_get_n_items(node_b_ports)) return;
			
			GNPort *port_a = GN_PORT(g_list_model_get_item(node_a_ports,port_a_index));
			GNPort *port_b = GN_PORT(g_list_model_get_item(node_b_ports,port_b_index));
			gn_port_set_link(port_a,port_b,error);
			g_object_unref(port_a);
			g_object_unref(port_b);
		}
	} // TODO else skip this tag
}

static void gn_net_load_skip_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	(*(guint*)user_data)++;
}
static void gn_net_load_skip_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if ((*(guint*)user_data--) != 0) {
		g_slice_free(guint,user_data);
		g_markup_parse_context_pop(context);
	}
}
void gn_net_load_skip_parser_push(GMarkupParseContext *context)
{
	g_markup_parse_context_push(context,&gn_net_load_skip_parser,g_slice_new0(guint));
}

GNNet* gn_net_load(GInputStream *stream, GCancellable *cancellable, GError **error)
{
	GNNet *net = GN_NET(g_object_new(GN_TYPE_NET,NULL));
	GMarkupParseContext *parser = g_markup_parse_context_new(&gn_net_load_root_parser,G_MARKUP_PREFIX_ERROR_POSITION,net,NULL);
	gssize bytes_read;
	char buffer[4096];
	// Parse loop
	while ((bytes_read = g_input_stream_read(stream,buffer,sizeof(buffer),cancellable,error)) > 0)
		if (!g_markup_parse_context_parse(parser,buffer,bytes_read,error))
			break;
	if (!bytes_read)
		if (!g_markup_parse_context_end_parse(parser,error))
			bytes_read = 1; // Indicate that we got a parsing failure
	// Cleanups
	g_markup_parse_context_unref(parser);
	if (bytes_read) {
		g_object_unref(net);
		return NULL;
	} else return net;
}
