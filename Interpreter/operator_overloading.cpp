#include "varity.h"

int min(int a, int b){return a>b?b:a;}
varity_info operator+(varity_info& obj1, varity_info& obj2)
{
	varity_info ret;
	ret.type = min(obj1.type, obj2.type);
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(obj1.type == obj2.type) {
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) + *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) + *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR || ret.type == CHAR) {
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) + *(char*)(obj2.content_ptr);
		} else if(ret.type = DOUBLE) {
			*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) + *(double*)(obj2.content_ptr);
		} else if(ret.type == FLOAT) {
			*(float*)(ret.content_ptr) = *(float*)(obj1.content_ptr) + *(float*)(obj2.content_ptr);
		}
	} else {
		
	}
	return ret;
}