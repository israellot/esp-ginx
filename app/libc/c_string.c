#include "c_string.h"

char* itoa(int i, char b[])  // Convert Integer to ASCII!!
{
    char const digit[] = "0123456789";
    char* p = b;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}

char * os_strcati(char *dest,int i){

	char * p = dest;
	while(*p!='\0') p++;

	itoa(i,p);
	
	p++;
	*p='\0';
	
    return dest;
}

// #define _tolower(__c) ((unsigned char)(__c) - 'A' + 'a')
// #define _toupper(__c) ((unsigned char)(__c) - 'a' + 'A')

// int tolower(int c){

//     if(isupper(c))
//         return _tolower(c);
//     else
//         return c;

// }

int os_strcasecmp(const char *s1, const char *s2)
{
    while (tolower(*s1) == tolower(*s2++))
    {
        if (*s1++ == '\0')
        {
            return 0;
        }
    }

    return *(unsigned char *)s1 - *(unsigned char *)(s2 - 1);
}

// const char *c_strstr(const char * __s1, const char * __s2){
// }

// char *c_strncat(char * s1, const char * s2, size_t n){
// }

// size_t c_strcspn(const char * s1, const char * s2){
// }

// const char *c_strpbrk(const char * s1, const char * s2){
// }

// int c_strcoll(const char * s1, const char * s2){
// }

