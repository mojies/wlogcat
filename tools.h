#ifndef __TOOLS_H
#define __TOOLS_H

typedef struct{
    int pid;
    const char *name;
}m_proc_info;

extern int regex_compare( const char *pattern, const char *target,
        bool need_complete );

extern int get_process_info( const char *iexename, m_proc_info **ipidbuf );

int free_process_pid( m_proc_info **ipidbuf );
bool is_process_exist( const char iprocess );
bool is_pids_exist( int *ipids, int inum );

#endif
