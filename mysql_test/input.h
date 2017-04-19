/*------------------------------------------------------------------------
* input.h - 
*------------------------------------------------------------------------
*/

#ifndef INPUT_H_
#define INPUT_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char* INVALIDCHAR = ".%?* ";

void logger (const char *format, ...);

void get_validate (const char* prome, char* get, const char* logger_format);

void getInt (const char* prome, int* get);

void getLong (const char* prome, long* get);

void getString (const char* prome, char* get);

void getPassword (const char* prome, char* password);

void getUsername (const char* prome, char* username);

#endif