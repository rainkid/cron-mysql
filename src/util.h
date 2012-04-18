#ifndef UTIL_H_
#define UTIL_H_

time_t GetNowTime();
void my_free(void * p);
void * my_malloc(size_t size);
void write_log(const char *fmt,  ...);
char * string_copy(char *dest, char *src);

#define MINUTE 60
#define HOUR (60*MINUTE)
#define DAY (24*HOUR)
#define YEAR (365*DAY)
static int month[12] = {
  0,
  DAY * (31),
  DAY * (31 + 29),
  DAY * (31 + 29 + 31),
  DAY * (31 + 29 + 31 + 30),
  DAY * (31 + 29 + 31 + 30 + 31),
  DAY * (31 + 29 + 31 + 30 + 31 + 30),
  DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31),
  DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
  DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
  DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
  DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
};
#endif /* UTIL_H_ */
