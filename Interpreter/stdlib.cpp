#include <stdarg.h>
#include <stdio.h>
#include "hal.h"
#include "cstdlib.h"

#define ZEROPAD 1               /* pad with zero */
#define SIGN    2               /* unsigned/signed long */
#define PLUS    4               /* show plus */
#define SPACE   8               /* space if plus */
#define LEFT    16              /* left justified */
#define SPECIAL 32              /* 0x */
#define LARGE   64              /* use 'ABCDEF' instead of 'abcdef' */
#define FLOAT   128
#define is_digit(c)     ((c) >= '0' && (c) <= '9')
inline int do_div(long &n,unsigned int base)
{ 
	int __res; \
	__res = ((unsigned long) n) % (unsigned) base; \
	n = ((unsigned long) n) / (unsigned) base; \
	return __res; 
}

static int skip_atoi(const char **s)
{
        int i=0;

        while (is_digit(**s))
                i = i*10 + *((*s)++) - '0';
        return i;
}

static char * float_number(char *str, double num, int precision)
{
	int i, j, k, r_digits;
	double num_bak = num;
	if(num > 1 || num < -1) {
		if(num < 0) {
			*str++ = '-';
			num = -num;
		}
		for(i=-1; num>1; i++) {
			num /= 10;
		}
		num *= 10;
		*str++ = ((int)num % 10) + '0';
		*str++ = '.';
		for(j=0; j<4; j++) {
			num *= 10;
			*str++ = ((int)num % 10) + '0';
		}
		if(num_bak <= -10 || num_bak >= 10)
			*str++ = 'e';
		for(k=i, r_digits=0; k; k/=10, r_digits++) {
		}
		for(j=r_digits-1; j>=0; j--) {
			str[j] = i % 10 + '0';
			i /= 10;
		}
		str += r_digits;
        //      str += sprintf(str, "%d", i);
	} else if(num == 1 || num == -1) {
		if(num == -1)
			*str++ = '-';
		*str++ = '1';
		*str++ = '.';
		*str++ = '0';
	} else if(num == 0) {
		i = 0;
		*str++ = '0';
		*str++ = '.';
		*str++ = '0';
	} else {
		if(num < 0) {
			*str++ = '-';
			num = -num;
		}
		for(i=0; num<1 && num>-1; i--) {
			num *= 10;
		}
		*str++ = ((int)num % 10) + '0';
		*str++ = '.'; 
		for(j=0; j<4; j++) {
			num *= 10;
			*str++ = ((int)num % 10) + '0';
		}
		*str++ = 'e';
		*str++ = '-';
		i = -i;
		for(k=i, r_digits=0; k; k/=10, r_digits++) {
		}
		for(j=r_digits-1; j>=0; j--) {
			str[j] = i % 10 + '0';
			i /= 10;
		}
		str += r_digits;
	}
	return str;
}

static char * number(char * str, long num, int base, int size, int precision ,int type)
{
        char c,sign,tmp[66];
        const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
        int i;

        if (type & LARGE)
                digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (type & LEFT)
                type &= ~ZEROPAD;
        if (base < 2 || base > 36)
                return 0;
        c = (type & ZEROPAD) ? '0' : ' ';
        sign = 0;
        if (type & SIGN) {
                if (num < 0) {
                        sign = '-';
                        num = -num;
                        size--;
                } else if (type & PLUS) {
                        sign = '+';
                        size--;
                } else if (type & SPACE) {
                        sign = ' ';
                        size--;
                }
        }
        if (type & SPECIAL) {
                if (base == 16)
                        size -= 2;
                else if (base == 8)
                        size--;
        }
        i = 0;
        if (num == 0)
                tmp[i++]='0';
        else while (num != 0)
                tmp[i++] = digits[do_div(num,base)];
        if (i > precision)
                precision = i;
        size -= precision;
        if (!(type&(ZEROPAD+LEFT)))
                while(size-->0)
                        *str++ = ' ';
        if (sign)
                *str++ = sign;
       if (type & SPECIAL) {
                if (base==8)
                        *str++ = '0';
                else if (base==16) {
                        *str++ = '0';
                        *str++ = digits[33];
                }
        }
        if (!(type & LEFT))
                while (size-- > 0)
                        *str++ = c;
        while (i < precision--)
                *str++ = '0';
        while (i-- > 0)
                *str++ = tmp[i];
        while (size-- > 0)
                *str++ = ' ';
        return str;
}

