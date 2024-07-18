/*
 ** Copyright (c) 2024 Bluestone Consulting Group, LLC
 ** Copyright (c) 2024 Rob Seward <rseward@bluestone-consulting.com>
 **
 ** This program is free software; please use, redistribute and modify it under the terms of
 ** the BSD license. (Chosen to be compatible with the libcfu library).
 **
 ** If you modify it for a specific purpose please consider making a pull request here:
 **
 ** Create an issue at the project for consideration to merge the pull request.
 */

#include "models.h"

int mysystem(char *strbuf, char *cmd, int buffer_size);
char *strtrim(char *str);
char* substr(const char* str, int start, int length, char *sbuf);
void sf_showresults(sumfiles_t *self, sumentry_t *results, size_t result_size);
char *se_show(sumfiles_t *self, sumentry_t *entry, char *sbufentry);
char *show_size(char *strbuf, size_t bytes);

