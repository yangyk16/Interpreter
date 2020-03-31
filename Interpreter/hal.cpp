#include "hal.h"
#include "config.h"
#include "kmalloc.h"
#include "cstdlib.h"
#include "error.h"
#include "interpreter.h"
#include "global.h"
#include "string_lib.h"

#if TTY_TYPE == 0
#include <iostream>
#include <stdio.h>
using namespace std;

tty::tty(char echo_en): terminal(echo_en)
{
#ifdef __GNUC__
//	system("stty raw");
//	system("stty -inlcr -igncr icrnl");
//	system("stty -onlcr ocrnl -onocr -onlret nl0 cr0");
#endif
}

void tty::dispose(void)
{
#ifdef __GNUC__
//	system("stty sane");
#endif
}

int tty::t_putc(char ch)
{
	cout << ch;
	return 0;
}

int tty::t_getc(char *ch)
{
	*ch = getchar();
	return 1;
}
#elif TTY_TYPE == 1
#include "../testcase/ff.h"
#include "uart.h"
extern "C" char uart_getc(void);
int uart::t_getc(char* ch)
{
	*ch = uart_getc();//uart_getchar_Polling_n(0, 0);
	return 1;
};

int uart::t_putc(char ch)
{
	uart_putchar(0, ch);
	return 0;
};

#endif

int terminal::readline(char* string, char ch, code_fptr callback)//×îºó±ØÐë²¹0
{
	char *string2 = string;
	char c;
	int i = 0, j;
	int ret;
	int complete_flag = 0;
	char *word = string;
	int i_bak;
	int last_com_len = 0;
	int escape_flag = 0;

	*string = 0;
	while((ret = this->t_getc(&c)) == 1) {
		if(c == '\n') {
			if(this->last_ch == '\r') {
				this->last_ch = c;
				continue;
			} else {
				this->last_ch = c;
				break;
			}
		}
		if(c == '\r') {
			this->last_ch = c;
			this->t_putc(c);
			break;
		}
		switch(escape_flag) {
			char *cmd_str;
			case 0:
				if(c == 0x1B) {
					escape_flag = 1;
					continue;
				} else {
					break;
				}
			case 1:
				if(c == 0x5B) {
					escape_flag = 2;
					continue;
				} else {
					break;
				}
			case 2:
				if(c == 'A' || c == 'B') {//UP/DN
					if(c == 'A')
						cmd_str = cmd_up(0);
					else
						cmd_str = cmd_down(0);
					if(cmd_str) {
						int cmd_len = (int)kstrlen(cmd_str);
						for(j=0; j<i-cmd_len; j++)
							this->t_putc('\b');
						for(j=0; j<i-cmd_len; j++)
							this->t_putc(' ');
						for(j=0; j<i; j++)
							this->t_putc('\b');
						kstrcpy(string2, cmd_str);
						i = kstrlen(string2);
						string = string2 + i;
						this->t_puts(string2);
					}
				} else if(c == 'D') {//LF
				} else if(c == 'C') {//RG
				} else if(c == '1' || c == '4') {
					escape_flag = 3;
					continue;
				}
				escape_flag = 0;
				complete_flag = 0;
				last_com_len = 0;
				continue;
		}
		if(c == ch) {
			if(!callback)
				continue;
			if(complete_flag++ != 0) {
				string2[i_bak] = 0;
			} else
				i_bak = i;
			char *ret = callback(string2, complete_flag);
			string -= last_com_len;
			i -= last_com_len;
			for(j=0; j<last_com_len; j++)
				this->t_putc('\b');
			for(j=0; j<last_com_len; j++)
				this->t_putc(' ');
			for(j=0; j<last_com_len; j++)
				this->t_putc('\b');
			if(ret) {
				kstrcpy(string, ret);
				last_com_len = kstrlen(ret);
				this->t_puts(ret);
				string += last_com_len;
				i += last_com_len;
			} else {
				last_com_len = 0;
				complete_flag = 0;
			}
			*string = 0;
		} else {
			complete_flag = 0;
			last_com_len = 0;
		}

		if(c == '\b') {
			if((int)string2 < (int)string) {
				if(*--string == '\t')
					this->t_puts("\b\b\b\b");
				else
					this->t_puts("\b \b");
				*string = 0;
				i--;
			}
		} else if(c != ch) {
			if(c == '\t') {
				if(this->echo_en)
					this->t_puts("    ");
			} else {
				if(this->echo_en)
					this->t_putc(c);
			}
			i++;
			*string++ = c;
			*string = 0;
			this->last_ch = c;
		}
	}
	*string='\0';
	if(!ret && !i)
		return -1;
	if(this->echo_en)
		this->t_putc('\n');
	cmd_enter(string2, 0);
	return i;
};

int file::t_getc(char *ch)
{
	return kfread(ch, 1, 1, file_ptr);
}

