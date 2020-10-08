#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "exe.h"
#include "Debug.h"

// =============================================================================
typedef struct{
    int month;
    int day;
    int hour;
    int min;
    int sec;
    int msec;
    int pid;
    int tid;
    char level;
    char tag[ 128 ];
    const char *desc;
}m_log_format;

#define IS_NUMBER(_x) (((_x)>='0')&&((_x)<='9'))
#define IS_NOT_NUMBER(_x) (((_x)<'0')||((_x)>'9'))

// =============================================================================
const char *gp_log_level = "V";

// =============================================================================
static void lf_update_adbexe( void );
static int lf_get_one_line( const char *src, const char **start );
static int lf_parse_line( const char *src, m_log_format *log );
static void lf_show_log_info( m_log_format *log );

// =============================================================================
static char *lv_adb_devsn = NULL;
static char lv_adbexe[ 64 ];
static int lv_pids[32];
static int lv_pidnums = 0;

// =============================================================================
int adb_set_devices_sn( const char *sn ){
    if( lv_adb_devsn ) free( lv_adb_devsn );
    lv_adb_devsn = NULL;
    if( sn ==  NULL ) return 0;
    lv_adb_devsn = strdup( sn );
    return 0;
}

int logcat_set_fillter_packages( const char *packages ){
    char            tv_cmd[ 1024 ];
    char           *tp_stdout = NULL;
    int             tv_stdout_size = 0;

    lv_pidnums = 0;

    lf_update_adbexe();

    snprintf( tv_cmd, 1024, "%s shell 'for dir in $(ls /proc);"
            " do test -f /proc/${dir}/cmdline; if [ $? -eq 0 ];"
            " then echo \"${dir} $(cat /proc/${dir}/cmdline)\"; fi; done'",
            lv_adbexe );

    tv_stdout_size = exe_run_and_get_stdout( tv_cmd, &tp_stdout );
    if( tv_stdout_size <= 0 ) return tv_stdout_size;

    {
        int         tv_res;
        const char *tp_str = tp_stdout;
        char        tv_line[ 128 ];
        int         tv_cpysize;
        do{
            tv_res = lf_get_one_line( tp_str, &tp_str );
            if( tv_res < 0 ) break;
            tv_cpysize = tv_res > 127 ? 127 : tv_res;
            strncpy( tv_line, tp_str, tv_cpysize );
            tv_line[ tv_cpysize ] = '\0';
            tp_str += tv_res;

            if( strstr( tv_line, packages ) ){
                DLLOGI( "%s", tv_line );
                lv_pids[ lv_pidnums ] = atoi( tv_line );
                lv_pidnums++;
            }
        }while(1);

        // for( int i = 0; i < lv_pidnums; i++ )
        //     DLLOGI( "pid (%d)-> %d", i, lv_pids[ i ] );
    }


    free( tp_stdout );

    return lv_pidnums;
}



int logcat_get_log( void ){
    m_log_format    tv_logf;
    char            tv_logcat_cmd[ 256 ];
    char            tv_wbuf[40960];
    char            tv_lbuf[10240];
    char           *start;
    int             tv_remain = 0;

    lf_update_adbexe();
    snprintf( tv_logcat_cmd, 256, "%s shell \"logcat -b all\"",
            lv_adbexe  );

    m_exe_options *tp_opt = exe_alloc();

    tp_opt->cmd = tv_logcat_cmd;
    tp_opt->flags = EXE_STDOUT | EXE_STDERR;
    exe_parse_cmd( tp_opt );
    exe_show_opts( tp_opt );
    exe_run( tp_opt );
    exe_set_read_noblock( tp_opt );

    do{
        if( exe_isrunning( tp_opt ) ){
            DLLOGD( "errno: %s", strerror( errno ) );
            break;
        }
        int rlen = exe_read_stdout( tp_opt, &tv_wbuf[ tv_remain ],
                sizeof( tv_wbuf ) - 1 - tv_remain );

        if( rlen > 0 ){
            rlen += tv_remain;
            tv_wbuf[ rlen ] = '\0';
            start = tv_wbuf;
            while(1){
                int tv_res;
                int tv_parse_ret;

                tv_res = lf_get_one_line( start, (const char **)&start );
                if( tv_res < 0 ) break;

                memcpy( tv_lbuf, start, tv_res );
                tv_lbuf[ tv_res ] = '\0';

                tv_parse_ret = lf_parse_line( tv_lbuf, &tv_logf );
                if( tv_parse_ret < 0 ){
                    DLLOGE( "PARSE FAILED(%d): %s", tv_parse_ret, tv_lbuf );
                    // lf_show_log_info( &tv_logf );
                }else{
                    if( lv_pidnums > 0 ){
                        for( int i = 0; i < lv_pidnums; i++ ){
                            // DLLOGD( "-> %d", tv_logf.pid );
                            if( tv_logf.pid == lv_pids[i] ){
                                if( tv_logf.level == 'E' )
                                    DLLOGE( "%s", tv_lbuf );
                                else if( tv_logf.level == 'W' )
                                    DLLOGW( "%s", tv_lbuf );
                                else
                                    DLLOG( "%s", tv_lbuf );
                                break;

                            }
                        }
                    }else{
                        DLLOGI( "-> %s", tv_lbuf );
                    }
                }

                start += tv_res;
            }
            tv_remain = rlen - ( start - tv_wbuf );
            memcpy( tv_wbuf, start, tv_remain );
        }

        usleep(1000000);
    }while( 1 );
    return 0;
}

