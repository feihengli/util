#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/un.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* ntp server list
pool.ntp.org
210.72.145.44      nok (中国国家授时中心服务器IP地址)
cn.pool.ntp.org    ok
time.windows.com   nok
time.nist.gov      nok
cn.ntp.org.cn       ok
time.buptnet.edu.cn

ntp1.aliyun.com
ntp2.aliyun.com
ntp3.aliyun.com
ntp4.aliyun.com
ntp5.aliyun.com
ntp6.aliyun.com
ntp7.aliyun.com

time.windows.com   微软

asia.pool.ntp.org  台湾
time.nist.gov   美国
ntp.fudan.edu.cn 复旦
timekeeper.isi.edu
subitaneous.cpsc.ucalgary.ca
usno.pa-x.dec.com
time.twc.weather.com
swisstime.ethz.ch
ntp0.fau.de
ntp3.fau.de
time-a.nist.gov
time-b.nist.gov
time-nw.nist.gov
nist1-sj.glassey.com
ntp.sjtu.edu.cn 202.120.2.101 (上海交通大学网络中心NTP服务器地址）
s1a.time.edu.cn 北京邮电大学
s1b.time.edu.cn 清华大学
s1c.time.edu.cn 北京大学
s1d.time.edu.cn 东南大学
s1e.time.edu.cn 清华大学
s2a.time.edu.cn 清华大学
s2b.time.edu.cn 清华大学
s2c.time.edu.cn 北京邮电大学
s2d.time.edu.cn 西南地区网络中心
s2e.time.edu.cn 西北地区网络中心
s2f.time.edu.cn 东北地区网络中心
s2g.time.edu.cn 华东南地区网络中心
s2h.time.edu.cn 四川大学网络管理中心
s2j.time.edu.cn 大连理工大学网络中心
s2k.time.edu.cn CERNET桂林主节点
s2m.time.edu.cn 北京大学
133.100.11.8  日本 福冈大学
time-a.nist.gov 129.6.15.28 NIST, Gaithersburg, Maryland
time-b.nist.gov 129.6.15.29 NIST, Gaithersburg, Maryland
time-a.timefreq.bldrdoc.gov 132.163.4.101 NIST, Boulder, Colorado
time-b.timefreq.bldrdoc.gov 132.163.4.102 NIST, Boulder, Colorado
time-c.timefreq.bldrdoc.gov 132.163.4.103 NIST, Boulder, Colorado
utcnist.colorado.edu 128.138.140.44 University of Colorado, Boulder
time.nist.gov 192.43.244.18 NCAR, Boulder, Colorado
time-nw.nist.gov 131.107.1.10 Microsoft, Redmond, Washington
nist1.symmetricom.com 69.25.96.13 Symmetricom, San Jose, California
nist1-dc.glassey.com 216.200.93.8 Abovenet, Virginia
nist1-ny.glassey.com 208.184.49.9 Abovenet, New York City
nist1-sj.glassey.com 207.126.98.204 Abovenet, San Jose, California
nist1.aol-ca.truetime.com 207.200.81.113 TrueTime, AOL facility, Sunnyvale, California
nist1.aol-va.truetime.com 64.236.96.53 TrueTime, AOL facility, Virginia
*/


#define NTP_SERVER    "pool.ntp.org"
#define NTP_PORT      123
#define JAN_1970      0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */

#define NTPFRAC(x) (4294 * (x) + ((1981 * (x))>>11))
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

#define DBG(fmt...) do\
{\
    printf("%s: %d: ", __FUNCTION__, __LINE__);\
    printf(fmt);\
}while(0)

#define CHECK(exp, ret, fmt...) do\
{\
    if (!(exp))\
    {\
        DBG(fmt);\
        return ret;\
    }\
}while(0)


struct ntptime
{
    unsigned int coarse;
    unsigned int fine;
};

struct ntp_packet
{
    unsigned int li;
    unsigned int vn;
    unsigned int mode;
    unsigned int stratum;
    unsigned int poll;
    unsigned int prec;
    unsigned int delay;
    unsigned int disp;
    unsigned int refid;
    struct ntptime reftime;
    struct ntptime orgtime;
    struct ntptime rectime;
    struct ntptime xmttime;
};

static int ntpc_send_request(int fd)
{
    unsigned int data[12];
    struct timeval now = {0, 0};
    int ret = -1;

    memset((char*)data, 0, sizeof(data));
    data[0] = htonl((LI << 30) | (VN << 27) | (MODE << 24)
                  | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
    data[1] = htonl(1<<16);  /* Root Delay (seconds) */
    data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
    gettimeofday(&now, NULL);
    data[10] = htonl(now.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
    data[11] = htonl(NTPFRAC(now.tv_usec));  /* Transmit Timestamp fine   */

    ret = send(fd, data, sizeof(data), 0);
    CHECK(ret == sizeof(data), -1, "Error with: %d: %s\n", ret, strerror(errno));

    return 0;
}

static int ntpc_localtime_timestamp(struct ntptime *udp_arrival_ntp)
{
    struct timeval udp_arrival;
    gettimeofday(&udp_arrival, NULL);
    udp_arrival_ntp->coarse = udp_arrival.tv_sec + JAN_1970;
    udp_arrival_ntp->fine   = NTPFRAC(udp_arrival.tv_usec);

    return 0;
}

static int ntpc_rfc1305parse(unsigned int *data, struct ntptime *arrival, struct timeval* tv)
{
    struct ntp_packet packet;
    memset(&packet, 0, sizeof(packet));

#define Data(i) ntohl(((unsigned int *)data)[i])
    packet.li      = Data(0) >> 30 & 0x03;
    packet.vn      = Data(0) >> 27 & 0x07;
    packet.mode    = Data(0) >> 24 & 0x07;
    packet.stratum = Data(0) >> 16 & 0xff;
    packet.poll    = Data(0) >>  8 & 0xff;
    packet.prec    = Data(0)       & 0xff;
    if (packet.prec & 0x80)
    {
        packet.prec |= 0xffffff00;
    }
    packet.delay   = Data(1);
    packet.disp    = Data(2);
    packet.refid   = Data(3);
    packet.reftime.coarse = Data(4);
    packet.reftime.fine   = Data(5);
    packet.orgtime.coarse = Data(6);
    packet.orgtime.fine   = Data(7);
    packet.rectime.coarse = Data(8);
    packet.rectime.fine   = Data(9);
    packet.xmttime.coarse = Data(10);
    packet.xmttime.fine   = Data(11);
#undef Data

    tv->tv_sec = packet.xmttime.coarse - JAN_1970;
    tv->tv_usec = USEC(packet.xmttime.fine);

    char time[32] = "";
    strftime(time, sizeof(time), "%F %H:%M:%S %A", localtime(&tv->tv_sec));
    DBG("ntp server response: %s\n", time);

    return 0;
}

static int ntpc_set_localtime(struct timeval tv)
{
    int ret = -1;

    /* need root user. */
    if (0 != getuid() && 0 != geteuid())
    {
        DBG("current user is not root.\n");
        return -1;
    }

    ret = settimeofday(&tv, NULL);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    return 0;
}

int main(int argc, char** argv)
{
    char ntp_server[32];
    memset(ntp_server, 0, sizeof(ntp_server));
    strcpy(ntp_server, NTP_SERVER);

    if (argc == 2)
    {
        memset(ntp_server, 0, sizeof(ntp_server));
        strcpy(ntp_server, argv[1]);
    }

    int ret = -1;
    /* create socket. */
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    CHECK(sock > 0, -1, "Error with: %s\n", strerror(errno));

    /* bind local address. */
    struct sockaddr_in addr_src, addr_dst;
    int addr_len = sizeof(struct sockaddr_in);
    memset(&addr_src, 0, sizeof(struct sockaddr_in));
    addr_src.sin_family = AF_INET;
    addr_src.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_src.sin_port = htons(0);

    ret = bind(sock, (struct sockaddr*)&addr_src, addr_len);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    /* connect to ntp server. */
    memset(&addr_dst, 0, addr_len);
    addr_dst.sin_family = AF_INET;

    DBG("ntp_server: %s\n", ntp_server);
    struct hostent* host = gethostbyname(ntp_server);
    CHECK(host, -1, "Error with: %s\n", strerror(errno));
    memcpy(&(addr_dst.sin_addr.s_addr), host->h_addr_list[0], 4);

    addr_dst.sin_port = htons(NTP_PORT);
    ret = connect(sock, (struct sockaddr*)&addr_dst, addr_len);
    CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    ret = ntpc_send_request(sock);
    CHECK(ret == 0, -1, "Error with: %d\n", ret);

    fd_set fds_read;
    FD_ZERO(&fds_read);
    FD_SET(sock, &fds_read);
    struct timeval timeout = {2, 0};

    unsigned int buf[12];
    memset(buf, 0, sizeof(buf));

    struct sockaddr server;
    memset(&server, 0, sizeof(struct sockaddr));
    socklen_t svr_len = sizeof(struct sockaddr);
    struct ntptime arrival_ntp;
    memset(&arrival_ntp, 0, sizeof(arrival_ntp));
    struct timeval newtime = {0, 0};

    ret = select(sock + 1, &fds_read, NULL, NULL, &timeout);
    if (ret < 0)
    {
        DBG("select error with: %s\n", strerror(errno));
        return -1;
    }
    else if (ret == 0)
    {
        DBG("select timeout\n");
        return -1;
    }

    if (FD_ISSET(sock, &fds_read))
    {
        /* recv ntp server's response. */
        ret = recvfrom(sock, buf, sizeof(buf), 0, &server, &svr_len);
        CHECK(ret == sizeof(buf), -1, "Error with: %d: %s\n", ret, strerror(errno));

        /* get local timestamp. */
        ret = ntpc_localtime_timestamp(&arrival_ntp);
        CHECK(ret == 0, -1, "Error with: %d\n", ret);

        /* get server's time and print it. */
        ret = ntpc_rfc1305parse(buf, &arrival_ntp, &newtime);
        CHECK(ret == 0, -1, "Error with: %d\n", ret);

        /* set local time to the server's time, if you're a root user. */
        ret = ntpc_set_localtime(newtime);
        CHECK(ret == 0, -1, "Error with: %d\n", ret);
    }

    close(sock);
    return 0;
}

