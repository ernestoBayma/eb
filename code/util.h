#pragma once

#include <stddef.h>
#include <stdio.h>

#include "core.h"
#include "str.h"
#include "os.h"

// Functions
int memcmp_nocase(char *str1, char *str2, size_t amt);
int cstr_len(char *ptr);
int cstr_to_lower(int c);
bool humanRedableFileRequestError(Str str, enum FileRequestErrors error);
void memoryCopy(void *pa, void *pb, s64 amt);
void zeroMemory(void *ptr, s64 amt);
void printUsage(char *prog_name, char*version, FILE *out, KeyValue *values);
