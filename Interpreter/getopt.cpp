#include "getopt.h"
#include "cstdlib.h"

char *optarg;
int optind = 1, opterr, optopt;
static int string_pos = 0;
int kgetopt(int argc, char *argv[], const char *optstring)
{
	int i, j;
	int argflag;
	int move_flag = 0;
	int size;
	do {
		if(optind == argc) {
			if(string_pos)
				optind = string_pos;
			else
				optind = argc;
			return -1;
		}
		optarg = NULL;
		unsigned int len = kstrlen(argv[optind]);
		unsigned int arglen = kstrlen(optstring);
		if(argv[optind][0] == '-') {
			for(i=1; i<len; i++) {
				for(j=0; j<arglen; j++) {
					if(argv[optind][i] == optstring[j]) {
						if(optstring[j + 1] == ':') {
							if(optstring[j + 2] == ':')
								argflag = 2;
							else
								argflag = 1;
						} else
							argflag = 0;
						if(argflag) {
							if(argv[optind][i + 1]) {
								optarg = &argv[optind][i + 1];
								size = 1;
								goto postfind;
							} else {
								if(argflag == 1) {
									if(optind + 1 < argc && argv[optind + 1][0] != '-') {
										optarg = argv[optind + 1];
										size = 2;
										goto postfind;
									} else {
										optarg = 0;
										return '?';
									}
								} else {
									if(optind + 1 < argc && argv[optind + 1][0] != '-') {
										optarg = argv[optind + 1];
										size = 2;
									} else {
										optarg = 0;
										size = 1;
									}
									goto postfind;
								}
							}
						} else {
							optarg = 0;
							size = 1;
							goto postfind;
						}
					}
				}
			}
		} else {
			optind++;
			if(!string_pos) {
				string_pos = optind - 1;
				continue;
			}
		}
	} while(1);
postfind:
	if(string_pos) {
		char *opt_bak, *arg_bak;
		if(size == 2) {
			opt_bak = argv[optind];
			arg_bak = argv[optind + 1];
			for(i=optind+1; i>string_pos+1; i--)
				argv[i] = argv[i - 2];
			argv[string_pos] = opt_bak;
			argv[string_pos + 1] = arg_bak;
			string_pos += 2;
		} else {
			opt_bak = argv[optind];
			for(i=optind; i>string_pos; i--)
				argv[i] = argv[i - 1];
			argv[string_pos] = opt_bak;
			string_pos++;
		}
	}
	optind += size;
	return optstring[j];
}