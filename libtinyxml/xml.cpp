
#include "tinyxml.h"
#include "tinystr.h"
#include <iostream>

#include "xml.h"



#if 0
/**
* @breif: 插入节点
* @param pPRE_Node: 父节点
* @param NodeName: 节点名字
* @param NodeText: 节点内容
* @param AttributeName: 属性名字
* @param AttributeValue: 属性值
* @return value: NO
*/
void CreateNode(TiXmlNode *ParentNode, const char *NodeName, const char *NodeText,
                const char *AttributeName, const char *AttributeValue)
{
    TiXmlElement* pElement = new TiXmlElement(NodeName);
    if (NodeText)
    {
        TiXmlText* pText = new TiXmlText(NodeText);
        pElement->LinkEndChild(pText);
    }
    if (AttributeName != NULL && AttributeValue != NULL)
    {
        pElement->SetAttribute(AttributeName, AttributeValue);
    }
    ParentNode->ToElement()->LinkEndChild(pElement);

    return ;
}

void CreateXmlFile()
{
    TiXmlDocument *pdoc = new TiXmlDocument("test.xml");
    if (pdoc == NULL)
    {
        return ;
    }

    TiXmlDeclaration *pdeclaration = new TiXmlDeclaration("1.1.0", "UTF-8", "");
    if (pdeclaration == NULL)
    {
        return ;
    }
    pdoc->LinkEndChild(pdeclaration);

    //添加头结点，并添加到头结点
    TiXmlNode* pnodeANJUBAO = new TiXmlElement("ANJUBAO");
    pdoc->LinkEndChild(pnodeANJUBAO);
    //添加第二级节点
    TiXmlNode* pnodeSHOPALARM = new TiXmlElement("SHOP_ALARM");
    pnodeANJUBAO->LinkEndChild(pnodeSHOPALARM);

    //添加同一层次的4个节点，并设置属性和内容
    //注意类型转换不可直接强制转换，需要掉类的内部成员函数
    CreateNode(pnodeSHOPALARM, "IPC_ID", "123456", "type", "int");
    CreateNode(pnodeSHOPALARM, "IP_ADDR", "192.168.34.115", "type", "string");
    CreateNode(pnodeSHOPALARM, "PORT", "6666", "type", "int");
    CreateNode(pnodeSHOPALARM, "SENSOR", "AR0130", "type", "string");

    pdoc->SaveFile();

    return ;
}

TiXmlNode * selectChildNode(TiXmlNode * pNode, std::string nodeName, std::string nodeAttrName, std::string nodeAttrValue)
{
    if(pNode == NULL)
    {
        return NULL;
    }
    TiXmlNode * pSelectedNode = NULL;
    TiXmlNode * pChildNode = NULL;
    int t = pNode->Type();
    switch (t)
    {
    case TiXmlText::TINYXML_DOCUMENT:
    case TiXmlText::TINYXML_DECLARATION:
    case TiXmlText::TINYXML_TEXT:
    case TiXmlText::TINYXML_UNKNOWN:
    case TiXmlText::TINYXML_COMMENT:
        break;
    case TiXmlText::TINYXML_ELEMENT:
        if(pNode->Value() == nodeName)
        {
            if(!nodeAttrName.empty() && !nodeAttrValue.empty())
            {
                TiXmlElement * pElement = pNode->ToElement();
                TiXmlAttribute * pAttr = pElement->FirstAttribute();
                if(pAttr != NULL)
                {
                    do
                    {
                        if(pAttr->Name()==nodeAttrName&&pAttr->Value()== nodeAttrValue)
                        {
                            return pNode;
                        }
                    }
                    while((pAttr = pAttr->Next()) != NULL);
                }
            }
            else
            {
                return pNode;
            }

        }
        else
        {
            //循环访问它的每一个元素
            for(pChildNode=pNode->FirstChild();
                    pChildNode!=0;
                    pChildNode = pChildNode->NextSibling())
            {
                pSelectedNode = selectChildNode(pChildNode,nodeName,nodeAttrName,nodeAttrValue);
                if(pSelectedNode)
                {
                    return pSelectedNode;
                }
            }
        }

    default:
        break;
    }

    return NULL;
}

//通过根节点查找一个指定节点
TiXmlNode * SelectSingleNodeByRootEle(TiXmlElement* RootElement,const char *nodeName,const char *nodeAttrName,const char *nodeAttrValue)
{

    TiXmlNode * pNode  = NULL;
    TiXmlNode * pSelectNode = NULL;

    for(pNode=RootElement->FirstChildElement(); pNode; pNode=pNode->NextSiblingElement())
    {

        pSelectNode = selectChildNode(pNode,nodeName,nodeAttrName,nodeAttrValue);
        if(pSelectNode)
        {
            break;
        }
    }

    if(pSelectNode)
    {
        std::cout << "Find Node success!" << std::endl;
        std::cout << pSelectNode->Value() << std::endl;
        //std::cout << pSelectNode->ToElement()->GetText() << std::endl;
        return pSelectNode;
    }
    else
    {
        std::cout << "Find Node fail!" << std::endl;
        return NULL;
    }

}

