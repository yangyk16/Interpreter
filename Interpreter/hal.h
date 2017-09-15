#pragma once

#ifndef HAL_H
#define HAL_H

class terminal{
	virtual int readline(char*) = 0;
};

class tty: public terminal {
	virtual int readline(char*);
};
#endif