#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "Debug.h"

int param_get( int argc, char *argv[], m_params *iparam, int num_param ){
    int                 i, j;

    if( iparam == NULL || num_param == 0 ){
        DLLOGE( "Param contain is null" );
        return -1;
    }

    for( i = 0; i < argc; i++ ){
        if( strcmp( "-h", argv[ i ] ) == 0 ){
            param_show_helps( iparam, num_param );
            return 0;
        }else if( strcmp( "--help", argv[ i ] ) == 0 ){
            if( access( "./README", F_OK ) ){
                DLLOGD("Current dir didnt contain README file,"\
                        "Please check this file under project dirctory.");
                return -1;
            }else{
                system("cat ./README");
                return 0;
            }
        }
        for( j = 0; j < num_param; j++ ){
            // DLLOGD( "param[j]: %d %s %s", j, iparam[j].pattern, iparam[j].subpattern );
            if( ( iparam[j].pattern )\
             && ( strcmp( argv[i], iparam[j].pattern ) == 0) )
                goto GT_param_get_update;
            else if( ( iparam[j].subpattern )\
                  && ( strcmp( argv[i], iparam[j].subpattern ) == 0 ) )
                goto GT_param_get_update;
            else
                continue;
GT_param_get_update:
            // DLLOGD( "argv[i]:%s", argv[i] );
            switch( iparam[j].type ){
            case E_PARAMS_TYPE_INT:
                i++; if( i >= argc ) return -1;
                sscanf( argv[i], "%d", &(iparam[j].data.aint) );
                // DLLOGD( "float -> %d", iparam[j].data.aint );
            break;
            case E_PARAMS_TYPE_UINT:
                i++; if( i >= argc ) return -1;
                sscanf( argv[i], "%u", &(iparam[j].data.auint) );
                // DLLOGD( "float -> %u", iparam[j].data.auint );
            break;
            case E_PARAMS_TYPE_FLOAT:
                i++; if( i >= argc ) return -1;
                sscanf( argv[i], "%f", &(iparam[j].data.afloat) );
                // DLLOGD( "float -> %f", iparam[j].data.afloat );
            break;
            case E_PARAMS_TYPE_STR:
                i++; if( i >= argc ) return -1;
                iparam[j].data.astr = argv[ i ];
            break;
            case E_PARAMS_TYPE_BOOL:
                iparam[j].data.abool = PTRUE;
            break;
            default:break;
            }
            break;
        }
    }
    return 0;
}

#define DESCRIPT_MAX_SIZE 60
int param_get_wrap_word( char *dest, const char *sour, int start ){
    int     i           = 0;
    int     count       = 0;
    int     word_start  = -1;
    int     word_len    = 0;
    bool    is_space    = false;
    bool    is_end      = false;
    bool    is_break    = false;
    bool    need_space  = false;

    while( true ){
        if( ( sour[start] == ' ' ) ||
            ( sour[start] == '\r' ) ||
            ( sour[start] == '\n' ) ||
            ( sour[start] == '\t' ) ){

            need_space = true;
            is_space = true;
        }else if( sour[start] == '\0' ){
            is_space = true;
            is_end = true;

        }else if( sour[start] < 32 ){
            return -1;
        }else{
            is_space = false;
        }

        if( is_space ){
            if( word_start != -1 ){
                if( ( DESCRIPT_MAX_SIZE - count ) < word_len ){
                    if( count != 0 )
                        break;
                }
                if( DESCRIPT_MAX_SIZE-count < word_len ){
                    word_len = DESCRIPT_MAX_SIZE-count;
                    is_break = true;
                }
                snprintf( &dest[count], word_len+1, "%s",
                        &sour[ word_start ] );
                count += word_len;
                if( is_break )
                    word_start = word_start + word_len;
                else
                    word_start = -1;
                word_len = 0;
                if( is_end )
                    break;
            }
        }else{
            if( word_start == -1 )
                word_start = start;

            if( need_space ){
                if( (count + 1) >= 80 ){
                    break;
                }
                dest[ count ] = ' ';
                count++;
                need_space = false;
            }

            word_len++;
        }
        start++;
    }

    dest[ count ] = '\0';
    return word_start;
}

