#ifndef UTIL_H_
#define UTIL_H_

void write_log(const char *fmt,  ...);
void p_strcpy(char **dest, const char *src);
void spr_strcpy(char **dest, char *src);
void spr_strcpy_fmt(char **dest, const char *fmt, ...);
void print_error(const char *fmt, ...);

#define var_free(p) var_free(p)
#define var_malloc(p) var_malloc(p)

#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)
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
