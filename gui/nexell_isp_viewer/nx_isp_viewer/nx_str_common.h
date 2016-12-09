/* ------------------------------------------------------------------------ */
/* common header file														*/
/* ------------------------------------------------------------------------ */
#ifndef NX_STR_COMMON_H
#define NX_STR_COMMON_H

/* ------------------------------------------------------------------------ */
typedef	unsigned char	u8;
typedef	unsigned short	u16;
typedef	unsigned int	u32;
/* ------------------------------------------------------------------------ */

u32 atoh( char *str )
{
    u32		ret;

    if ( (str[0]=='0') && (str[1]=='x') )
        str = &(str[2]);

    ret = 0;
    while( 1 )
    {
        if ( (*str>='0') && (*str<='9') )
            ret = (ret << 4) | (*(str++) - '0');
        else if ( (*str>='A') && (*str<='F') )
            ret = (ret << 4) | (*(str++) - 'A' + 0xA);
        else if ( (*str>='a') && (*str<='f') )
            ret = (ret << 4) | (*(str++) - 'a' + 0xA);
        else	break;
    }

    return( ret );
}

/* ------------------------------------------------------------------------ */

u32 atod( char *str )
{
    if ( (str[0]=='x') || (str[0]=='X') )
        str = &(str[1]);
    else if ( !strncmp(str,"0x",2) || !strncmp(str,"0X",2) )
        str = &(str[2]);
    else	return( atoi(str) );

    return( atoh(str) );
}

/* ------------------------------------------------------------------------ */

int
fget_line( FILE *fp, char *str, int max_len )
{
    char	ch;

    *str = 0;

    while( (max_len--) && (!feof(fp)) )
    {
        ch = fgetc( fp );
        if ( feof(fp) )	break;

        *str = ch;
        if ( *str == '\n' )
        {
            ch = fgetc( fp );
            if ( feof(fp) )
            {
                *(str+1) = 0;
                break;
            }
            if ( ch != '\r' )
                fseek( fp, -1L, SEEK_CUR );
            *str = 0;
            return( 0 );
        }
        else if( *str == '\r' )
        {
            ch = fgetc( fp );
            if ( feof(fp) )
            {
                *(str+1) = 0;
                break;
            }
            if ( ch != '\n' )
                fseek( fp, -1L, SEEK_CUR );
            *str = 0;
            return( 0 );
        }

        str++;
    }
    return( 1 );
}

/* ------------------------------------------------------------------------ */

char*
str_strip( char *str, const char *del )
{
    int		f_find, len, i, j;

    /* --- find backward --- */
    len = strlen( str );
    for( i=(len-1) ; i>=0 ; i-- )
    {
        if ( str[i] == 0 )	continue;	// check null

        f_find = 0;
        for( j=0 ; j<(int)strlen(del) ; j++ )
        {
            if ( str[i] == del[j] )		// find delimiter
            {
                str[i] = 0;
                f_find = 1;
            }
        }
        if ( f_find == 0 )	break;		// if normal character
    }

    /* --- find forward --- */
    len = strlen( str );
    for( i=0 ; i<len ; i++ )
    {
        if ( str[i] == 0 )	continue;	// check null

        f_find = 0;
        for( j=0 ; j<(int)strlen(del) ; j++ )
        {
            if ( str[i] == del[j] )		// find delimiter
            {
                str[i] = 0;
                f_find = 1;
            }
        }
        if ( f_find == 0 )	break;		// if normal character
    }

    return( &(str[i]) );
}

/* ------------------------------------------------------------------------ */

char*
str_token( char *str, const char *del )
{
    u32				i, j, len, f_find;
    char			*p;
    static	char	*next_p;

    /* --- set start pointer --- */
    if( str != NULL )
            p = str;
    else	p = next_p;

    len = strlen( p );
    f_find = 0;
    for( i=0 ; i < len ; i++ )
    {
        if ( p[i] == 0 )		break;	// if last of string

        for( j=0 ; j < (u32)strlen(del) ; j++ )
        {
            if ( p[i] == del[j] )	// find delimiter
            {
                f_find = 1;
                p[i] = 0;
                break;
            }
        }
        if ( f_find && (j == strlen(del) ) )	break;
    }

    next_p = &(p[i]);
    return( p );
}

/* ------------------------------------------------------------------------ */

void
str_upper( char *str )
{
    int		i, len;

    len = strlen( str );
    for( i=0 ; i < len ; i++ )
    {
        if ( (str[i] >= 'a') && (str[i] <= 'z') )
            str[i] -= ('a' - 'A');
    }
}

/* ------------------------------------------------------------------------ */

void
str_lower( char *str )
{
    int		i, len;

    len = strlen( str );
    for( i=0 ; i<len ; i++ )
    {
        if ( (str[i]>='A') && (str[i]<='Z') )
            str[i] += ('a' - 'A');
    }
}

/* ------------------------------------------------------------------------ */
#endif // NX_STR_COMMON_H
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
