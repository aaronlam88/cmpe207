/*------------------------------------------------------------------------
* input.c - get user input and do error checking
*------------------------------------------------------------------------
*/
#include "input.h"

void logger (const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
} 

void get_validate (const char* prome, char* get, const char* logger_format) {
    if(prome != NULL && strlen(prome) > 0) {
        printf("%s\n", prome);
    }
    scanf("%s", get);

    /*********************************************
     * verify: is valid get
     *********************************************/
    int error_flag = 0;
    int len = strlen(get);
    //is get len between 5 and 40
    if(len < 5 || len > 40) {
        error_flag = 1;
        logger(logger_format, "ERROR", "invalid length");
    }
    // is get contend invalid char
    char* c = strpbrk(get, INVALIDCHAR);
    if( c != NULL) {
        error_flag = 1;
        logger(logger_format, "ERROR", "contend invalid character");
        logger(logger_format, "INVALICHAR", c);
    }
    // if error, set get to NULL
    if(error_flag)  {
        logger(logger_format, "ERROR", "result is NULL\n");
        get = NULL;
    } else {
        logger(logger_format, "INFO", "DONE\n");
    }
    return;
}

void  getInt (const char* prome, int* get) {
    if(prome != NULL && strlen(prome) > 0) {
        printf("%s\n", prome);
    }
    scanf("%d", get);

    return;
}

void  getLong (const char* prome, long* get) {
    if(prome != NULL && strlen(prome) > 0) {
        printf("%s\n", prome);
    }
    scanf("%ld", get);

    return;
}

void getString (const char* prome, char* get) {
    if(prome != NULL && strlen(prome) > 0) {
        printf("%s\n", prome);
    }
    scanf("%s", get);

    return;
}

void getPassword (const char* prome, char* password) {
    const char* logger_format = "getPassword [%s] %s\n";
    get_validate(prome, password, logger_format);
    return;
}

void getUsername (const char* prome, char* username) {
    const char* logger_format = "getUsername [%s] %s\n";
    get_validate(prome, username, logger_format);
    return;
}