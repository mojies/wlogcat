#ifndef __LOGCAT_H
#define __LOGCAT_H

extern const char *gp_log_level;

extern int adb_set_devices_sn( const char *sn );
extern int logcat_set_fillter_packages( const char *packages );
extern int logcat_get_log( void );
extern int logcat_set_fillter_pids( int *pids, int nums );

#endif
