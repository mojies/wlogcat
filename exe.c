#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exe.h"
#include "Debug.h"

// =============================================================================
typedef unsigned char uint8_t;

typedef struct{
    const char *cmd;

    uint32_t flags;

    // int mstdin;
    // int mstdout;
    // int mstderr;
    pid_t pid;

    char *executor;
    char **args;
    int  argc;
    int  arg_size;
    int fd_stdin[2];
    int fd_stdout[2];
    int fd_stderr[2];
} m_inner_options;


// =============================================================================
static void* lf_realloc_clear( void *p, int origin_size, int expend_size );
static int lf_strndup_args( m_inner_options *opt, bool need_translate,
        const char *start, const char *stop  );

// =============================================================================

m_exe_options *exe_alloc( void ){
    m_inner_options *p = (m_inner_options*)calloc( sizeof( m_inner_options ), 1 );
    // p->mstdin = -1;
    // p->mstdout = -1;
    // p->mstderr = -1;

    return (m_exe_options*)p;
}

int exe_alloc_free( m_exe_options *opt ){
    m_inner_options *p = (m_inner_options*)opt;
    int i = 0;

    if( opt == NULL ) return -1;
    // if( p->executor ){
    //     free( p->executor );
    // }
    if( p->args ){
        while( p->args[i] != NULL ){
            free( p->args[i] );
            i++;
        }
        free( p->args );
    }
    free( p );
    return 0;
}

int exe_run( m_exe_options *iopt ){
    if( iopt == NULL ) return -1;

    m_inner_options *tp_opt = ( m_inner_options* )iopt;

    /*
    if( exe_parse_cmd( tp_opt ) < 0 ){
        // 
        return -1;
    }*/

    if( tp_opt->flags & EXE_STDIN ){
        pipe( tp_opt->fd_stdin );
    }
    if( tp_opt->flags & EXE_STDOUT ){
        pipe( tp_opt->fd_stdout );
    }
    if( tp_opt->flags & EXE_STDERR ){
        pipe( tp_opt->fd_stderr );
    }

    // split args

    // create pip
    // fork
    //
    pid_t pid;

    if( ( pid = fork() ) < 0 ){
        DLLOGE( "exe_run: fork failed!" );
        return -1;
    }
    if( pid == 0 ){
        int tv_null_in;
        // setsid();
        if( tp_opt->flags & EXE_STDIN ){
            close( tp_opt->fd_stdin[1] );
            dup2( tp_opt->fd_stdin[0], 0 );
        }else{
            int tv_null_out = open( "/dev/null", O_RDONLY );
            if( tv_null_out < 0 ){
                exit( -1 );
            }
            dup2( tv_null_out, 0 );
        }
        if( tp_opt->flags & EXE_STDOUT ){
            close( tp_opt->fd_stdout[0] );
            dup2( tp_opt->fd_stdout[1], 1 );
        }else{
            tv_null_in = open( "/dev/null", O_RDWR );
            if( tv_null_in < 0 ){
                exit( -1 );
            }
            dup2( tv_null_in, 1 );
        }
        if( tp_opt->flags & EXE_STDERR ){
            close( tp_opt->fd_stderr[0] );
            dup2( tp_opt->fd_stderr[1], 2 );
        }else{
            if( tv_null_in == -1 ){
                tv_null_in = open( "/dev/null", O_RDWR );
                if( tv_null_in < 0 ){
                    exit( -1 );
                }
            }
            dup2( tv_null_in, 2 );
        }

        execvp( tp_opt->executor, tp_opt->args );
        abort();
    }

    tp_opt->pid = pid;

    if( tp_opt->flags & EXE_STDIN ){
        close( tp_opt->fd_stdin[0] );
        // tp_opt->mstdin = tp_opt->fd_stdin[1];
    }
    if( tp_opt->flags & EXE_STDOUT ){
        close( tp_opt->fd_stdout[1] );
        // tp_opt->mstdout = tp_opt->fd_stdout[0];
    }
    if( tp_opt->flags & EXE_STDERR ){
        close( tp_opt->fd_stderr[1] );
        // tp_opt->mstderr = tp_opt->fd_stderr[0];
    }
    return 0;
}

int exe_wait_exit( m_exe_options *iopt ){
    m_inner_options *tp_opt = ( m_inner_options * )iopt;
    if( tp_opt->pid > 0 ){
        if( waitpid( tp_opt->pid, NULL, 0 ) != tp_opt->pid ){
            return 0;
        }
    }
    return 0;
}

int exe_isrunning( m_exe_options *iopt ){
    m_inner_options *tp_opt = ( m_inner_options * )iopt;
    if( tp_opt == NULL ) return -1;
    int ret = 0;
    if( tp_opt->pid > 0 ){
        ret = waitpid( tp_opt->pid, NULL, WNOHANG );
        if( ret == 0 ){
            return 0;
        }else if( ret == tp_opt->pid ){
            return -1;
        }
    }
    DLLOGE( "Shouldn't run this: ret(%d) errno(%d)(%s)",
            ret, errno, strerror(errno));
    exit( -1 );
}


