#include "Debug.h"
#include "params.h"
#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include<sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "exe.h"
#include "logcat.h"
#include "str.h"

// =============================================================================
#define MAX_MARCH_PIDS 32

// =============================================================================
typedef struct{
    m_params    serial;
    m_params    packages;
    m_params    processes;
    m_params    pids;
    m_params    action;
    m_params    clear;
    m_params    llevel;
}m_inputparams;

// =============================================================================
static int lf_get_pids_via_packages( int pids[], int pids_size,
        const char *packages );

long long send_SSDP_M_SEARCH_request( int testTimes );
void listen_SSDP_PORT( void );
void *get_in_addr(struct sockaddr *sa);
void show_help_msg(void);

// =============================================================================
m_inputparams gv_params = {
    .serial =  {
        .pattern = "--serial",
        .subpattern = "-s",
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = NULL,
        },
        .description = "adb serialNumber",
    },
    .packages =  {
        .pattern = "--packages",
        .subpattern = "-p",
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = NULL,
        },
        .description = "target package name",
    },
    .processes =  {
        .pattern = "--processes",
        .subpattern = NULL,
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = NULL,
        },
        .description = "target processes name",
    },
    .pids =  {
        .pattern = "--pids",
        .subpattern = "-P",
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = NULL,
        },
        .description = "target pid number",
    },
    .action =  {
        .pattern = "--action",
        .subpattern = "-a",
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = NULL,
        },
        .description = "following value [ log, thread_nums ]",
    },
    .clear =  {
        .pattern = NULL,
        .subpattern = "-c",
        .type = E_PARAMS_TYPE_BOOL,
        .data = {
            .abool = false,
        },
        .description = "clear first befor cat log",
    },
    .llevel =  {
        .pattern = "--level",
        .subpattern = "-l",
        .type = E_PARAMS_TYPE_STR,
        .data = {
            .astr = "V",
        },
        .description = "Logcat Log level, value range [ V, D, I, W, E, F, S ]",
    },
};

unsigned int debug_level = 7;

int gv_pids[ MAX_MARCH_PIDS ];
int gv_pidnums = 0;

// =============================================================================
int main( int argc, char *argv[] ){
    // DLLOGD("Hello world!");

    if( param_get( argc, argv, (m_params*)&gv_params, sizeof( m_inputparams )/sizeof( m_params ) ) ){
        param_show_helps( (m_params*)&gv_params, sizeof( m_inputparams )/sizeof( m_params ) );
        return -1;
    }

    adb_set_devices_sn( gv_params.serial.data.astr );

    if( gv_params.pids.data.astr ){
        // split pid
        // checke pid is exist and warning user
        // build pattern str
    }else if( gv_params.processes.data.astr ){
        // split processes
        // get pids
        // build pattern str

    }else if( gv_params.packages.data.astr ){

        gv_pidnums = logcat_set_fillter_packages( gv_params.packages.data.astr );

    }

    if( gv_pidnums <= 0 ){
        param_show_helps( (m_params*)&gv_params,
                sizeof( m_inputparams )/sizeof( m_params ) );

        return -1;
    }

    gp_log_level = gv_params.llevel.data.astr;

    // while( 0 ){
        logcat_get_log();
        // run adb logcat to cache the log
        // save the whole logs
        // filter the target
        // print last ms log
    // }

    return 0;
}


static int lf_get_pids_via_packages( int pids[], int pids_size,
        const char *packages  ){

    m_proc_info *tp_pkg_pids;

    const char *tp_pkgs[ 32 ];

    int tv_split_pkgnum;
    int tv_pkgnums = 0;
    int tv_pidnums = 0;
    int i;

    tv_split_pkgnum = str_split( tp_pkgs, 32, packages, ',' );
    if( tv_split_pkgnum == 0 ){
        tp_pkgs[0] = packages;
        tv_pkgnums = 1;
    }else{
        tv_pkgnums = tv_split_pkgnum;
    }

    for( i = 0; i < tv_pkgnums; i++ ){
        int tv_nums = get_process_info( tp_pkgs[ i ], &tp_pkg_pids );
        if( tv_nums > 0 ){
            for( int j = 0; j < tv_nums; j++ ){
                pids[ tv_pidnums ] = tp_pkg_pids[j].pid;
                tv_pidnums++;
                if( tv_pidnums == pids_size ) break;
            }
            free_process_pid( &tp_pkg_pids );
        }
        if( tv_pidnums == 32 ) break;
    }

    str_split_free( tp_pkgs, tv_split_pkgnum );

    return tv_pidnums;
}

void show_help_msg(void){
}

