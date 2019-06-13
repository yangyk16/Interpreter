#pragma once
#ifndef GETOPT_H
#define GETOPT_H

extern char *optarg;
extern int optind, opterr, optopt;

int kgetopt(int argc, char *argv[], const char *optstring);
#endif