#define EXE_PARSE_IDLE              0x01
#define EXE_PARSE_ONFOCUS           0x02
#define EXE_PARSE_START_WITH_39     0x04 // '
#define EXE_PARSE_START_WITH_34     0x08 // "

// #define EXE_PARSE_GOT_FILE          0x10
#define EXE_PARSE_NEED_TRANSLATE    0x20

int exe_parse_cmd( m_exe_options *opt ){
    m_inner_options *tp_opt = ( m_inner_options * )opt;
    const char *str_start;
    const char *p;

    // relloc size = origin size * 1.75

    uint8_t tv_status = EXE_PARSE_IDLE;

    if( tp_opt->cmd == NULL ){
        DLLOGE( "opt->CMD not set!" );
        return -1;
    }
    p = tp_opt->cmd;

    if( tp_opt->executor || tp_opt->args ){
        DLLOGE( "Your Options need to init" );
        return -1;
    }

    tp_opt->args = (char **)calloc( sizeof(char **), 1 );
    if( tp_opt->args == NULL ) return -1;
    tp_opt->arg_size = 1;


    while( true ){
        char c = *p;

        if( tv_status & EXE_PARSE_ONFOCUS ){
            if( (c == 0x20) || (c == 0x09) || ( c == '\0' ) ){
                if( ( tv_status & ( EXE_PARSE_START_WITH_34 |
                                EXE_PARSE_START_WITH_39 ) ) ){
                    if( c == '\0' ){
                        str_start--;
                        tv_status &= ~( EXE_PARSE_START_WITH_34 |
                                EXE_PARSE_START_WITH_39 );
                    }else{
                        goto GT_PARSE_LOOP_UNDER;
                    }
                }

                if(  c != *str_start ){
                    if( lf_strndup_args( tp_opt, false, str_start, p ) < 0 ){
                        // TODO
                        return -1;
                    }
                    str_start = p+1;
                }
                tv_status &= ~EXE_PARSE_ONFOCUS;
                tv_status |= EXE_PARSE_IDLE;
                    // end
                if( c == '\0' ) break;
                goto GT_PARSE_LOOP_UNDER;
            }else if( c == '\'' ){
                if( tv_status & EXE_PARSE_START_WITH_39 ){
                    // end
                    if( lf_strndup_args( tp_opt, false, str_start, p ) < 0 ){
                        // TODO
                        return -1;
                    }
                    str_start = p+1;

                    tv_status &= ~( EXE_PARSE_START_WITH_39 |
                            EXE_PARSE_ONFOCUS );
                }else{
                    goto GT_PARSE_LOOP_UNDER;
                }
            }else if( c == '"' ){
                if( tv_status & EXE_PARSE_START_WITH_34 ){
                    // end
                    if( lf_strndup_args( tp_opt, false, str_start, p ) < 0 ){
                        // TODO
                        return -1;
                    }

                    tv_status &= ~( EXE_PARSE_START_WITH_34 |
                            EXE_PARSE_ONFOCUS );
                    str_start = p+1;
                }else{
                    goto GT_PARSE_LOOP_UNDER;
                }
            }else if( c == '\\' ){
                tv_status |= EXE_PARSE_NEED_TRANSLATE;
                p++;
            }else if( c > 0x20 && c < 0x7F ){
                goto GT_PARSE_LOOP_UNDER;
            }else{
                return -1;
            }

        }else{
            if( (c == 0x20) && (c == 0x09) ){
                tv_status |= EXE_PARSE_IDLE;
                goto GT_PARSE_LOOP_UNDER;
            }else if( c == '\0' ){
                break;
            }else if( c > 0x20 && c < 0x7F ){
                if( tv_status & EXE_PARSE_IDLE ){
                    tv_status &= ~EXE_PARSE_IDLE;

                    tv_status |= EXE_PARSE_ONFOCUS;

                    // if( tv_status & EXE_PARSE_GOT_FILE  ){
                    if( c == '\'' ){
                        tv_status |= EXE_PARSE_START_WITH_39;
                        str_start = p+1;
                    }else if( c == '"' ){
                        tv_status |= EXE_PARSE_START_WITH_34;
                        str_start = p+1;
                    }else{
                        str_start = p;
                    }
                }else{
                    // error formay
                    return -1;
                }
            }else{
                return -1;
            }
        }
GT_PARSE_LOOP_UNDER:
        p++;
    }

    if( ( tp_opt->argc > 0 ) &&
        ( tp_opt->argc == tp_opt->arg_size ) ){

        tp_opt->args = (char**)lf_realloc_clear( (void*)tp_opt->args,
                tp_opt->arg_size*sizeof(char**),
                ( tp_opt->arg_size + 1 )*sizeof(char**) );

        tp_opt->args[tp_opt->arg_size] = 0;
        tp_opt->arg_size++;
    }

    tp_opt->executor = tp_opt->args[0];
    return 0;
}

