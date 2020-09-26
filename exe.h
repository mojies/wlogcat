#ifndef _EXE_H
#define _EXE_H

#include <cstdint>

typedef struct{
    const char *cmd;

#define EXE_STDIN   (0x01)
#define EXE_STDOUT  (0x02)
#define EXE_STDERR  (0x04)
#define EXE_EXIT    (0x80)
    uint8_t flags;

    int mstdin;
    int mstdout;
    int mstderr;
    pid_t pid;
} m_exe_options;

extern m_exe_options *exe_alloc( void );
extern int exe_alloc_free( m_exe_options *opt );
extern int exe_run( m_exe_options *iopt );
extern int exe_wait_exit( m_exe_options *iopt );
/*
 * 0 isRunning
 * -1 isStop or not exist
 * */
extern int exe_isrunning( m_exe_options *iopt );
extern int exe_parse_cmd( m_exe_options *opt );
extern void exe_show_opts( m_exe_options *opt );

#endif // __EXE_H

