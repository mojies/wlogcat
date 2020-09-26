#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <regex.h>
#include <unistd.h>

#include "tools.h"
#include "Debug.h"

#define REGEX_PROGRAM_STAND "https://pubs.opengroup.org/onlinepubs/007908799/xbd/re.html"

static int regex_complete_pattern( const char **dest, const char *pattern ){
    int pattern_size = strlen( pattern );

    char *new_pattern;
    int is_match = 1;

    if( ( pattern[0] != '^' ) ||
        ( pattern[pattern_size-1] != '$' ) ){

        char re_pattern_templat[8];
        int count = 0;
        if( pattern[0] != '^' )
            re_pattern_templat[count++] = '^';
        re_pattern_templat[count++] = '%';
        re_pattern_templat[count++] = 's';
        if( pattern[pattern_size-1] != '$' )
            re_pattern_templat[count++] = '$';
        re_pattern_templat[count] = '\0';

        new_pattern = (char*)malloc( pattern_size + count - 2 + 1 );
        if( new_pattern == NULL ){
            DLLOGE( "Alloc mem failed!" );
            exit(1);
        }

        sprintf( new_pattern, re_pattern_templat, pattern );
    }

    *dest = new_pattern;
    return 0;
}

static int regex_free_pattern( const char **pattern ){
    if( pattern == NULL ) return 0;
    const char *p = *pattern;
    if( p == NULL ) return 0;
    *pattern == NULL;
    free( (void*)p );
    return 0;
}

int regex_compare( const char *pattern, const char *target, bool need_complete ){
    int is_match = 1;
    int pattern_size = strlen( pattern );

    const char *new_pattern = NULL;

    if( need_complete )
        regex_complete_pattern( &new_pattern, pattern );
    else
        new_pattern = pattern;

    regex_t regex;
    if( regcomp( &regex, new_pattern, 0) ){
        DLLOGE( "Regex pattern error style, Please refer:" REGEX_PROGRAM_STAND );
        exit(1);
    }
    if( regexec( &regex, target, 0, NULL, 0) == 0 ){
        is_match = 0;
    }

    regfree(&regex);
    if( need_complete )
        regex_free_pattern( &new_pattern );

    return is_match;
}

int get_process_info( const char *iexename, m_proc_info **ipidbuf ){
    DIR                *tv_dir;
    struct dirent      *tv_dptr;
    char               *tp_name;
    const char         *tp_new_pattern;
    int                 tv_num = 0;
    m_proc_info        *tp_pidbuf = NULL;
    int                 i;

    if( ( tv_dir = opendir( "/proc" ) ) == NULL ){
        DLLOGE( "Cant open /proc" );
        return -1;
    }

    regex_complete_pattern( &tp_new_pattern, iexename );

    while( (tv_dptr = readdir(tv_dir)) != NULL ){
        int     tv_namelen;
        char    tv_fullpath[128];
        int     tv_pid;
        char    tv_exename[128];
        FILE   *tv_fp;

        if( tv_dptr->d_type != DT_DIR  )
            continue;

        tp_name = tv_dptr->d_name;
        tv_namelen = strlen( tp_name );
        for( i=0; i<tv_namelen; i++ )
            if( ( tv_dptr->d_name[i] < '0' )\
             || ( tv_dptr->d_name[i] > '9' ) )
                break;

         if( i != tv_namelen )
            continue;

        tv_pid = atoi( tp_name );
        tv_fullpath[ sprintf( tv_fullpath, "/proc/%d/comm", tv_pid ) ] = '\0';
        if( access( tv_fullpath, F_OK ) )
            continue;
        if( ( tv_fp = fopen( tv_fullpath, "r" ) ) == NULL )
            continue;
        if( fgets( tv_exename, 128, tv_fp ) == NULL ){
            fclose( tv_fp );
            continue;
        }
        fclose( tv_fp );

        for( i=0; i<(int)(strlen(tv_exename)+1); i++ )
            if( ( tv_exename[i] == '\r' )\
             || ( tv_exename[i] == '\n' ) ){
                tv_exename[i] = '\0';
                break;
            }

        if( regex_compare( tp_new_pattern, tv_exename, false ) )
            continue;

        tp_pidbuf = (m_proc_info*)realloc( tp_pidbuf, sizeof( m_proc_info ) * tv_num + 1 );
        if( tp_pidbuf == NULL ){
            DLLOGE( "realloc memory failed!" );
            exit(-1);
        }
        tp_pidbuf[ tv_num ].pid = tv_pid;
        tv_num++;
    }

    regex_free_pattern( &tp_new_pattern );
    *ipidbuf = tp_pidbuf;
    return tv_num;
}

int free_process_pid( m_proc_info **ipidbuf ){
    if( ipidbuf == NULL ) return 0;

    m_proc_info *tv_tmp = *ipidbuf;
    *ipidbuf = NULL;

    free( tv_tmp );

    return 0;
}

bool is_process_exist( const char iprocess ){
    m_proc_info *tp_pid_info;
    int nums;

    nums = get_process_info( "chrome", &tp_pid_info );
    free_process_pid( &tp_pid_info );
    return (nums>0);
}

bool is_pids_exist( int *ipids, int inum ){
    struct stat tv_stat;
    char tv_file_path[128];

    for( int i = 0; i < inum; i++ ){
        memset( tv_file_path, 128, 0x00 );
        snprintf( tv_file_path, sizeof(tv_file_path ),
                "/proc/%d/stat", ipids[i] );

        if( stat( tv_file_path, &tv_stat ) < 0 ){
            return false;
        }
    }
    return true;
}





