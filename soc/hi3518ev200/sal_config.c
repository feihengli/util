#include "sal_debug.h"
#include "sal_config.h"
#include "libtinyxml/xml.h"

sal_config_s g_config =
{
    {"1.0.0"},
    {{0}}, //stream

    { //md
        1,
        2,
        320,
        240
    },

    { //jpeg
        1,
        3,
        640,
        360
    }
};

int sal_config_default(sal_config_s* config)
{
    CHECK(config, -1, "invalid parameter\n");

    strcpy(config->version, "1.0.0");
    unsigned int i = 0;
    strcpy(config->video[i].type, "H264");
    strcpy(config->video[i].resolution, "1080P");
    config->video[i].width = 1920;
    config->video[i].height = 1080;
    strcpy(config->video[i].profile, "HIGH");
    config->video[i].fps = 25;
    config->video[i].gop = 2;
    config->video[i].bitrate = 2000;
    strcpy(config->video[i].bitrate_ctrl, "VBR");
    i++;

    strcpy(config->video[i].type, "H264");
    strcpy(config->video[i].resolution, "360P");
    config->video[i].width = 640;
    config->video[i].height = 360;
    strcpy(config->video[i].profile, "HIGH");
    config->video[i].fps = 25;
    config->video[i].gop = 2;
    config->video[i].bitrate = 500;
    strcpy(config->video[i].bitrate_ctrl, "VBR");

    return 0;
}

int sal_config_save(char* path, sal_config_s* config)
{
    CHECK(path && config, -1, "invalid parameter\n");

    unsigned int i = 0;
    handle_node node = NULL;
    char tmp[128];

    handle_doc doc = xml_new();
    CHECK(doc, -1, "error with %#x\n", doc);

    handle_node root_node = xml_root_append(doc, "config", NULL);
    CHECK(root_node, -1, "error with %#x\n", root_node);

    node = xml_node_append(root_node, "version", config->version);
    CHECK(node, -1, "error with %#x\n", node);

    for (i = 0; i < sizeof(config->video)/sizeof(*config->video); i++)
    {
        snprintf(tmp, sizeof(tmp), "video_%d", i);
        node = xml_node_append(root_node, tmp, NULL);
        CHECK(node, -1, "error with %#x\n", node);

        snprintf(tmp, sizeof(tmp), "%s", config->video[i].type);
        xml_attr_append(node, "type", tmp);

        snprintf(tmp, sizeof(tmp), "%s", config->video[i].resolution);
        xml_attr_append(node, "resolution", tmp);

        snprintf(tmp, sizeof(tmp), "%d", config->video[i].width);
        xml_attr_append(node, "width", tmp);

        snprintf(tmp, sizeof(tmp), "%d", config->video[i].height);
        xml_attr_append(node, "height", tmp);

        snprintf(tmp, sizeof(tmp), "%s", config->video[i].profile);
        xml_attr_append(node, "profile", tmp);

        snprintf(tmp, sizeof(tmp), "%d", config->video[i].fps);
        xml_attr_append(node, "fps", tmp);

        snprintf(tmp, sizeof(tmp), "%d", config->video[i].gop);
        xml_attr_append(node, "gop", tmp);

        snprintf(tmp, sizeof(tmp), "%d", config->video[i].bitrate);
        xml_attr_append(node, "bitrate", tmp);

        snprintf(tmp, sizeof(tmp), "%s", config->video[i].bitrate_ctrl);
        xml_attr_append(node, "bitrate_ctrl", tmp);
    }

    xml_savefile(doc, path);
    xml_delete(doc);
    return 0;
}

static int sal_config_vattr_parse(handle_attr* attr, sal_config_video_s* cfg)
{
    while (attr)
    {
        const char* attr_name = xml_attr_name(attr);
        const char* attr_text = xml_attr_text(attr);
        if (!strcmp(attr_name, "type"))
        {
            strcpy(cfg->type, attr_text);
        }
        else if (!strcmp(attr_name, "resolution"))
        {
            strcpy(cfg->resolution, attr_text);
        }
        else if (!strcmp(attr_name, "width"))
        {
            cfg->width = strtod(attr_text, NULL);
        }
        else if (!strcmp(attr_name, "height"))
        {
            cfg->height = strtod(attr_text, NULL);
        }
        else if (!strcmp(attr_name, "profile"))
        {
            strcpy(cfg->profile, attr_text);
        }
        else if (!strcmp(attr_name, "fps"))
        {
            cfg->fps = strtod(attr_text, NULL);
        }
        else if (!strcmp(attr_name, "gop"))
        {
            cfg->gop = strtod(attr_text, NULL);
        }
        else if (!strcmp(attr_name, "bitrate"))
        {
            cfg->bitrate = strtod(attr_text, NULL);
        }
        else if (!strcmp(attr_name, "bitrate_ctrl"))
        {
            strcpy(cfg->bitrate_ctrl, attr_text);
        }
        else
        {
            WRN("not support node[%s]\n", attr_name);
        }
        attr = xml_attr_next(attr);
    }
    return 0;
}

int sal_config_parse(char* path, sal_config_s* config)
{
    CHECK(path && config, -1, "invalid parameter\n");

    handle_node node = NULL;

    handle_doc doc = xml_load_file(path);
    CHECK(doc, -1, "error with %#x\n", doc);

    handle_node root_node = xml_node_root(doc);
    CHECK(root_node, -1, "error with %#x\n", root_node);

    node = xml_node_child(root_node);
    while (node)
    {
        const char* node_name = xml_node_name(node);
        if (!strcmp(node_name, "version"))
        {
            strcpy(config->version, xml_node_text(node));
        }
        else if (!strcmp(node_name, "video_0"))
        {
            handle_attr* attr = xml_attr_child(node);
            sal_config_vattr_parse(attr, &config->video[0]);
        }
        else if (!strcmp(node_name, "video_1"))
        {
            handle_attr* attr = xml_attr_child(node);
            sal_config_vattr_parse(attr, &config->video[1]);
        }
        else
        {
            WRN("not support node[%s]\n", node_name);
        }
        node = xml_node_next(node);
    }

    xml_unload(doc);
    return 0;
}


/*
int config_parse_data()
{
    //char* p = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><config><version>1.0.0</version></config>";
    char* p = "<config><version>1.0.0</version></config>";

    handle_doc doc = xml_load_data(p);
    handle_node root_node = xml_node_root(doc);
    printf("%s\n", xml_node_name(root_node));
    handle_node node = xml_node_child(root_node);
    printf("%s\n", xml_node_name(node));
    printf("%s\n", xml_node_text(node));

    return 0;
}
*/


