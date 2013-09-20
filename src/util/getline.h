#ifndef GETLINE_H_
#define GETLINE_H_

#include "../config.h"
#ifndef _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp);
ssize_t getline(char** buf, size_t* bufsiz, FILE* fp);


#endif
#endif /* GETLINE_H_ */
