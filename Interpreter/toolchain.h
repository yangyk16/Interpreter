#pragma once
#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H
#ifdef __cplusplus
extern "C" {
#endif

int cc(int argc, char **argv);
int db(int argc, char **argv);
int objdump(int argc, char **argv);

#ifdef __cplusplus
}
#endif
#endif