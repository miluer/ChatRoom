/*--------------------------------------------------------------------*/
/* 服务器端口信息 */
#define PORTLINK ".chatport"

/* 缓存限制 */
#define MAXNAMELEN 256
#define MAXPKTLEN  2048

/* 信息类型的定义 */
#define LIST_GROUPS    0
#define JOIN_GROUP     1
#define LEAVE_GROUP    2
#define USER_TEXT      3
#define JOIN_REJECTED  4
#define JOIN_ACCEPTED  5

/* 数据包结构 */
typedef struct _packet {

  /* 数据包类型 */
  char      type;

  /* 数据包内容长度 */
  long      lent;

  /* 数据包内容 */
  char *    text;

} Packet;

extern int startserver();
extern int locateserver();

extern Packet *recvpkt(int sd);
extern int sendpkt(int sd, char typ, long len, char *buf);
extern void freepkt(Packet *msg);
/*--------------------------------------------------------------------*/
