#ifndef TOOL_H_
#define TOOL_H_

#ifdef	__cplusplus
extern "C" {
#endif

char* substr(const char*str,unsigned start, unsigned end);
time_t GetNowTime();
size_t Curl_Callback(void *ptr, size_t size, size_t nmemb, void *data);
void Curl_Request(char * query_url);

#ifdef	__cplusplus
}
#endif

#endif /* TOOL_H_ */