unsigned int kstrnlen(const char * s, unsigned int count)
{
        const char *sc;

        for (sc = s; count-- && *sc != '\0'; ++sc)
                /* nothing */; 
        return sc - s;
}
/*stdlib interface*/
static int kvsprintf(char *buf, const char *fmt, va_list args)
{
        int len;
        unsigned long num;
        int i, base;
        char * str;
        const char *s;

        double float_num;
        int flags;              /* flags to number() */

        int field_width;        /* width of output field */
        int precision;          /* min. # of digits for integers; max
                                   number of chars for from string */
        int qualifier;          /* 'h', 'l', or 'L' for integer fields */

        for (str=buf ; *fmt ; ++fmt) {
                if (*fmt != '%') {
                        *str++ = *fmt;
                        continue;
                }

                /* process flags */
                flags = 0;
                repeat:
                        ++fmt;          /* this also skips first '%' */
                        switch (*fmt) {
                                case '-': flags |= LEFT; goto repeat;
                                case '+': flags |= PLUS; goto repeat;
                                case ' ': flags |= SPACE; goto repeat;
                                case '#': flags |= SPECIAL; goto repeat;
                                case '0': flags |= ZEROPAD; goto repeat;
                                }
                field_width = -1;
                if (is_digit(*fmt))
                        field_width = skip_atoi(&fmt);
                else if (*fmt == '*') {
                        ++fmt;
                        /* it's the next argument */
                        field_width = va_arg(args, int);
                        if (field_width < 0) {
                                field_width = -field_width;
                                flags |= LEFT;
                        }
                }

                /* get the precision */
                precision = -1;
                if (*fmt == '.') {
                        ++fmt;
                        if (is_digit(*fmt))
                                precision = skip_atoi(&fmt);
                        else if (*fmt == '*') {
                                ++fmt;
                                /* it's the next argument */
                                precision = va_arg(args, int);
                        }
                        if (precision < 0)
                                precision = 0;
                }

                /* get the conversion qualifier */
                qualifier = -1;
                if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
                        qualifier = *fmt;
                        ++fmt;
                }
                base = 10;

                switch (*fmt) {
                case 'c':
                        if (!(flags & LEFT))
                                while (--field_width > 0)
                                        *str++ = ' ';
                        *str++ = (unsigned char) va_arg(args, int);
                        while (--field_width > 0)
                                *str++ = ' ';
                        continue;

                case 's':
                        s = va_arg(args, char *);
                        if (!s)
                                s = "<NULL>";

                        len = kstrnlen(s, precision);

                        if (!(flags & LEFT))
                                while (len < field_width--)
                                        *str++ = ' ';
                        for (i = 0; i < len; ++i)
                                *str++ = *s++;
                        while (len < field_width--)
                                *str++ = ' ';
                        continue;

                case 'p':
                        if (field_width == -1) {
                                field_width = 2*sizeof(void *);
                                flags |= ZEROPAD;
                        }
                        str = number(str,
                                (unsigned long) va_arg(args, void *), 16,
                                field_width, precision, flags);
                        continue;


                case 'n':
                        if (qualifier == 'l') {
                                long * ip = va_arg(args, long *);
                                *ip = (str - buf);
                        } else {
                                int * ip = va_arg(args, int *);
                                *ip = (str - buf);
                        }
                        continue;

                case '%':
                        *str++ = '%';
                        continue;

                /* integer number formats - set up the flags and "break" */
                case 'o':
                        base = 8;
                        break;

                case 'X':
                        flags |= LARGE;
                case 'x':
                        base = 16;
                        break;

                case 'd':
                case 'i':
                        flags |= SIGN;
                case 'u':
                        break;
                case 'f':
                        flags |= FLOAT;
                        break;
                default:
                        *str++ = '%';
                        if (*fmt)
                                *str++ = *fmt;
                        else
                                --fmt;
                        continue;
                }
                if (qualifier == 'l')
                        num = va_arg(args, unsigned long);
                else if (qualifier == 'h') {
                        num = (unsigned short) va_arg(args, int);
                        if (flags & SIGN)
                                num = (short) num;
                } else if (flags & SIGN)
                        num = va_arg(args, int);
                else if(flags & FLOAT)
                        float_num = va_arg(args, double);
                else
                        num = va_arg(args, unsigned int);
                if(flags & FLOAT)
                        str = float_number(str, float_num, precision);
                else
                        str = number(str, num, base, field_width, precision, flags);
        }
        *str = '\0';
        return str-buf;
}

int kprintf(const char *fmt, ...)
{
	va_list ap; 
	char string[0x400]; 
	va_start(ap,fmt);
	kvsprintf(string,fmt,ap);
	va_end(ap);
	kfputs(string);
	return 0;
}

int ksprintf(char *buf, const char *fmt, ...)
{
	va_list ap; 
	va_start(ap,fmt);
	kvsprintf(buf,fmt,ap);
	va_end(ap);
	return 0;
}

