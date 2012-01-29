#ifndef TOOL_H_
#define TOOL_H_

#ifdef	__cplusplus
extern "C" {
#endif
time_t GetNowTime();
void write_log(const char *fmt,  ...);
#ifdef	__cplusplus
}
#endif

#endif /* TOOL_H_ */
