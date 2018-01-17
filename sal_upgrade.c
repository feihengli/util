
#include "sal_debug.h"
#include "sal_list.h"
#include "sal_malloc.h"
#include "sal_util.h"
#include "sal_mtd.h"
#include "sal_curl.h"
#include "libtinyxml/xml.h"
#include "libmd5/sal_md5.h"
#include "sal_upgrade.h"

#define VERSION_PATH "/etc/version.txt"

typedef struct file_info_s
{
   int valid;
   char name[256];
   char partition[256];
   char type[256];
   int len;
   char md5[256];
   char downloadUrl[256];
}file_info_s;

typedef struct sal_upgrade_args_s
{
    int enable;
    int updated;
    version_s version;
    file_info_s file[8];

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_upgrade_args_s;

static sal_upgrade_args_s* g_upgrade_args = NULL;

static int upgrade_GetHttpCode(char* input)
{
    int s32Ret = -1;
    char* szTmp = strstr(input, "HTTP/1.1 ");
    if(szTmp != NULL)
    {
        s32Ret = atoi(szTmp + strlen("HTTP/1.1 "));
    }
    return s32Ret;
}

static int upgrade_backup_flag(char* mtd)
{
    int ret = 0;
    char flag[8];
    memset(flag, 0, sizeof(flag));
    strcpy(flag, "WORK");

    ret = sal_mtd_writeflag(mtd, flag, strlen(flag));
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    memset(flag, 0, sizeof(flag));
    ret = sal_mtd_readflag(mtd, flag, sizeof(flag));
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    return 0;
}

static int upgrade_mtd(int idx)
{
    int ret = -1;
    file_info_s* pfile = &g_upgrade_args->file[idx];
    int timeout = 30*1000;

    handle g_hndCurlWrapper = curl_wrapper_init(timeout);
    CHECK(g_hndCurlWrapper, -1, "Error with: %#x\n", g_hndCurlWrapper);

    ret = curl_wrapper_StartDownload2Mem(g_hndCurlWrapper, pfile->downloadUrl, NULL, NULL);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    CURL_STATUS_S stStatus;
    memset(&stStatus, 0, sizeof(stStatus));

    while (g_upgrade_args->running)
    {
        ret = curl_wrapper_GetProgress(g_hndCurlWrapper, &stStatus);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        DBG("DownloadProgress: %d\n", stStatus.u32Progress);

        if (!stStatus.bRunning)
        {
            break;
        }

        usleep(1000*1000);
    }

    if (stStatus.enResult == CURL_RESULT_OK && 200 == upgrade_GetHttpCode((char*)stStatus.pu8Recv))
    {
        DBG("pu8Recv: %s\n", stStatus.pu8Recv);
        DBG("u32RecvHeaderTotal: %d\n", stStatus.u32RecvHeaderTotal);
        DBG("u32RecvBodyTotal: %d\n", stStatus.u32RecvBodyTotal);

        char* output = NULL;
        int len = 0;
        ret = curl_wrapper_Download2MemGet(g_hndCurlWrapper, &output, &len);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        unsigned char digest[64];
        memset(digest, 0, sizeof(digest));
        md5_memsum(output, len, digest);
        DBG("md5: %s\n", digest);

        if (!memcmp(digest, pfile->md5, strlen((char*)digest)))
        {
            ret = sal_mtd_rerase(pfile->partition);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            ret = sal_mtd_write(output, len, pfile->partition);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            ret = sal_mtd_verify(output, len, pfile->partition);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);

            if (strstr(pfile->partition, "mtd1")
                || strstr(pfile->partition, "mtd2")
                || strstr(pfile->partition, "mtd3"))
            {
                ret = upgrade_backup_flag(pfile->partition);
                CHECK(ret == 0, -1, "Error with: %#x\n", ret);
            }
        }
    }

    ret = curl_wrapper_Stop(g_hndCurlWrapper);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = curl_wrapper_destroy(g_hndCurlWrapper);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    return 0;
}

static int upgrade_parse_fileinfo(handle_node hndnode, int idx)
{
    file_info_s* pfile = &g_upgrade_args->file[idx];
    memset(pfile, 0, sizeof(*pfile));
    handle_node node = xml_node_child(hndnode);
    while (node)
    {
        const char* node_name = xml_node_name(node);
        if (!strcmp(node_name, "valid"))
        {
            pfile->valid = atoi(xml_node_text(node));
        }
        else if (!strcmp(node_name, "name"))
        {
            strcpy(pfile->name, xml_node_text(node));
        }
        else if (!strcmp(node_name, "partition"))
        {
            strcpy(pfile->partition, xml_node_text(node));
        }
        else if (!strcmp(node_name, "type"))
        {
            strcpy(pfile->type, xml_node_text(node));
        }
        else if (!strcmp(node_name, "length"))
        {
            pfile->len = atoi(xml_node_text(node));;
        }
        else if (!strcmp(node_name, "md5"))
        {
            strcpy(pfile->md5, xml_node_text(node));
        }
        else if (!strcmp(node_name, "downloadUrl"))
        {
            strcpy(pfile->downloadUrl, xml_node_text(node));
        }
        else
        {
            WRN("not support node[%s]\n", node_name);
        }
        node = xml_node_next(node);
    }

    return 0;
}

