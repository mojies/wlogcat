#ifndef __PARAM_H
#define __PARAM_H
#include <stdbool.h>

#define PFLASE      0
#define PTRUE       1

#ifdef __cplusplus
extern "C"{
#endif

typedef union {
    int         aint;
    float       afloat;
    const char *astr;
    unsigned    auint;
    unsigned    abool;
}u_udata;

typedef enum {
    E_PARAMS_TYPE_INT,
    E_PARAMS_TYPE_UINT,
    E_PARAMS_TYPE_FLOAT,
    E_PARAMS_TYPE_STR,
    E_PARAMS_TYPE_BOOL,
}e_udatatype;

typedef struct{
    const char     *pattern; // pattern str
    const char     *subpattern;
    e_udatatype     type;
    u_udata         data;
    const char     *description;
}m_params;

extern int param_get( int argc, char *argv[],\
        m_params *iparam, int num_param );

extern int param_get_wrap_word( char *dest, const char *sour, int start );
extern void param_show_helps( m_params *iparam, int inum );
extern void param_show_values( m_params *iparam, int inum );

#ifdef __cplusplus
}
#endif
#endif

