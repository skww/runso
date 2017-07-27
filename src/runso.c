/*
 * runso.c
 *
 * 2006-19-12 : jyf
 *
 * Type of Parameters
 *
 *  c - char        C,S - char *
 *  i - int         I - int *
 *  l - long        L - long *
 *  v - void        V,P - void *
 *
 * 2006-19-12 : support x86
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

void show_params(void **pv, char *params_type, int size)
{
    int i, n = 0;

    printf("parameters: (%d)\n", size);

    for (i=size-1; i>=0; i--, n++) {
        printf("  %d: 0x%08x : ", i, (unsigned int)pv[i]);
        switch ( params_type[n] ) {
            case 'C':
            case 'S':
                printf("char * : %s\n", (char *)pv[i]);
                break;
            case 'i':
                printf("int    : %d\n", (int)pv[i]);
                break;
            case 'l':
                printf("long   : %ld\n", (long)pv[i]);
                break;
            case 'I':
                printf("int *  : 0x%08x\n", (unsigned int)pv[i]);
                break;
            case 'L':
                printf("long * : 0x%08x\n", (unsigned int)pv[i]);
            case 'V':
            case 'P':
                printf("void * : 0x%08x\n", (unsigned int)pv[i]);
            default:
                printf("type (%c) error!\n", params_type[n]);
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    char libname[256];
    char ret_type[16];
    char params_type[32];
    char funcname[64];
    const char *error;
    int i, j, argi, n, rc, verbose = 0;
    void *module, *pvv[16];
    int (*func)(void);

    if ( argc < 5 ) {
        printf(
            "runso version 0.1\n\n"
            "usage: runso [-v] lib ret-type params-type-list function param1 param2 ...\n\n"
            "type of parameters\n"
            "  c - char     C,S - char *\n"
            "  i - int      I - int *\n"
            "  l - long     L - long *\n"
            "  v - void     V,P - void *\n"
            "\nex:\n"
            "  function: char *getenv(const char *NAME);\n"
            "  runso libc.so C C getenv PATH\n"
            "  runso libc.so -v C C getenv PATH\n\n"
            "  function: char *strncpy(char *DST, const char *SRC, size_t LENGTH);\n"
            "  runso libc.so S SSi strncpy ----- xyz 2\n\n"
            "  function: char *strerror(int ERRNUM);\n"
            "  runso libc.so C i strerror 4\n"
            "  runso libc.so C i strerror 15\n");
        exit(1);
    }

    argi = 1;
    if ( strcmp(argv[argi], "-v") == 0 ) {
        verbose = 1;
        argi++;
    }

    strcpy(libname, argv[argi++]);
    strcpy(ret_type, argv[argi++]);
    strcpy(params_type, argv[argi++]);
    strcpy(funcname, argv[argi]);

    module = dlopen(libname, RTLD_NOW); // RTLD_LAZY

    if ( !module ) {
        printf("loading module (%s) %s\n", libname, dlerror());
        exit(1);
    }

    func = dlsym(module, funcname);

    if ( (error = dlerror()) ) {
        printf("lookup function (%s) %s\n", funcname, error);
        exit(1);
    }

    if ( verbose ) printf("function : %s:%s : %p\n", libname, funcname, func);

    // MAKE PARAMETERS LIST

    j = 0;
    n = strlen(params_type);

    for (i=argc-1; i>argi; i--) {
        switch ( params_type[--n] ) {
            case 'C': // char *
            case 'S': // char *
                pvv[j] = (char *)alloca(strlen(argv[i])+1);
                strcpy(pvv[j++], argv[i]);
                break;
            case 'i': // int
                pvv[j++] = (void *)atoi(argv[i]);
                break;
            case 'l': // long
                pvv[j++] = (void *)atol(argv[i]);
                break;
            case 'V': // void *
            case 'P': // void *
                pvv[j++] = NULL;
                break;
            default:
                printf("type (%c) error!\n", params_type[n]);
                exit(1);
                break;
        }
    }

    if ( verbose ) show_params(pvv, params_type, j);

    // PUSH PARAMETERS TO STACK THEN CALL FUNCTION

    if ( verbose ) printf("call: %p\n", func);

    asm("addl %0, %%esp\n" :: "r" (j*4));
    for (i=0; i<j; i++) {
        asm("subl $4, %%esp\n\t"
            "movl %0, (%%esp)\n\t"
            :: "r" (pvv[i]));
    }

    rc = (func)();

    switch ( *ret_type ) {
        case 'i': // int
        case 'l': // long
            printf("\nreturn:\n  %d\n", rc);
            break;
        case 'C': // char *
        case 'S': // char *
            printf("\nreturn:\n  %s\n", (char *)rc);
            break;
        case 'V': // void *
        case 'P': // void *
            printf("\nreturn:\n  %p\n", (void *)rc);
            break;
        case 'v': // void
            printf("\n");
            break;
        default:
            printf("type (%c) error!\n", *ret_type);
            break;
    }

    if ( verbose ) show_params(pvv, params_type, j);

    dlclose(module);

    exit(0);
}
