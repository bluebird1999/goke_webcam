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
#include "ntpc.h"

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

#define NTPC_CHECK(exp, ret, fmt...) do\
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
	
    NTPC_CHECK(ret == sizeof(data), -1, "Error with: %d: %s\n", ret, strerror(errno));

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

    tv->tv_sec = packet.xmttime.coarse - JAN_1970 + 28800;
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
    NTPC_CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    return 0;
}

int ntpcdate(char *ntp_server)
{
    int ret = -1;
    /* create socket. */
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    NTPC_CHECK(sock > 0, -1, "Error with: %s\n", strerror(errno));

    /* bind local address. */
    struct sockaddr_in addr_src, addr_dst;
    int addr_len = sizeof(struct sockaddr_in);
    memset(&addr_src, 0, sizeof(struct sockaddr_in));
    addr_src.sin_family = AF_INET;
    addr_src.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_src.sin_port = htons(0);

    ret = bind(sock, (struct sockaddr*)&addr_src, addr_len);
    NTPC_CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    /* connect to ntp server. */
    memset(&addr_dst, 0, addr_len);
    addr_dst.sin_family = AF_INET;

    DBG("ntp_server: %s\n", ntp_server);
    struct hostent* host = gethostbyname(ntp_server);
    NTPC_CHECK(host, -1, "Error with: %s\n", strerror(errno));
    memcpy(&(addr_dst.sin_addr.s_addr), host->h_addr_list[0], 4);

    addr_dst.sin_port = htons(NTP_PORT);
    ret = connect(sock, (struct sockaddr*)&addr_dst, addr_len);
    NTPC_CHECK(ret == 0, -1, "Error with: %s\n", strerror(errno));

    ret = ntpc_send_request(sock);
    NTPC_CHECK(ret == 0, -1, "Error with: %d\n", ret);

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
        NTPC_CHECK(ret == sizeof(buf), -1, "Error with: %d: %s\n", ret, strerror(errno));

        /* get local timestamp. */
        ret = ntpc_localtime_timestamp(&arrival_ntp);
        NTPC_CHECK(ret == 0, -1, "Error with: %d\n", ret);

        /* get server's time and print it. */
        ret = ntpc_rfc1305parse(buf, &arrival_ntp, &newtime);
        NTPC_CHECK(ret == 0, -1, "Error with: %d\n", ret);

        /* set local time to the server's time, if you're a root user. */
        ret = ntpc_set_localtime(newtime);
        NTPC_CHECK(ret == 0, -1, "Error with: %d\n", ret);
    }

    close(sock);
    return 0;
}