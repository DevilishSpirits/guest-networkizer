#include "gn-node.h"

// Check if we can use vdens
gboolean gn_vde_ns_can(GError **error);
GSubprocess *gn_vde_ns_subprocess_node(GNNode *node, const gchar * const *argv, GSubprocessFlags flags, GError **error);