//修改节点的属性和内容
void ModifyXmlFile(TiXmlElement* element, const char *NewText, const char *NewAttributeName,
                    const char *NewAttributeValue)
{
    element->Clear();
    if (NewAttributeName != NULL && NewAttributeValue != NULL)
    {
        element->SetAttribute(NewAttributeName, NewAttributeValue);
    }

    TiXmlText* pNEWtext = new TiXmlText(NewText);
    element->LinkEndChild(pNEWtext);

    return ;
}

//增加节点
TiXmlNode * AddElement(TiXmlElement *position, const char *ElementName, const char *ElementText,
                const char *AttributeName, const char *AttributeValue,
                const char *SecondAttributeName, const char *SecondAttributeValue)
{
    TiXmlElement* addNode = new TiXmlElement(ElementName);
    if(!addNode)
    {
        return NULL;
    }

    //增加节点的值
    if (ElementText != NULL)
    {
        TiXmlText* text = new TiXmlText(ElementText);
        addNode->LinkEndChild(text);
    }

    if (AttributeName != NULL && AttributeValue != NULL)
    {
        addNode->SetAttribute(AttributeName, AttributeValue);
    }
    if (SecondAttributeName != NULL && SecondAttributeValue != NULL)
    {
        addNode->SetAttribute(SecondAttributeName, SecondAttributeValue);
    }

    //增加到同一层的末端
    //position->Parent()->InsertEndChild(*addNode);
    //增加到节点的后面第一个位置
    //position->Parent()->InsertAfterChild(position, *addNode);


    //增加到子节点
    //position->InsertAfterChild(position, *addNode);
    TiXmlNode *ret = position->InsertEndChild(*addNode);

    return ret;
}

//删除指定节点
void DeleteElement(TiXmlNode *pNode)
{
    TiXmlNode *pParNode =  pNode->Parent();
    TiXmlElement* pParentEle = pParNode->ToElement();
    pParentEle->RemoveChild(pNode);

    return ;
}

//遍历同一层元素，并把所有内容输出到标准输出
void ReadXmlElement(TiXmlElement *pCurrentElement)
{
    while( pCurrentElement )
    {
        std::cout << "value: "<<pCurrentElement->Value()<<std::endl;;
        std::cout << "text: "<<pCurrentElement->GetText()<<std::endl;
        TiXmlAttribute *pAttribute = pCurrentElement->FirstAttribute();
        while(pAttribute)
        {
            std::cout<<"Attribute name: "<<pAttribute->Name()<<std::endl;
            std::cout<<"Attribute value: "<<pAttribute->Value()<<std::endl;
            pAttribute = pAttribute->Next();
        }
        pCurrentElement = pCurrentElement->NextSiblingElement();
    }
    return ;
}

const char *GetAttributeValue(TiXmlNode *pNode, const char *name)
{
    TiXmlElement *pElement = pNode->ToElement();
    TiXmlAttribute *pAttribute = pElement->FirstAttribute();
    const char *temp;

    while(pAttribute)
    {
        if (strcmp(pAttribute->Name(), name) == 0)
        {
           temp = pAttribute->Value();
           break;
        }else
        {
            pAttribute = pAttribute->Next();
        }
    }

    return temp;
}
#endif

handle_doc xml_new()
{
    TiXmlDocument *pdoc = new TiXmlDocument;
    if (pdoc)
    {
        TiXmlDeclaration *pdeclaration = new TiXmlDeclaration("1.0", "utf-8", "");
        if (pdeclaration)
        {
            pdoc->LinkEndChild(pdeclaration);
            return pdoc;
        }
    }

    return NULL;
}

int xml_delete(handle_doc hnddoc)
{
    if (hnddoc)
    {
        TiXmlDocument *pdoc = (TiXmlDocument*)hnddoc;
        delete pdoc;
        return 0;
    }

    return -1;
}

// write the document to a string buf
int xml_data(handle_doc hnd, char* buf, unsigned int size)
{
    if (hnd)
    {
        TiXmlDocument *pdoc = (TiXmlDocument*)hnd;
        // Declare a printer
        TiXmlPrinter printer;
        // attach it to the document you want to convert in to a std::string
        pdoc->Accept(&printer);
        if (buf && size >= printer.Size())
        {
            memcpy(buf, printer.CStr(), printer.Size());
            return 0;
        }
    }
   return -1;
}

