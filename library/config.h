/*
 * config.h
 *
 *  Created on: 2011-11-16
 *      Author: rainkid
 */

#ifndef CONFIG_H_
#define CONFIG_H_

int c_get_path(char buf[], char *pFileName);
char *c_get_string(char *title, char *key, char *filename);
int c_get_init(char *title, char *key, char *filename);

#endif /* CONFIG_H_ */