static int upgrade_parse_all(char* path)
{
    int ret = -1;
    handle_node node = NULL;

    handle_doc doc = xml_load_file(path);
    CHECK(doc, -1, "error with %#x\n", doc);

    handle_node root_node = xml_node_root(doc);
    CHECK(root_node, -1, "error with %#x\n", root_node);

    node = xml_node_child(root_node);
    while (node)
    {
        const char* node_name = xml_node_name(node);
        if (!strcmp(node_name, "version_info"))
        {
            strcpy(g_upgrade_args->version.remote, xml_node_text(node));
        }
        else if (!strcmp(node_name, "file0"))
        {
            ret = upgrade_parse_fileinfo(node, 0);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file1"))
        {
            ret = upgrade_parse_fileinfo(node, 1);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file2"))
        {
            ret = upgrade_parse_fileinfo(node, 2);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file3"))
        {
            ret = upgrade_parse_fileinfo(node, 3);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file4"))
        {
            ret = upgrade_parse_fileinfo(node, 4);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file5"))
        {
            ret = upgrade_parse_fileinfo(node, 5);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
        }
        else if (!strcmp(node_name, "file6"))
        {
            ret = upgrade_parse_fileinfo(node, 6);
            CHECK(ret == 0, -1, "Error with: %#x\n", ret);
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

static int upgrade_show_all()
{
    DBG("bUpdate: %d\n", g_upgrade_args->version.bUpdate);
    DBG("local: %s\n", g_upgrade_args->version.local);
    DBG("remote: %s\n", g_upgrade_args->version.remote);

    unsigned int i = 0;
    for (i = 0; i < sizeof(g_upgrade_args->file)/sizeof(g_upgrade_args->file[0]); i++)
    {
         DBG("file%d valid: %d\n", i, g_upgrade_args->file[i].valid);
         DBG("file%d name: %s\n", i, g_upgrade_args->file[i].name);
         DBG("file%d partition: %s\n", i, g_upgrade_args->file[i].partition);
         DBG("file%d type: %s\n", i, g_upgrade_args->file[i].type);
         DBG("file%d len: %d\n", i, g_upgrade_args->file[i].len);
         DBG("file%d md5: %s\n", i, g_upgrade_args->file[i].md5);
         DBG("file%d downloadUrl: %s\n", i, g_upgrade_args->file[i].downloadUrl);
    }

    return 0;
}

static int upgrade_IsMajorPartion()
{
    int bret = 0;
    char buf[512];
    memset(buf, 0, sizeof(buf));

    snprintf(buf, sizeof(buf), "cat /proc/cmdline");
    DBG("run cmd: %s\n", buf);

    FILE* fp = popen(buf, "r");
    CHECK(fp, -1, "Error with %s.\n", strerror(errno));

    memset(buf, 0, sizeof(buf));
    int size = fread(buf, 1, sizeof(buf), fp);
    if (size > 0)
    {
        if (strstr(buf, "/dev/mtdblock2"))
        {
            bret = 1;
        }
    }

    pclose(fp);

    return bret;
}

static int upgrade_local_version_set(char* input)
{
    int ret = -1;

    FILE* fp = fopen(VERSION_PATH, "wb+");
    CHECK(fp, -1, "Error with: %s.\n", strerror(errno));

    ret = fwrite(input, 1, strlen(input), fp);
    CHECK(ret > 0, -1, "Error with: %s.\n", strerror(errno));

    fclose(fp);
    sync();

    return 0;
}

static int upgrade_local_version_get(char* output)
{
    int ret = -1;

    strcpy(output, "1.0.0.0");
    if (!access(VERSION_PATH, F_OK))
    {
        FILE* fp = fopen(VERSION_PATH, "r");
        CHECK(fp, -1, "Error with %s.\n", strerror(errno));

        ret = fread(output, 1, 256, fp);
        CHECK(ret > 0, -1, "Error with %s.\n", strerror(errno));

        fclose(fp);
    }

    if (!upgrade_IsMajorPartion())
    {
        WRN("this is backup partion, need to upgrade.reset version\n");
        strcpy(output, "1.0.0.0");

        ret = upgrade_local_version_set(output);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
    }

    return 0;
}

static void* upgrade_proc(void* args)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    int ret = -1;
    unsigned int i = 0;

    while (g_upgrade_args->running)
    {
        pthread_mutex_lock(&g_upgrade_args->mutex);

        if (g_upgrade_args->enable)
        {
            g_upgrade_args->enable = 0;
            for (i = 0; i < sizeof(g_upgrade_args->file)/sizeof(g_upgrade_args->file[0]); i++)
            {
                if (g_upgrade_args->file[i].valid)
                {
                    g_upgrade_args->updated = 1;
                    ret = upgrade_mtd(i);
                    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);
                }
            }

        }

        if (g_upgrade_args->updated)
        {
            ret = upgrade_local_version_set(g_upgrade_args->version.remote);
            CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

            //后续需用硬看门狗代替软重启
            DBG("run cmd: reboot. delay 1 second\n");
            sleep(1);
            system("reboot");
        }

        pthread_mutex_unlock(&g_upgrade_args->mutex);
        usleep(200*1000);
     }

    return NULL;
}

int sal_upgrade_init()
{
    CHECK(NULL == g_upgrade_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;
    g_upgrade_args = (sal_upgrade_args_s*)mem_malloc(sizeof(sal_upgrade_args_s));
    CHECK(NULL != g_upgrade_args, -1, "malloc %d bytes failed.\n", sizeof(sal_upgrade_args_s));

    memset(g_upgrade_args, 0, sizeof(sal_upgrade_args_s));
    pthread_mutex_init(&g_upgrade_args->mutex, NULL);


    g_upgrade_args->running = 1;
    ret = pthread_create(&g_upgrade_args->pid, NULL, upgrade_proc, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_upgrade_exit()
{
    CHECK(NULL != g_upgrade_args, -1,"moudle is not inited.\n");

    //int ret = -1;
    if (g_upgrade_args->running)
    {
        g_upgrade_args->running = 0;
        pthread_join(g_upgrade_args->pid, NULL);
    }

    pthread_mutex_destroy(&g_upgrade_args->mutex);
    mem_free(g_upgrade_args);
    g_upgrade_args = NULL;

    return 0;
}

int sal_upgrade_check(char* urlCfg, int timeout, version_s* version_info)
{
    CHECK(NULL != g_upgrade_args, -1, "moudle is not inited.\n");
    CHECK(urlCfg, -1, "invalid parameter with: %#x.\n", urlCfg);
    CHECK(timeout > 0, -1, "invalid parameter with: %#x.\n", timeout);
    CHECK(version_info, -1, "invalid parameter with: %#x.\n", version_info);

    int ret = -1;
    char* cfg_path = "/tmp/upgrade.xml";

    handle g_hndCurlWrapper = curl_wrapper_init(timeout);
    CHECK(g_hndCurlWrapper, -1, "Error with: %#x\n", g_hndCurlWrapper);

    ret = curl_wrapper_StartDownload(g_hndCurlWrapper, urlCfg, NULL, NULL, cfg_path);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    CURL_STATUS_S stStatus;
    memset(&stStatus, 0, sizeof(stStatus));

    while (g_upgrade_args->running)
    {
        ret = curl_wrapper_GetProgress(g_hndCurlWrapper, &stStatus);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        DBG("DownloadProgress: %d\n", stStatus.u32Progress);

        if (!stStatus.bRunning)
        {
            break;
        }

        usleep(100*1000);
    }

    if (stStatus.enResult == CURL_RESULT_OK
        && 200 == upgrade_GetHttpCode((char*)stStatus.pu8Recv))
    {
        ret = upgrade_parse_all(cfg_path);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        ret = upgrade_local_version_get(g_upgrade_args->version.local);
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        g_upgrade_args->version.bUpdate = 0;
        if (strcmp(g_upgrade_args->version.local, g_upgrade_args->version.remote))
        {
            g_upgrade_args->version.bUpdate = 1;
        }

        ret= upgrade_show_all();
        CHECK(ret == 0, -1, "Error with: %#x\n", ret);

        memcpy(version_info, &g_upgrade_args->version, sizeof(g_upgrade_args->version));
    }

    ret = curl_wrapper_Stop(g_hndCurlWrapper);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = curl_wrapper_destroy(g_hndCurlWrapper);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    return 0;
}

int sal_upgrade_enable(int enable)
{
    CHECK(NULL != g_upgrade_args, -1, "moudle is not inited.\n");

    g_upgrade_args->enable = enable;

    return 0;
}

