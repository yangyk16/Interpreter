#ifdef __GNUC__
#define weak __attribute__((weak)) 
#else
#define weak __weak
#endif
extern "C" void _exit(void){}
extern "C" void _fstat(void){}
extern "C" void _sbrk(void){}
extern "C" void _kill(void){}
extern "C" void _getpid(void){}
extern "C" void _write(void){}
extern "C" void _close(void){}
extern "C" void _isatty(void){}
extern "C" void _lseek(void){}
extern "C" void _read(void){}
extern "C" void weak __dso_handle(void){}
extern "C" void __cxa_pure_virtual(void){}