int exe_set_read_noblock( m_exe_options *opt ){
    m_inner_options *tp_opt = (m_inner_options*)opt;
    int set = 1;
    int r;

    if( opt == NULL ) return -1;
    do
        ioctl( tp_opt->fd_stdout[1], FIONBIO, &set );
    while( r == -1 && errno == EINTR );
    if( r ) return errno;

    do
        ioctl( tp_opt->fd_stderr[1], FIONBIO, &set );
    while( r == -1 && errno == EINTR );
    if( r ) return errno;
    return 0;
}

int exe_read_stdout( m_exe_options *opt, char *rbuf, int rsize  ){
    m_inner_options *tp_opt = (m_inner_options*)opt;
    return read( tp_opt->fd_stdout[0], rbuf, rsize );
}

int exe_read_stderr( m_exe_options *opt, char *rbuf, int rsize  ){
    m_inner_options *tp_opt = (m_inner_options*)opt;
    return read( tp_opt->fd_stderr[0], rbuf, rsize );
}

int exe_write_stdin( m_exe_options *opt, char *wbuf, int wsize  ){
    m_inner_options *tp_opt = (m_inner_options*)opt;
    return write( tp_opt->fd_stdin[1], wbuf, wsize );
}

void exe_show_opts( m_exe_options *opt ){
    m_inner_options *tp_opt = (m_inner_options*)opt;

    if( tp_opt == NULL ) return;
    DLLOGV( "CMD: %s", tp_opt->cmd );
    DLLOGV( "flags;" );
    if( tp_opt->flags & EXE_STDIN )
        DLLOGV( "    STDIN" );
    if( tp_opt->flags & EXE_STDOUT )
        DLLOGV( "    STDOUT" );
    if( tp_opt->flags & EXE_STDERR )
        DLLOGV( "    STDERR" );

    if( tp_opt->executor )
        DLLOGV( "executor: %s", tp_opt->executor );

    DLLOGV( "argc: %d", tp_opt->argc );
    DLLOGV( "args size: %d", tp_opt->arg_size );

    if( tp_opt->args ){
        int i = 0;
        DLLOGV( "args:" );
        while( tp_opt->args[i] != NULL ){
            DLLOGV( "    {%d}:%s", i, tp_opt->args[i] );
            i++;
        }
    }
    DLLOGV( "DONE!\n" );
}

// =============================================================================
static void* lf_realloc_clear( void *p, int origin_size, int expend_size ){
    char *tp = (char*)p;
    tp = (char *)realloc( tp, expend_size );
    if( tp == NULL ){
        return NULL;
    }
    memset( &tp[ origin_size ], 0x00, expend_size - origin_size );

    return (void*)tp;
}

static int lf_strndup_args( m_inner_options *opt, bool need_translate,
        const char *start, const char *stop  ){

    if( opt->argc == opt->arg_size ){
        int tv_next_space = opt->arg_size*1.75;
        if( tv_next_space == opt->arg_size )
            tv_next_space++;

        opt->args = (char**)lf_realloc_clear( (void*)opt->args
                , opt->arg_size*sizeof(char**)
                , tv_next_space*sizeof(char **) );
        if( opt->args == NULL ){
            return -1;
        }
        // opt->args = (char**)realloc( (void*)opt->args,
        //         tv_next_space*sizeof(char **) );
        // if( opt->args == NULL ){
        //     // TODO
        //     return -1;
        // }
        // void *p = (void*)(& opt->args[ opt->arg_size ]);
        // memset( p, ( tv_next_space - opt->arg_size )*sizeof(char **), 0x00 );
        opt->arg_size = tv_next_space;
    }
    opt->args[ opt->argc ] = strndup( start, stop-start );
    if( opt->args[ opt->argc ] == NULL ){
        // TODO
        return -1;
    }

    if( need_translate ){
        char *s;
        char *d;
        s = d = opt->args[ opt->argc ];
        while( *s != '\0' ){
            if( *s == '\\' ){
                s++; *d = *s;
            }else{
                *d = *s;
            }
            d++;
            s++;
        }
        *d = '\0';
    }
    opt->argc++;

    return 0;
}

// =============================================================================
// const char *cmd_set[] = {
//     "ls -la",
//     "ls '-ls'",
//     "ls -ls'",
//     "ls '-ls",
//     "ls \"-ls\"",
//     "ls -ls\"",
//     "ls \"-ls",
//     "ls \"\\\"-ls",
//     "ls \"\\\"-ls -la",
//     "/aa/tcl.tools \"\\\"-ls -la",
//     NULL,
// };
//
// for( int i = 0; cmd_set[i] != NULL; i++ ){
//     m_exe_options *tp_opt = exe_alloc();
//     tp_opt->cmd = cmd_set[i];
//
//     exe_parse_cmd( tp_opt );
//
//     exe_show_opts( tp_opt );
//     exe_alloc_free( tp_opt );
//     DLLOGD( "" );
// }
// =============================================================================


