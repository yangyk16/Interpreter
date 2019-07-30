#pragma once
#ifndef GETOPT_H
#define GETOPT_H

extern char *optarg;
extern int optind, opterr, optopt;
#ifdef __cplusplus
extern "C" {
#endif
int kgetopt(int argc, char *argv[], const char *optstring);
void koptrst(void);
#ifdef __cplusplus
}
#endif
#endif
