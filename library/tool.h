#ifndef TOOL_H_
#define TOOL_H_

#ifdef	__cplusplus
extern "C" {
#endif

char* substr(const char*str,unsigned start, unsigned end);
time_t GetNowTime();
int send_notice_mail(char *subject, char *content);
int send_notice_sms(char *url, char *subject, char *content);

#ifdef	__cplusplus
}
#endif

#endif /* TOOL_H_ */