int xml_savefile(handle_doc hnd, char* path)
{
    if (hnd && path)
    {
        TiXmlDocument *pdoc = (TiXmlDocument*)hnd;
        pdoc->SaveFile(path);
        //pdoc->Print();
        return 0;
    }
   return -1;
}

handle_node xml_root_append(handle_doc hnd, char *node_name, char *node_text)
{
    if (hnd && node_name)
    {
        TiXmlDocument *pdoc = (TiXmlDocument*)hnd;
        TiXmlElement* pnode_root = new TiXmlElement(node_name);
        if (node_text)
        {
            TiXmlText* pText = new TiXmlText(node_text);
            pnode_root->LinkEndChild(pText);
        }
        pdoc->LinkEndChild(pnode_root);
        return pnode_root;
    }
    return NULL;
}

handle_node xml_node_append(handle_node parent, char *node_name, char *node_text)
{
    if (parent && node_name)
    {
        TiXmlNode *ParentNode = (TiXmlNode*)parent;
        TiXmlElement* pElement = new TiXmlElement(node_name);
        if (node_text)
        {
            TiXmlText* pText = new TiXmlText(node_text);
            pElement->LinkEndChild(pText);
        }
        ParentNode->ToElement()->LinkEndChild(pElement);
        return pElement;
    }

    return NULL;
}

handle_node xml_attr_append(handle_node hndnode, char *attr_name, char *attr_text)
{
    if (hndnode)
    {
        TiXmlElement* pElement = (TiXmlElement*)hndnode;
        if (attr_name != NULL && attr_text != NULL)
        {
            pElement->SetAttribute(attr_name, attr_text);
        }
        return pElement;
    }

    return NULL;
}

handle_doc xml_load_file(char* path)
{
    if (path)
    {
        TiXmlDocument* pdoc = new TiXmlDocument;;
        pdoc->LoadFile(path);
        return pdoc;
    }
    return NULL;
}

handle_doc xml_load_data(char* data)
{
    if (data)
    {
        TiXmlDocument* pdoc = new TiXmlDocument;;
        pdoc->Parse(data);
        if (!pdoc->ErrorId())
        {
            return pdoc;
        }
    }
    return NULL;
}

int xml_unload(handle_doc hnddoc)
{
    if (hnddoc)
    {
        TiXmlDocument* pdoc = (TiXmlDocument*)hnddoc;
        delete pdoc;
        return 0;
    }
    return -1;
}

handle_node xml_node_root(handle_doc hnddoc)
{
    if (hnddoc)
    {
        TiXmlDocument* pdoc = (TiXmlDocument*)hnddoc;
        TiXmlElement* root = pdoc->RootElement();
        return root;
    }
    return NULL;
}

handle_node xml_node_child(handle_node hndnode)
{
    if (hndnode)
    {
        TiXmlElement* pnode = (TiXmlElement*)hndnode;
        TiXmlElement* child = pnode->FirstChildElement();
        return child;
    }
    return NULL;
}

handle_node xml_node_next(handle_node hndnode)
{
    if (hndnode)
    {
        TiXmlElement* pnode = (TiXmlElement*)hndnode;
        return pnode->NextSiblingElement();
    }
    return NULL;
}

const char* xml_node_name(handle_node hndnode)
{
    if (hndnode)
    {
        TiXmlElement* pnode = (TiXmlElement*)hndnode;
        return pnode->Value();
    }
    return NULL;
}

const char* xml_node_text(handle_node hndnode)
{
    if (hndnode)
    {
        TiXmlElement* pnode = (TiXmlElement*)hndnode;
        return pnode->GetText();
    }
    return NULL;
}

handle_attr xml_attr_child(handle_node hndnode)
{
    if (hndnode)
    {
        TiXmlElement* pnode = (TiXmlElement*)hndnode;
        return pnode->FirstAttribute();
    }
    return NULL;
}

handle_attr xml_attr_next(handle_attr hndattr)
{
    if (hndattr)
    {
        TiXmlAttribute *pAttribute = (TiXmlAttribute*)hndattr;
        return pAttribute->Next();
    }
    return NULL;
}

const char* xml_attr_name(handle_attr hndattr)
{
    if (hndattr)
    {
        TiXmlAttribute* pattr = (TiXmlAttribute*)hndattr;
        return pattr->Name();
    }
    return NULL;
}

const char* xml_attr_text(handle_attr hndattr)
{
    if (hndattr)
    {
        TiXmlAttribute* pattr = (TiXmlAttribute*)hndattr;
        return pattr->Value();
    }
    return NULL;
}