int logcat_set_fillter_pids( int *pids, int nums ){
    if( nums > 32 ) nums = 32;
    for( int i = 0; i < nums; i++ )
        lv_pids[ i ] = pids[ i ];
    lv_pidnums = nums;
    return 0;
}

// =============================================================================
static void lf_update_adbexe( void ){
    if( lv_adb_devsn ) snprintf( lv_adbexe, 64, "adb -s %s", lv_adb_devsn );
    else snprintf( lv_adbexe, 64, "adb" );
}

static int lf_get_one_line( const char *src, const char **start ){
    const char *s = NULL;

    if( src == NULL ) return -1;
    // *start = NULL;

    while( 1 )
        if( *src == '\0' ) return -1;
        else if( ( *src == '\r' ) || ( *src == '\n' ) ) src++;
        else break;

    s = src;

    while( 1 )
        if( *src == '\0' ) return -1;
        else if( *src == '\r' || *src == '\n' ) break;
        else src++;

    *start = s;
    return src - *start;
}

// 10-04 12:49:19.809  1586  1911 D WindowManager: null is detected
inline static int lf_skip_space( const char *src ){
    int tv_count = 0;
    do
        if( src[ tv_count ] == '\0' )
            return -1;
        else if( ( src[ tv_count ] == ' ' ) || ( src[ tv_count ] == '\t' )  )
            continue;
        else break;
    while( ++tv_count );
    return tv_count;
}
inline static int lf_skip_nums( const char *src ){
    int tv_count = 0;
    do
        if( src[ tv_count ] == '\0' )
            return -1;
        else if( ( src[ tv_count ] >= '0' ) && ( src[ tv_count ] <= '9' )  )
            continue;
        else break;
    while( ++tv_count );
    return tv_count;
}

static int lf_parse_line( const char *src, m_log_format *log ){
    int tv_count = 0;
    int tv_ret;

    // date
    if( ( tv_count = lf_skip_space( src ) ) < 0 ){
        return -1;
    }

    if( IS_NOT_NUMBER( src[ tv_count ] ) ) return -2;
    log->month = atoi( &src[ tv_count + 0 ] );

    if( IS_NOT_NUMBER( src[ tv_count + 3  ] ) ) return -3;
    log->day = atoi( &src[ tv_count + 3 ] );

    // time
    if( IS_NOT_NUMBER( src[ tv_count + 6  ] ) ) return -4;
    log->hour = atoi( &src[ tv_count + 6 ] );

    if( IS_NOT_NUMBER( src[ tv_count + 9  ] ) ) return -5;
    log->min = atoi( &src[ tv_count + 9 ] );

    if( IS_NOT_NUMBER( src[ tv_count + 12  ] ) ) return -6;
    log->sec = atoi( &src[ tv_count + 12 ] );

    if( IS_NOT_NUMBER( src[ tv_count + 15  ] ) ) return -7;
    log->msec = atoi( &src[ tv_count + 15 ] );

    // pid
    tv_count += 19;
    if( ( tv_ret = lf_skip_space( &src[ tv_count ] ) ) < 0 ){
        return -8;
    }
    tv_count += tv_ret;
    if( IS_NOT_NUMBER( src[ tv_count ] ) ) return -9;
    log->pid = atoi( &src[ tv_count ] );

    // tid
    tv_count += lf_skip_nums( &src[ tv_count ] );
    if( ( tv_ret = lf_skip_space( &src[ tv_count ] ) ) < 0 ){
        return -10;
    }
    tv_count += tv_ret;
    if( IS_NOT_NUMBER( src[ tv_count ] ) ) return -11;
    log->tid = atoi( &src[ tv_count ] );

    // log level
    tv_count += lf_skip_nums( &src[ tv_count ] );
    if( ( tv_ret = lf_skip_space( &src[ tv_count ] ) ) < 0 ){
        return -13;
    }
    tv_count += tv_ret;
    log->level = src[ tv_count ];
    tv_count++;

    // tag
    if( ( tv_ret = lf_skip_space( &src[ tv_count ] ) ) < 0 ){
        return -14;
    }
    tv_count += tv_ret;
    int i = 0;
    do
        if( src[ tv_count ] == '\0' )
            break;
        else if( ( src[ tv_count ] == ':' )&&( src[ tv_count+1 ] == ' ' ) )
            break;
        else if( i > 128 - 1 )
            break;
        else log->tag[ i ] = src[ tv_count ];
    while( ++tv_count && ++i );
    log->tag[ i ] = '\0';
    tv_count++;

    // description
    if( ( tv_ret = lf_skip_space( &src[ tv_count ] ) ) <= 0 ){
        log->desc = "";
        return 0;
    }
    tv_count += tv_ret;
    log->desc = &src[tv_count];

    return 0;
}

static void lf_show_log_info( m_log_format *log ){
    printf( "%d %d %d %d %d %d %d %d %c %s %s\r\n",
            log->month,
            log->day,
            log->hour,
            log->min,
            log->sec,
            log->msec,
            log->pid,
            log->tid,
            log->level,
            log->tag,
            log->desc );
}

