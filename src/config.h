#ifndef CONFIG_H_
#define CONFIG_H_

int c_get_path(char buf[], char *pFileName);
char *c_get_string(char *title, char *key, char *filename);
int c_get_int(char *title, char *key, char *filename);

#endif /* CONFIG_H_ */
