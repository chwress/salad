#ifndef UTIL_GETLINE_H_
#define UTIL_GETLINE_H_

#include "config.h"
#if (_XOPEN_SOURCE -0) < 700

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp);
ssize_t getline(char** buf, size_t* bufsiz, FILE* fp);


#else
#include <stdio.h>
#endif

#endif /* UTIL_GETLINE_H_ */