void* kmemcpy(void *d, const void *s, unsigned int size)
{//TODO：加入空指针检查？可能不需要
	unsigned int i, j, remain;
	if(size < 64 || (long)d & 1 || (long)s & 1) {
		for(i=0; i<size; i++) {
			*(char*)((long)d + i) = *(char*)((long)s + i);
		}
	} else {
		if(!((long)s & 3) && !((long)d & 3)) {
			remain = size & 3;
			for(i=0; i<size; i+=4) {
				*(int*)((unsigned long)d + i) = *(int*)((unsigned long)s + i);
			}
			for(j=size-remain; j<size; j++)
				*(char*)((long)d + j) = *(char*)((long)s + j);
		} else if(!((long)s & 1) && !((long)d & 1)) {
			remain = size & 1;
			for(i=0; i<size; i+=2) {
				*(short*)((unsigned long)d + i) = *(short*)((unsigned long)s + i);
			}
			for(j=size-remain; j<size; j++)
				*(char*)((long)d + j) = *(char*)((long)s + j);
		} else {
			//比对结尾位置是否4/2Byte对齐
		}
	}
	return d;
}

void* kmemset(register void *d, int ch, register unsigned int size)
{
	register int unsigned i, j;
	int block, remain;
	if(size < 64 || (long)d & 1) {
		for(i=0; i<size; i++) {
			*(char*)((long)d + i) = ch;
		}
	} else {
		if(!((long)d & 3)) {
			remain = size & 3;
			*(char*)&block = ch;
			*(char*)((unsigned long)&block + 1) = ch;
			*(char*)((unsigned long)&block + 2) = ch;
			*(char*)((unsigned long)&block + 3) = ch;
			for(i=0; i<size; i+=4) {
				*(int*)((unsigned long)d + i) = block;
			}
			for(j=size-remain; j<size; j++)
				*(char*)((long)d + j) = ch;
		} else if(!((long)d & 1)) {
			remain = size & 3;
			*(char*)&block = ch;
			*(char*)((unsigned long)&block + 1) = ch;
			for(i=0; i<size; i+=2) {
				*(short*)((unsigned long)d + i) = (short)block;
			}
			for(j=size-remain; j<size; j++)
				*(char*)((long)d + j) = ch;
		}
	}
	return d;
}

unsigned int kstrlen(const char *str)
{
	unsigned int i = 0;
	while(*str++)
		i++;
	return i;
}

int kstrcmp(const char *str1, const char *str2)
{
	while(*str1 && *str2) {
		if(*str1++ != *str2++)
			return -1;
	}
	if(*str1 == *str2)
		return 0;
	else
		return -1;
}

char *kstrcpy(char *d, const char *s)
{
	int i;
	for(i=0; s[i]; i++) {
		d[i] = s[i];
	}
	d[i] = 0;
	return d;
}

double katof(const char* sptr)
{
	double temp = 10;
	bool ispnum = true;
	double ans = 0;
	int index = 0;
	while(*sptr == ' ' || *sptr == '\t')
		sptr++;
	if(*sptr == '-') {
		ispnum = false;
		sptr++;
	} else if(*sptr == '+') {
		sptr++;
	}
	while(kisdigit(*sptr)) {
		ans = ans * 10 + (*sptr - '0');
		sptr++;
	}
	if(*sptr == '.')
		sptr++;
	while(kisdigit(*sptr)) {
		ans = ans + (*sptr - '0') / temp;
		temp *= 10;
		sptr++;
	}
	if(*sptr == 'e' || *sptr == 'E')
		sptr++;
	else
		goto negative_calc;
	index = katoi(sptr);
	if(index > 0) {
		for(int n=0; n<index; n++)
			ans *= 10;
	} else if(index < 0) {
		for(int n=0; n>index; n--)
			ans /= 10;
	}
negative_calc:
	if(ispnum)
		return ans;
	else
		return -ans;
}

int katoi(const char* sptr)
{
	bool ispnum=true;
	int ans=0;
	while(*sptr == ' ' || *sptr == '\t')
		sptr++;
	if(*sptr=='-') {
		ispnum=false;
		sptr++;
	} else if(*sptr=='+') {
        sptr++;
    }
	if(sptr[0] == '0' && (sptr[1] == 'x' || sptr[1] == 'X')) {
		sptr += 2;
		while(1) {
			if(*sptr >= '0' && *sptr <= '9')
				ans = ans * 16 + *sptr - '0';
			else if(*sptr >= 'a' && *sptr <= 'f')
				ans = ans * 16 + *sptr - 'a' + 10;
			else if(*sptr >= 'A' && *sptr <= 'F')
				ans = ans * 16 + *sptr - 'A' + 10;
			else
				break;
			sptr++;
		}
		goto negative_calc;
	}
	while(*sptr)//类型转化
	{
		ans=ans*10+(*sptr-'0');
		sptr++;
	}
negative_calc:
	if(ispnum)
		return ans;
	else
		return -ans;
}

bool kisdigit(unsigned char ch)//ctype.h
{
	return ch >= '0' && ch <= '9'? true: false;
}