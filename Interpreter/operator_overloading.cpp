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
		varity_info obja, objb;
		obja = obj1.type==ret.type?obj1:obj2;
		objb = obj1.type==ret.type?obj2:obj1;
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) + *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) + *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR) {
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) + *(char*)(obj2.content_ptr);
		} else if(ret.type == DOUBLE) {
			if(objb.type == LONG || objb.type == INT || objb.type == SHORT || objb.type == CHAR) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) + (double)(*(int*)(objb.content_ptr));
			} else if(objb.type == U_LONG || objb.type == U_INT || objb.type == U_SHORT || objb.type == U_CHAR) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) + (double)(*(uint*)(objb.content_ptr));
			} else if(objb.type == FLOAT) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) + (double)(*(float*)(objb.content_ptr));
			}
		} else if(ret.type == FLOAT) {
			if(objb.type == LONG || objb.type == INT || objb.type == SHORT || objb.type == CHAR) {
				*(float*)(ret.content_ptr) = *(float*)(obja.content_ptr) + (float)(*(int*)(objb.content_ptr));
			} else if(objb.type == U_LONG || objb.type == U_INT || objb.type == U_SHORT || objb.type == U_CHAR) {
				*(float*)(ret.content_ptr) = *(float*)(obja.content_ptr) + (float)(*(uint*)(objb.content_ptr));
			}
		}
	}
	return ret;
}