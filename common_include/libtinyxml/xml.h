#ifndef __XML_H__
#define __XML_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef void* handle_doc; //xml文件指针
typedef void* handle_node;//xml节点指针
typedef void* handle_attr;//xml节点属性指针

/*
 函 数 名: xml_new
 功能描述: 创建一个xml文件指针，需与xml_delete配对使用
 输入参数: 无
 输出参数: 无
 返 回 值: 成功返回文件指针,失败返回小于NULL
*/
handle_doc xml_new();

/*
 函 数 名: xml_delete
 功能描述: 释放一个xml文件指针，需与xml_new配对使用
 输入参数: hnddoc 文件指针
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int xml_delete(handle_doc hnddoc);

/*
 函 数 名: xml_data
 功能描述: 把xml所有的数据（文本内容），写到buf里面
 输入参数:  hnd 文件指针
            size buf最大长度
 输出参数: buf 输出buf
 返 回 值: 成功返回0,失败返回小于0
*/
int xml_data(handle_doc hnd, char* buf, unsigned int size);

/*
 函 数 名: xml_savefile
 功能描述: 把xml文档指针保存到文件
 输入参数:  hnd 文件指针
            path 文件路径
 输出参数: 无
 返 回 值: 成功返回0,失败返回小于0
*/
int xml_savefile(handle_doc hnd, char* path);

/*
 函 数 名: xml_root_append
 功能描述: 增加一个根节点（适用于xml_new之后），只能有一个根节点
 输入参数:  hnd 文件指针
            node_name 节点名字
            node_text 节点内容
 输出参数: 无
 返 回 值: 成功返回新增节点指针,失败返回NULL
*/
handle_node xml_root_append(handle_doc hnd, char *node_name, char *node_text);

/*
 函 数 名: xml_node_append
 功能描述: 追加一个普通孩子节点
 输入参数:  parent 父节点
            node_name 节点名字
            node_text 节点内容
 输出参数: 无
 返 回 值: 成功返回新增节点指针,失败返回NULL
*/
handle_node xml_node_append(handle_node parent, char *node_name, char *node_text);

/*
 函 数 名: xml_attr_append
 功能描述: 追加一个节点属性
 输入参数:  parent 父节点
            node_name 节点名字
            node_text 节点内容
 输出参数: 无
 返 回 值: 成功返回所加节点指针,失败返回NULL
*/
handle_node xml_attr_append(handle_node hndnode, char *attr_name, char *attr_text);

/*
 函 数 名: xml_load_file
 功能描述: 加载一个xml文件，需要与xml_unload配对使用
 输入参数:  path 文件路径
 输出参数: 无
 返 回 值: 成功返回文件指针,失败返回NULL
*/
handle_doc xml_load_file(char* path);

/*
 函 数 名: xml_load_data
 功能描述: 加载一段xml内容，需要与xml_unload配对使用
 输入参数:  data xml数据
 输出参数: 无
 返 回 值: 成功返回文件指针,失败返回NULL
*/
handle_doc xml_load_data(char* data);

/*
 函 数 名: xml_unload
 功能描述: 释放文件指针，需要与xml_load_file/xml_load_data配对使用
 输入参数:  data xml数据
 输出参数: 无
 返 回 值: 成功返回文件指针,失败返回NULL
*/
int xml_unload(handle_doc hnddoc);

/*
 函 数 名: xml_node_root
 功能描述: 获取根节点
 输入参数:  hnddoc 文件指针
 输出参数: 无
 返 回 值: 成功返回节点指针,失败返回NULL
*/
handle_node xml_node_root(handle_doc hnddoc);

/*
 函 数 名: xml_node_child
 功能描述: 获取孩子节点
 输入参数:  hndnode 节点指针
 输出参数: 无
 返 回 值: 成功返回节点指针,失败返回NULL
*/
handle_node xml_node_child(handle_node hndnode);

/*
 函 数 名: xml_node_next
 功能描述: 获取同级下一个节点
 输入参数:  hndnode 节点指针
 输出参数: 无
 返 回 值: 成功返回节点指针,失败返回NULL
*/
handle_node xml_node_next(handle_node hndnode);

/*
 函 数 名: xml_node_name
 功能描述: 获取节点名字
 输入参数:  hndnode 节点指针
 输出参数: 无
 返 回 值: 成功返回节点名字,失败返回NULL
*/
const char* xml_node_name(handle_node hndnode);

/*
 函 数 名: xml_node_text
 功能描述: 获取节点内容
 输入参数:  hndnode 节点指针
 输出参数: 无
 返 回 值: 成功返回节点内容,失败返回NULL
*/
const char* xml_node_text(handle_node hndnode);

/*
 函 数 名: xml_attr_child
 功能描述: 获取节点的第一个属性
 输入参数:  hndnode 节点指针
 输出参数: 无
 返 回 值: 成功返回节点属性,失败返回NULL
*/
handle_attr xml_attr_child(handle_node hndnode);

/*
 函 数 名: xml_attr_next
 功能描述: 获取同级属性的下一个属性
 输入参数:  hndattr 节点属性
 输出参数: 无
 返 回 值: 成功返回节点属性,失败返回NULL
*/
handle_attr xml_attr_next(handle_attr hndattr);

/*
 函 数 名: xml_attr_name
 功能描述: 获取属性名字
 输入参数:  hndattr 节点属性
 输出参数: 无
 返 回 值: 成功返回属性名字,失败返回NULL
*/
const char* xml_attr_name(handle_attr hndattr);

/*
 函 数 名: xml_attr_text
 功能描述: 获取属性内容
 输入参数:  hndattr 节点属性
 输出参数: 无
 返 回 值: 成功返回属性内容,失败返回NULL
*/
const char* xml_attr_text(handle_attr hndattr);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif

