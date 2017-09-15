#pragma once

#ifndef HAL_H
#define HAL_H

class terminal{
public:
	virtual int readline(char*) = 0;
	virtual int puts(char*) = 0;
};

class tty: public terminal {
public:
	virtual int readline(char*);
	virtual int puts(char*);
};
#endif