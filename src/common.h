#ifndef COMMON_H
#define COMMON_H

#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

xmlNodePtr get_node_by_name(xmlNodePtr node, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
