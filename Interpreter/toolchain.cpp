#include "interpreter.h"
#include "getopt.h"
#include "global.h"
#include "error.h"

extern "C" int cc(int argc, char **argv)
{
	int ret;
	char ch; 
	int run_flag = 0;
	int link_flag = LINK_ADDR;
	int extra_flag = 0;
	const char *output_file_name = NULL;
	const char *log_file_name = NULL;
	char *fun_file_name = NULL;
	void *load_base, *bss_base;
	int stack_size = STACK_SIZE;
	while((ch = kgetopt(argc, (char**)argv, "iceo:rgw:")) != (char)-1) {
		switch(ch) {
		case 'i':
			link_flag = LINK_ADDR;
			break;
		case 'c':
			link_flag = LINK_STRNO; 
			break;
		case 'e':
			link_flag = LINK_NUMBER;
			break;
		case 'o':
			output_file_name = optarg;
			break;
		case 'w':
			log_file_name = optarg;
			break;
		case 'r':
			run_flag = 1;
			break;
		case 'g':
			extra_flag |= EXTRA_FLAG_DEBUG;
			break;
		}
		debug("opt=%c,arg=%s\n", ch, optarg);
	}
	if(run_flag) {
		myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 0, stack_size);
		irq_interpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 0, stack_size);
		ret = myinterpreter.load_ofile(argv[optind], 0, &load_base, &bss_base);
		if(ret)
			return ret;
		ret = myinterpreter.run_main(STOP_FLAG_RUN, load_base, bss_base);
		myinterpreter.dispose();
		return ret;
	}
	switch(link_flag) {
		case LINK_ADDR:
			if(optind == argc) {
				myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 1, stack_size);
			} else {
				myinterpreter.init(&fileio, RTL_FLAG_IMMEDIATELY, 1, stack_size);
				ret = fileio.init(argv[optind], "r");
				if(ret)
					return ret;
			}
			if(log_file_name) {
				lfileio.init(log_file_name, "w");
				myinterpreter.set_tty(0, &lfileio);
			}
			irq_interpreter.init(&stdio, 1, 0, stack_size);
			myinterpreter.run_interpreter();
			myinterpreter.dispose();
			break;
		case LINK_STRNO:
		{
			int file_count = argc - optind;
			for(int i=0; i<file_count; i++) {
				ret = fileio.init(argv[optind + i], "r");
				if(ret) {
					error("open file %s failed\n", argv[optind + i]);
					return ERROR_FILE;
				}
				myinterpreter.init(&fileio, RTL_FLAG_DELAY, 1, stack_size);
				tip("compiling %s...\n", argv[optind + i]);
				myinterpreter.run_interpreter();
				int len = kstrlen(argv[optind + i]);
				argv[optind + i][len - 1] = 'o';
				ret = myinterpreter.tlink(LINK_STRNO);
				if(ret) {
					error("compile error\n");
					return ret;
				}
				tip("compile finish, writing to disk...\n");
				//compile(argv[optind + i], EXPORT_FLAG_LINK);
				myinterpreter.write_ofile(argv[optind + i], EXPORT_FLAG_LINK, extra_flag);
				tip("%s made success!\n", argv[optind + i]);
				myinterpreter.dispose();
			}
			break;
		}
		case LINK_NUMBER:
		{
			int file_count = argc - optind;
			myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 1, stack_size);
			for(int i=0; i<file_count; i++) {
				ret = myinterpreter.load_ofile(argv[optind + i], 0, &load_base, &bss_base);
				if(ret) {
					error("load file %s failed\n", argv[optind + i]);
					goto link_exit;
				}
			}
			tip("linking...\n");
			ret = myinterpreter.tlink(LINK_NUMBER);
			if(ret)
				return ret;
			if(!output_file_name)
				output_file_name = "a.elf";
			tip("link finish, writing to disk...\n");
			myinterpreter.write_ofile(output_file_name, EXPORT_FLAG_EXEC, extra_flag);
			tip("%s made success!\n", output_file_name);
			link_exit:
			myinterpreter.dispose();
			break;
		}
	}
	return 0;
}

extern "C" int db(int argc, char **argv)
{
	int ret;
	int stack_size = STACK_SIZE;
	void *load_base, *bss_base;
	if(argc < 2)
		return -1;
	else
		optind = 1;
	myinterpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 1, stack_size);
	irq_interpreter.init(&stdio, RTL_FLAG_IMMEDIATELY, 0, stack_size);
	ret = myinterpreter.load_ofile(argv[optind], 0, &load_base, &bss_base);
	if(ret)
		return ret;
	ret = myinterpreter.run_main(STOP_FLAG_STOP, load_base, bss_base);
	myinterpreter.dispose();
	return ret;
}

extern "C" int objdump(int argc, char **argv)
{
	return 0;
}
