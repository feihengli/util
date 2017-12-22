#ifndef __XML_H__
#define __XML_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef void* handle_doc;
typedef void* handle_node;
typedef void* handle_attr;


handle_doc xml_new();
int xml_delete(handle_doc hnddoc);

int xml_data(handle_doc hnd, char* buf, unsigned int size);
int xml_savefile(handle_doc hnd, char* path);
handle_node xml_root_append(handle_doc hnd, char *node_name, char *node_text);
handle_node xml_node_append(handle_node parent, char *node_name, char *node_text);
handle_node xml_attr_append(handle_node hndnode, char *attr_name, char *attr_text);

handle_doc xml_load_file(char* path);
handle_doc xml_load_data(char* data);
int xml_unload(handle_doc hnddoc);
handle_node xml_node_root(handle_doc hnddoc);
handle_node xml_node_child(handle_node hndnode);
handle_node xml_node_next(handle_node hndnode);
const char* xml_node_name(handle_node hndnode);
const char* xml_node_text(handle_node hndnode);

handle_attr xml_attr_child(handle_node hndnode);
handle_attr xml_attr_next(handle_attr hndattr);
const char* xml_attr_name(handle_attr hndattr);
const char* xml_attr_text(handle_attr hndattr);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif

