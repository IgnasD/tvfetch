#include <stdlib.h>
#include <libxml/tree.h>

xmlNodePtr get_node_by_name(xmlNodePtr node, const char *name) {
    xmlNodePtr cur_node;
    const xmlChar *xml_name = (const xmlChar *)name;

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if (!xmlStrcmp(cur_node->name, xml_name)) {
                return cur_node;
            }
        }
    }
    
    return NULL;
}