#define HELPS_BUF_SIZE (48 + 60)
void param_show_helps( m_params *iparam, int inum ){
    int i;

    int slen;
    int llen;
    int llen_max = 6;
    int slen_max = 10;

    char template_firstline[HELPS_BUF_SIZE];
    char template_otherline[HELPS_BUF_SIZE];
    char info[ HELPS_BUF_SIZE ];

    for( i = 0; i < inum; i++ ){
        int llen = 0;
        int slen = 0;
        if( iparam[i].pattern )
            llen = strlen( iparam[i].pattern );
        if( iparam[i].subpattern )
             slen = strlen( iparam[i].subpattern );
        llen_max = (llen_max > llen)?llen_max:llen;
        slen_max = (slen_max > slen)?slen_max:slen;
    }

    snprintf( template_firstline, HELPS_BUF_SIZE, "%%-%ds | %%-%ds | %%-8s | %%s",
            llen_max, slen_max );

    {
        char tmp[ HELPS_BUF_SIZE ];
        snprintf( tmp, HELPS_BUF_SIZE,
                "%%-%ds | %%-%ds | %%-8s | %%%%s", llen_max, slen_max );
        snprintf( template_otherline, HELPS_BUF_SIZE, tmp,
                "", "", "" );
    }

    DLLOGV("================================================================================");
    snprintf( info, HELPS_BUF_SIZE, template_firstline,
            "option", "sub option", "type",  "description" );

    DLLOGV( "%s", info );
    DLLOGV("--------------------------------------------------------------------------------");
    for( i = 0; i < inum; i++ ){
        const char *pattern = " ";
        const char *subpattern = " ";
        const char *type;
        const char *description = " ";

        if( iparam[i].pattern )
            pattern = iparam[i].pattern;
        if( iparam[i].subpattern )
            subpattern = iparam[i].subpattern;
        if( iparam[i].description )
            description = iparam[i].description;

        switch( iparam[i].type ){
        case E_PARAMS_TYPE_INT:
            type = "int";

        break;
        case E_PARAMS_TYPE_UINT:
            type = "uint";

        break;
        case E_PARAMS_TYPE_FLOAT:
            type = "float";

        break;
        case E_PARAMS_TYPE_STR:
            type = "string";

        break;
        case E_PARAMS_TYPE_BOOL:
            type = "boolean";

        break;
        default:
            continue;
        break;
        }

        int line_start = 0;
        char dest[ DESCRIPT_MAX_SIZE + 1 ];
        bool is_first = true;
        while(true){
            line_start = param_get_wrap_word( dest, description, line_start );
            if( is_first ){
                snprintf( info, HELPS_BUF_SIZE, template_firstline, pattern,
                        subpattern, type, dest );
                is_first = false;
            }else{
                snprintf( info, HELPS_BUF_SIZE, template_otherline, dest );
            }
            DLLOGV( "%s", info );
            if( line_start < 0 ) break;
        }

    }
    DLLOGV("================================================================================");
}

void param_show_values( m_params *iparam, int inum ){
    int i;

    int slen;
    int llen;
    int llen_max = 6;
    int slen_max = 10;

    char template_int[128];
    char template_uint[128];
    char template_float[128];
    char template_str[128];
    char info[ 256 ];

    for( i = 0; i < inum; i++ ){
        int llen = 0;
        int slen = 0;
        if( iparam[i].pattern )
            llen = strlen( iparam[i].pattern );
        if( iparam[i].subpattern )
             slen = strlen( iparam[i].subpattern );
        llen_max = (llen_max > llen)?llen_max:llen;
        slen_max = (slen_max > slen)?slen_max:slen;
    }

    snprintf( template_int, 128, "%%-%ds | %%-%ds | %%-8s | %%d", llen_max, slen_max );
    snprintf( template_uint, 128, "%%-%ds | %%-%ds | %%-8s | %%u", llen_max, slen_max );
    snprintf( template_float, 128, "%%-%ds | %%-%ds | %%-8s | %%f", llen_max, slen_max );
    snprintf( template_str, 128, "%%-%ds | %%-%ds | %%-8s | %%s", llen_max, slen_max );
    // DLLOGD( "%s", template_int );
    // DLLOGD( "%s", template_uint );
    // DLLOGD( "%s", template_float );
    // DLLOGD( "%s", template_str );

    DLLOGV("================================================================================");
    snprintf( info, 256, template_str, "option", "sub option", "type", "value",
            "description" );
    DLLOGV( "%s", info );
    DLLOGV("--------------------------------------------------------------------------------");
    for( i = 0; i < inum; i++ ){
        const char *pattern = " ";
        const char *subpattern = " ";
        const char *value = " ";
        const char *description = " ";

        if( iparam[i].pattern )
            pattern = iparam[i].pattern;
        if( iparam[i].subpattern )
            subpattern = iparam[i].subpattern;
        if( iparam[i].description )
            description = iparam[i].description;

        switch( iparam[i].type ){
        case E_PARAMS_TYPE_INT:
            snprintf( info, 256, template_int, pattern,
                    subpattern, "int", iparam[i].data.aint );
        break;
        case E_PARAMS_TYPE_UINT:

            snprintf( info, 256, template_uint, pattern,
                    subpattern, "uint", iparam[i].data.auint );

        break;
        case E_PARAMS_TYPE_FLOAT:

            snprintf( info, 256, template_float, pattern,
                    subpattern, "float", iparam[i].data.afloat );

        break;
        case E_PARAMS_TYPE_STR:
            if( iparam[i].data.astr )
                value = iparam[i].data.astr;

            snprintf( info, 256, template_str, pattern,
                    subpattern, "string", value );

        break;
        case E_PARAMS_TYPE_BOOL:
        if( iparam[i].data.abool )

            snprintf( info, 256, template_str, pattern,
                    subpattern, "boolean", "true" );
        else

            snprintf( info, 256, template_str, pattern,
                    subpattern, "boolean", "false" );
        break;
        }
        DLLOGV( "%s", info );
    }
    DLLOGV("================================================================================");
}

// =============================================================================

// test param_get_wrap_word
// const char *test[] = {
//     "xx",
//     "xx xxx",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx12",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx12",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx123",
//     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx3 12xxxxxxxxxxxxxxxxxxxxxxxxxxxxx123",
//     NULL,
// };
// 
// char dest[81];
// for( int i = 0;; i++ ){
//     if( test[i] == NULL ) break;
//     int start = 0;
//     while(true){
//         start = param_get_wrap_word( dest, test[i], start );
// 
//         DLLOGD( "%s", dest );
//         if( start == -1 ) break;
//     }
//     DLLOGD("");
// }
