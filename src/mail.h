#ifndef	__SEND_MAIL_H
#define __SEND_MAIL_H

#include <stdio.h>
/* html格式发送mail */
#define	HTML_STYLE_MAIL    0x1
/* 发送mail需要认证 */
#define	AUTH_SEND_MAIL    0x1

#define SEND_RESULT_SUCCESS    0
#define SEND_RESULT_OPEN_SOCK_FINAL    0x1
#define SEND_RESULT_CONNECT_FINAL   0x2
#define SEND_RESULT_FINAL    0x3

#define READ_FILE_LEN	1024

#define PROTOCOL "tcp"

struct st_char_arry{
  char *str_p;
};
struct st_mail_msg_{
 /* 接收地址个数 */
 int to_addr_len;
 int bc_addr_len;
 /* 秘送地址数 */
 int cc_addr_len;
 int att_file_len;
 /* 优先权 */
 int priority;
 /* 商品 */
 int port;
 /* 是否需要认证 1 y*/
 int authorization;
 /* 信体是否是html格式 */
 int mail_style_html;
 /* 邮件服务器ip或host*/
 char *server;
 /* 标题 */
 char *subject;
 /* 文本正文 */
 char *content;
 /* 需要认证的帐号 */
 char *auth_user;
 /* 需要认证的密码 */
 char *auth_passwd;
 /* 字符编码 */
 char *charset;
 /* 发送地址 */
 char *from;
 /* 收信人看到的地址，如果不设置则为from */
 char *from_subject;
 /* 附件数组file path*/
 struct st_char_arry *att_file_ary;
 /* 秘送 */
 struct st_char_arry *cc_address_ary;
 /* 抄送人地址数组 */
 struct st_char_arry *bc_address_ary;
 /* 接收人地址 */
 struct st_char_arry *to_address_ary;
};
/* 初始化结构 */
void init_mail_msg(struct st_mail_msg_ *msg);
/* 发送mail */
int send_mail(struct st_mail_msg_ *msg_);

int send_mail_header(int sockfd,struct st_mail_msg_ *msg);
int cmd_msg(int sockfd,const char *cmd,const char *flag);
#endif
//__SEND_MAIL_H

