#ifdef VKFLUID_WIN32

#include "pez.h"
#include "bstrlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <wchar.h>


void pezPrintStringW(const wchar_t* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
}

void pezPrintString(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
}

void pezFatalW(const wchar_t* pStr, ...)
{
    fwide(stderr, 1);

    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
    exit(1);
}

void _pezFatal(const char* pStr, va_list a)
{
    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void pezFatal(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);
    _pezFatal(pStr, a);
}

void pezCheck(int condition, ...)
{
    va_list a;
    const char* pStr;

    if (condition)
        return;

    va_start(a, condition);
    pStr = va_arg(a, const char*);
    _pezFatal(pStr, a);
}

void pezCheckPointer(void* p, ...)
{
    va_list a;
    const char* pStr;

    if (p != NULL)
        return;

    va_start(a, p);
    pStr = va_arg(a, const char*);
    _pezFatal(pStr, a);
}

int pezIsPressing(char key)
{
    return 0;
}

const char* pezResourcePath()
{
    return ".";
}


#endif//VKFLUID_WIN32
