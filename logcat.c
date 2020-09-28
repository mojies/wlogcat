#include <string.h>
#include <errno.h>

#include "exe.h"
#include "Debug.h"
// =============================================================================
char gv_adbexe[ 64 ];

// =============================================================================
#define INIT_LOGCAT_CMD 0x01
static uint32_t tv_status = 0;
static char gv_logcat_cmd[ 256 ];

// =============================================================================


int locat_get_log( void ){
    if( ! (tv_status & INIT_LOGCAT_CMD) ){
        snprintf( gv_logcat_cmd, 256, "%s shell \"logcat -b all -d\"", gv_adbexe  );
        tv_status |= INIT_LOGCAT_CMD;
    }

    m_exe_options *tp_opt = exe_alloc();

    tp_opt->cmd = gv_logcat_cmd;
    tp_opt->flags = EXE_STDOUT | EXE_STDERR;

    DLLOGD( "exe_parse_cmd ret:%d", exe_parse_cmd( tp_opt ) );
    exe_show_opts( tp_opt );

    exe_run( tp_opt );
    exe_set_read_noblock( tp_opt );
    do{
        if(  exe_isrunning( tp_opt ) ){
            DLLOGD( "errno: %s", strerror( errno ) );
            break;
        }
        char buf[40960];
        memset( buf, 0, sizeof buf );
        int rlen = exe_read_stdout( tp_opt, buf, sizeof buf - 1 );
        if( rlen > 0 ){
            DLLOGD( "stdout: %s", buf );
        }else{
            DLLOGD( "read 0" );
        }
        // memset( buf, 0, sizeof buf );
        // rlen = read( tp_opt->mstderr, buf, sizeof( buf ) );
        // if( rlen > 0 ){
        //     DLLOGD( "strerr: %s", buf );
        // }
        usleep(1000000);
    }while( 1 );
    return 0;
}

