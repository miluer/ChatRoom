/* 连接服务器和客户机的函数 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "common.h"

/*
  为服务器接收客户端请求做准备，
  正确返回 socket 文件描述符
  错误返回 -1
*/
int startserver()
{
  int     sd;      /* socket 描述符 */
  int     myport;  /* 服务器端口 */
  const char *  myname;  /* 本地主机的全称 */

  char 	  linktrgt[MAXNAMELEN];
  char 	  linkname[MAXNAMELEN];

  /*
	调用 socket 函数创建 TCP socket 描述符
  */
  sd = socket(PF_INET, SOCK_STREAM, 0);

  /*
    调用bind函数将一个本地地址指派给 socket
  */

  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY); /* 通配地址 INADDR_ANY 表示IP地址为 0.0.0.0，
													  内核在套接字被连接后选择一个本地地址
													  htonl函数 用于将 INADDR_ANY 转换为网络字节序 */
  server_address.sin_port = htons(0);  /* 指派为通配端口 0，调用 bind 函数后内核将任意选择一个临时端口 */

  bind(sd, (struct sockaddr *) &server_address, sizeof(server_address));

  /* 调用listen 将服务器端 socket 描述符 sd 设置为被动地监听状态，并设置接受队列的长度为20 */
  listen(sd, 20);

  /*
    调用 getsockname、gethostname 和 gethostbyname 确定本地主机名和服务器端口号
  */

  char hostname[MAXNAMELEN];

  if (gethostname(hostname, sizeof hostname) != 0)
  	perror("gethostname");

  struct hostent* h;
	h = gethostbyname(hostname);

  int len = sizeof(struct sockaddr);

  getsockname(sd, (struct sockaddr *) &server_address, &len);

  myname = h->h_name;
  myport = ntohs(server_address.sin_port);

  /* 在家目录下创建符号链接'.chatport'指向linktrgt */
  sprintf(linktrgt, "%s:%d", myname, myport);
  sprintf(linkname, "%s/%s", getenv("HOME"), PORTLINK); /* 在头文件 common.h 中：
														#define PORTLINK ".chatport" */
  if (symlink(linktrgt, linkname) != 0) {
    fprintf(stderr, "error : server already exists\n");
    return(-1);
  }

  /* 准备接受客户端请求 */
  printf("admin: started server on '%s' at '%d'\n",
	 myname, myport);
  return(sd);
}

/*
  和服务器建立连接，正确返回 socket 描述符，
  失败返回  -1
*/
int hooktoserver()
{
	int sd;                 

	char linkname[MAXNAMELEN];
	char linktrgt[MAXNAMELEN];
	char *servhost;
	char *servport;
	int bytecnt;

  /* 获取服务器地址 */
  sprintf(linkname, "%s/%s", getenv("HOME"), PORTLINK);
  bytecnt = readlink(linkname, linktrgt, MAXNAMELEN);
  if (bytecnt == -1) 
	{
		fprintf(stderr, "error : no active chat server\n");
		return(-1);
	}

	linktrgt[bytecnt] = '\0';

	/* 获得服务器 IP 地址和端口号 */
	servport = index(linktrgt, ':');
	*servport = '\0';
	servport++;
	servhost = linktrgt;

	/* 获得服务器 IP 地址的 unsigned short 形式 */
	unsigned short number = (unsigned short) strtoul(servport, NULL, 0);

	/*
	调用函数 socket 创建 TCP 套接字
	*/

	sd = socket(AF_INET, SOCK_STREAM, 0);
	
	/*
	调用 gethostbyname() 和 connect()连接 'servhost' 的 'servport' 端口
	*/
	struct hostent *hostinfo;
	struct sockaddr_in address;

	hostinfo = gethostbyname(servhost); /* 得到服务器主机名 */
	address.sin_addr = *(struct in_addr *) *hostinfo->h_addr_list;
	address.sin_family = AF_INET;
	address.sin_port = htons(number);

  

	if (connect(sd, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		perror("connecting");
		exit(1);
	}

	/* 连接成功 */
	printf("admin: connected to server on '%s' at '%s'\n",
		servhost, servport);
	return(sd);
}

/* 从内核读取一个套接字的信息 */
int readn(int sd, char *buf, int n)
{
  int     toberead;
  char *  ptr;

  toberead = n;
  ptr = buf;
  while (toberead > 0) {
    int byteread;

    byteread = read(sd, ptr, toberead);
    if (byteread <= 0) {
      if (byteread == -1)
	perror("read");
      return(0);
    }

    toberead -= byteread;
    ptr += byteread;
  }
  return(1);
}

/* 接收数据包 */
Packet *recvpkt(int sd)
{
  Packet *pkt;

  /* 动态分配内存 */
  pkt = (Packet *) calloc(1, sizeof(Packet));
  if (!pkt) {
    fprintf(stderr, "error : unable to calloc\n");
    return(NULL);
  }

  /* 读取消息类型 */
  if (!readn(sd, (char *) &pkt->type, sizeof(pkt->type))) {
    free(pkt);
    return(NULL);
  }

  /* 读取消息长度 */
  if (!readn(sd, (char *) &pkt->lent, sizeof(pkt->lent))) {
    free(pkt);
    return(NULL);
  }
  pkt->lent = ntohl(pkt->lent);

  /* 为消息内容分配空间 */
  if (pkt->lent > 0) {
    pkt->text = (char *) malloc(pkt->lent);
    if (!pkt) {
      fprintf(stderr, "error : unable to malloc\n");
      return(NULL);
    }

    /* 读取消息文本 */
    if (!readn(sd, pkt->text, pkt->lent)) {
      freepkt(pkt);
      return(NULL);
    }
  }
  return(pkt);
}

/* 发送数据包 */
int sendpkt(int sd, char typ, long len, char *buf)
{
  char tmp[8];
  long siz;

  /* 把包的类型和长度写入套接字 */
  bcopy(&typ, tmp, sizeof(typ));
  siz = htonl(len);
  bcopy((char *) &siz, tmp+sizeof(typ), sizeof(len));
  write(sd, tmp, sizeof(typ) + sizeof(len));

  /* 把消息文本写入套接字 */
  if (len > 0)
    write(sd, buf, len);
  return(1);
}

/* 释放数据包占用的内存空间 */
void freepkt(Packet *pkt)
{
  free(pkt->text);
  free(pkt);
}