int file::t_putc(char ch)
{
	return kfwrite(&ch, 1, 1, this->file_ptr);
}

int file::init(const char *filename, const char *mode)
{
	file_ptr = kfopen(filename, mode);
	if(file_ptr)
		return 0;
	else
		return -1;
}

void file::dispose(void)
{
	kfclose(file_ptr);
}

int kfputs(char *str)
{
#if TTY_TYPE == 0
	printf("%s", str);
#else
	uart_sendstring(str);
#endif
	return 0;
}

void *kfopen(const char *filename, const char *mode)
{
#if TTY_TYPE == 0
	return fopen(filename, mode);
#else
	int ret;
	int tmode = 0;
	for(int i=0; mode[i]; i++) {
		if(mode[i] == 'r')
			tmode |= FA_READ; 
		if(mode[i] == 'w')
			tmode |= FA_WRITE | FA_CREATE_ALWAYS; 
	}
	FIL *file = (FIL*)dmalloc(sizeof(FIL), "");
	ret = f_open (file, filename, tmode);	/* Open or create a file */
	if(!ret)
		return file;
	else {
		vfree(file);
		return 0;
	}
#endif
}

int kfclose(void *fp)
{
#if TTY_TYPE == 0
	return fclose((FILE*)fp);
#else
	if(fp) {
		vfree(fp);
		return f_close((FIL*)fp);
	}
	return 0;
#endif
}

unsigned int kfread(void *buffer, unsigned int size, unsigned int nmemb, void *fileptr)
{
#if TTY_TYPE == 0
	return fread(buffer, size, nmemb, (FILE*)fileptr);
#else
	unsigned int ret;
	f_read((FIL*)fileptr, buffer, size * nmemb, &ret);
	return ret;
#endif
}

unsigned int kfwrite(void *buffer, unsigned int size, unsigned int count, void *fileptr)
{
#if TTY_TYPE == 0
	int ret;
	ret = fwrite(buffer, size, count, (FILE*)fileptr);
	fflush((FILE*)fileptr);
	return ret;
#else
	unsigned int ret;
	f_write((FIL*)fileptr, buffer, size * count, &ret);
	return ret;
#endif
}

unsigned int vfread(void* buffer, unsigned int size, unsigned int nmemb, void* fileptr, char* log)
{
	static int total_read = 0;
	total_read += size * nmemb;
	filedebug("read %d byte,total %d, %s\n", size * nmemb, total_read, log);
	return kfread(buffer, size, nmemb, fileptr);
}

unsigned int vfwrite(void* buffer, unsigned int size, unsigned int count, void* fileptr, char* log)
{
	static int total_write = 0;
	if(size * count == 0)
		return 0;
	total_write += size * count;
	filedebug("write %d byte, total %d, %s\n", size * count, total_write, log);
	return kfwrite(buffer, size, count, fileptr);
}

void *irq_service[32];
void *irq_data[32];
#if HW_PLATFORM == PLATFORM_X86
#define IRQ
#else
#define IRQ __attribute__((interrupt ("IRQ")))
extern "C" void irq_MASK(int base,int priority);
extern "C" void irq_UNMASK(int base,int priority);
#endif
void irq_mask(int irq_no)
{
#if HW_PLATFORM == PLATFORM_X86
#else
	irq_MASK(0, irq_no);
#endif
}

void irq_unmask(int irq_no)
{
#if HW_PLATFORM == PLATFORM_X86
#else
	irq_UNMASK(0, irq_no);
#endif
}

void irq_entry(void) IRQ;
void irq_entry(void)
{
#if HW_PLATFORM == PLATFORM_ARM
	int irq_no = 31 - __builtin_clz(*(int*)0x2310030);
	irq_mask(irq_no);
	if(irq_service[irq_no]) {
		stack *mid_code_stack = &((function_info*)irq_service[irq_no])->mid_code_stack;
		irq_interpreter.exec_mid_code((mid_code*)mid_code_stack->get_base_addr(), mid_code_stack->get_count());
	}
	irq_unmask(irq_no);
#endif
}

extern "C" void irq_init(int base,int priority, void (*isr_fun)(void));
void irq_reg(int irq_no, void *func_ptr, void *data)
{
	irq_service[irq_no] = func_ptr;
	irq_data[irq_no] = data;
	//intreg(irq_no, irq_entry, 0);
#if HW_PLATFORM == PLATFORM_ARM
	irq_init(0, irq_no, irq_entry);
#endif
}

int hard_fault_check(unsigned int addr)
{
#if TTY_TYPE == 1
	if(addr < 0x200000 || addr >= 0x18000000 && addr < 0xFFFF0000 || (addr > 0x47FFFF && addr < 0x2000000) || (addr > 0x2400000 && addr <= 0x10000000)) {
		return ERROR_HARD_FAULT;
	}
#else
#endif
	return ERROR_NO;
}
