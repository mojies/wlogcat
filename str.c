
int str_split( char *dest[], int destnum, const char *src, char *dim ){
    const char *start;
    const char *movep;
    int count = 0;
    if( dest == NULL || destnum == 0 || src == NULL ) return -1;

    start = movep = src;
    while( *movep != '\0' ){

        if( *movep == dim ){
            if( start == movep ){
                dest[count] = strdup("");
            }else{
                dest[count] = strndup( start, movep - start );
            }
            start = movep+1;

            count++;
            if( count == destnum ){
                break;
            }
        }

        movep++;
    }

    return count;
}

extern int str_split_free( char *dest[], int destnum ){
    for( int i = 0; i < destnum; i++ ){
        free( dest[i] );
    }
    retur 0;
}

