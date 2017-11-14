#include "varity.h"
#include <stdio.h>
#include "operator.h"
//TODO: 4 macros rewrite to avoid char in struct be used as integer.
#define OPERATOR_OVERLOAD_MACRO(opt) \
	if(obj1.type == obj2.type) { \
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(ret.type == U_SHORT || ret.type == SHORT) { \
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(ret.type == U_CHAR || ret.type == CHAR) { \
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} else if(ret.type = DOUBLE) { \
			*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt *(double*)(obj2.content_ptr); \
		} else if(ret.type == FLOAT) { \
			*(float*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt *(float*)(obj2.content_ptr); \
		} \
	} else { \
		int mintype = min(obj1.type, obj2.type); \
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(mintype == U_SHORT || mintype == SHORT) { \
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(mintype == U_CHAR) { \
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} else if(mintype == DOUBLE) { \
			if(obj1.type == DOUBLE) { \
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) { \
					*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(int*)(obj2.content_ptr)); \
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) { \
					*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(uint*)(obj2.content_ptr)); \
				} else if(obj2.type == FLOAT) { \
					*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(float*)(obj2.content_ptr)); \
				} \
			} else { \
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) { \
					*(double*)(ret.content_ptr) = (double)(*(int*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) { \
					*(double*)(ret.content_ptr) = (double)(*(uint*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} else if(obj1.type == FLOAT) { \
					*(double*)(ret.content_ptr) = (double)(*(float*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} \
			} \
		} else if(mintype == FLOAT) { \
			if(obj1.type == FLOAT) { \
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) { \
					*(float*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt (float)(*(int*)(obj2.content_ptr)); \
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) { \
					*(float*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt (float)(*(uint*)(obj2.content_ptr)); \
				} \
			} else { \
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) { \
					*(float*)(ret.content_ptr) = (float)(*(int*)(obj1.content_ptr)) opt *(float*)(obj2.content_ptr); \
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) { \
					*(float*)(ret.content_ptr) = (float)(*(uint*)(obj1.content_ptr)) opt *(float*)(obj2.content_ptr); \
				} \
			} \
		} \
	} \
	return ret

#define OPERATOR_OVERLOAD_MACRO_RET_INT(opt) \
	if(obj1.type == obj2.type) { \
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(ret.type == U_SHORT || ret.type == SHORT) { \
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(ret.type == U_CHAR || ret.type == CHAR) { \
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} else if(ret.type = DOUBLE) { \
			*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt *(double*)(obj2.content_ptr); \
		} else if(ret.type == FLOAT) { \
			*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt *(float*)(obj2.content_ptr); \
		} \
	} else { \
		int mintype = min(obj1.type, obj2.type); \
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(mintype == U_SHORT || mintype == SHORT) { \
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(mintype == U_CHAR) { \
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} else if(mintype == DOUBLE) { \
			if(obj1.type == DOUBLE) { \
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) { \
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(int*)(obj2.content_ptr)); \
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) { \
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(uint*)(obj2.content_ptr)); \
				} else if(obj2.type == FLOAT) { \
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) opt (double)(*(float*)(obj2.content_ptr)); \
				} \
			} else { \
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) { \
					*(int*)(ret.content_ptr) = (double)(*(int*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) { \
					*(int*)(ret.content_ptr) = (double)(*(uint*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} else if(obj1.type == FLOAT) { \
					*(int*)(ret.content_ptr) = (double)(*(float*)(obj1.content_ptr)) opt *(double*)(obj2.content_ptr); \
				} \
			} \
		} else if(mintype == FLOAT) { \
			if(obj1.type == FLOAT) { \
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) { \
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt (float)(*(int*)(obj2.content_ptr)); \
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) { \
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) opt (float)(*(uint*)(obj2.content_ptr)); \
				} \
			} else { \
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) { \
					*(int*)(ret.content_ptr) = (float)(*(int*)(obj1.content_ptr)) opt *(float*)(obj2.content_ptr); \
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) { \
					*(int*)(ret.content_ptr) = (float)(*(uint*)(obj1.content_ptr)) opt *(float*)(obj2.content_ptr); \
				} \
			} \
		} \
	} \
	return ret

#define OPERATOR_OVERLOAD_MACRO_RET_INT_WITHOUT_FLOAT(opt) \
	if(obj1.type == obj2.type) { \
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(ret.type == U_SHORT || ret.type == SHORT) { \
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(ret.type == U_CHAR || ret.type == CHAR) { \
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} \
	} else { \
		int mintype = min(obj1.type, obj2.type); \
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(mintype == U_SHORT || mintype == SHORT) { \
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(mintype == U_CHAR) { \
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} \
	} \
	return ret

#define OPERATOR_OVERLOAD_MACRO_WITHOUT_FLOAT(opt) \
	if(obj1.type == obj2.type) { \
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(ret.type == U_SHORT || ret.type == SHORT) { \
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(ret.type == U_CHAR || ret.type == CHAR) { \
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} \
	} else { \
		int mintype = min(obj1.type, obj2.type); \
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) { \
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) opt *(int*)(obj2.content_ptr); \
		} else if(mintype == U_SHORT || mintype == SHORT) { \
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) opt *(short*)(obj2.content_ptr); \
		} else if(mintype == U_CHAR) { \
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) opt *(char*)(obj2.content_ptr); \
		} \
	} \
	return ret

varity_info& operator+(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
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

varity_info& operator-(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = min(obj1.type, obj2.type);
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO(-);
}

varity_info& operator*(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = min(obj1.type, obj2.type);
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(obj1.type == obj2.type) {
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) * *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) * *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR || ret.type == CHAR) {
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) * *(char*)(obj2.content_ptr);
		} else if(ret.type = DOUBLE) {
			*(double*)(ret.content_ptr) = *(double*)(obj1.content_ptr) * *(double*)(obj2.content_ptr);
		} else if(ret.type == FLOAT) {
			*(float*)(ret.content_ptr) = *(float*)(obj1.content_ptr) * *(float*)(obj2.content_ptr);
		}
	} else {
		varity_info obja, objb;
		obja = obj1.type==ret.type?obj1:obj2;
		objb = obj1.type==ret.type?obj2:obj1;
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) * *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(short*)(ret.content_ptr) = *(short*)(obj1.content_ptr) * *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR) {
			*(char*)(ret.content_ptr) = *(char*)(obj1.content_ptr) * *(char*)(obj2.content_ptr);
		} else if(ret.type == DOUBLE) {
			if(objb.type == LONG || objb.type == INT || objb.type == SHORT || objb.type == CHAR) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) * (double)(*(int*)(objb.content_ptr));
			} else if(objb.type == U_LONG || objb.type == U_INT || objb.type == U_SHORT || objb.type == U_CHAR) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) * (double)(*(uint*)(objb.content_ptr));
			} else if(objb.type == FLOAT) {
				*(double*)(ret.content_ptr) = *(double*)(obja.content_ptr) * (double)(*(float*)(objb.content_ptr));
			}
		} else if(ret.type == FLOAT) {
			if(objb.type == LONG || objb.type == INT || objb.type == SHORT || objb.type == CHAR) {
				*(float*)(ret.content_ptr) = *(float*)(obja.content_ptr) * (float)(*(int*)(objb.content_ptr));
			} else if(objb.type == U_LONG || objb.type == U_INT || objb.type == U_SHORT || objb.type == U_CHAR) {
				*(float*)(ret.content_ptr) = *(float*)(obja.content_ptr) * (float)(*(uint*)(objb.content_ptr));
			}
		}
	}
	return ret;
}

varity_info& operator/(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = min(obj1.type, obj2.type);
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO(/);
}

varity_info& operator%(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = min(obj1.type, obj2.type);
	if(ret.type == FLOAT || ret.type == DOUBLE) {
		ret.reset();
		error("float cannot be used in mod operator\n");
		return ret;
	}
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO_WITHOUT_FLOAT(%);
	return ret;
}

varity_info& operator>(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(obj1.type == obj2.type) {
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) > *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) > *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR || ret.type == CHAR) {
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) > *(char*)(obj2.content_ptr);
		} else if(ret.type = DOUBLE) {
			*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) > *(double*)(obj2.content_ptr);
		} else if(ret.type == FLOAT) {
			*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) > *(float*)(obj2.content_ptr);
		}
	} else {
		int mintype = min(obj1.type, obj2.type);
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) > *(int*)(obj2.content_ptr);
		} else if(mintype == U_SHORT || mintype == SHORT) {
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) > *(short*)(obj2.content_ptr);
		} else if(mintype == U_CHAR) {
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) > *(char*)(obj2.content_ptr);
		} else if(mintype == DOUBLE) {
			if(obj1.type == DOUBLE) {
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) > (double)(*(int*)(obj2.content_ptr));
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) > (double)(*(uint*)(obj2.content_ptr));
				} else if(obj2.type == FLOAT) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) > (double)(*(float*)(obj2.content_ptr));
				}
			} else {
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) {
					*(int*)(ret.content_ptr) = (double)(*(int*)(obj1.content_ptr)) > *(double*)(obj2.content_ptr);
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) {
					*(int*)(ret.content_ptr) = (double)(*(uint*)(obj1.content_ptr)) > *(double*)(obj2.content_ptr);
				} else if(obj1.type == FLOAT) {
					*(int*)(ret.content_ptr) = (double)(*(float*)(obj1.content_ptr)) > *(double*)(obj2.content_ptr);
				}
			}
		} else if(mintype == FLOAT) {
			if(obj1.type == FLOAT) {
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) {
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) > (float)(*(int*)(obj2.content_ptr));
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) {
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) > (float)(*(uint*)(obj2.content_ptr));
				}
			} else {
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) {
					*(int*)(ret.content_ptr) = (float)(*(int*)(obj1.content_ptr)) > *(float*)(obj2.content_ptr);
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) {
					*(int*)(ret.content_ptr) = (float)(*(uint*)(obj1.content_ptr)) > *(float*)(obj2.content_ptr);
				}
			}
		}
	}
	return ret;
}

varity_info& operator<(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(obj1.type == obj2.type) {
		if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) < *(int*)(obj2.content_ptr);
		} else if(ret.type == U_SHORT || ret.type == SHORT) {
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) < *(short*)(obj2.content_ptr);
		} else if(ret.type == U_CHAR || ret.type == CHAR) {
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) < *(char*)(obj2.content_ptr);
		} else if(ret.type = DOUBLE) {
			*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) < *(double*)(obj2.content_ptr);
		} else if(ret.type == FLOAT) {
			*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) < *(float*)(obj2.content_ptr);
		}
	} else {
		int mintype = min(obj1.type, obj2.type);
		if(mintype == U_LONG || mintype == LONG || mintype == U_INT || mintype == INT) {
			*(int*)(ret.content_ptr) = *(int*)(obj1.content_ptr) < *(int*)(obj2.content_ptr);
		} else if(mintype == U_SHORT || mintype == SHORT) {
			*(int*)(ret.content_ptr) = *(short*)(obj1.content_ptr) < *(short*)(obj2.content_ptr);
		} else if(mintype == U_CHAR) {
			*(int*)(ret.content_ptr) = *(char*)(obj1.content_ptr) < *(char*)(obj2.content_ptr);
		} else if(mintype == DOUBLE) {
			if(obj1.type == DOUBLE) {
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) < (double)(*(int*)(obj2.content_ptr));
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) < (double)(*(uint*)(obj2.content_ptr));
				} else if(obj2.type == FLOAT) {
					*(int*)(ret.content_ptr) = *(double*)(obj1.content_ptr) < (double)(*(float*)(obj2.content_ptr));
				}
			} else {
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) {
					*(int*)(ret.content_ptr) = (double)(*(int*)(obj1.content_ptr)) < *(double*)(obj2.content_ptr);
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) {
					*(int*)(ret.content_ptr) = (double)(*(uint*)(obj1.content_ptr)) < *(double*)(obj2.content_ptr);
				} else if(obj1.type == FLOAT) {
					*(int*)(ret.content_ptr) = (double)(*(float*)(obj1.content_ptr)) < *(double*)(obj2.content_ptr);
				}
			}
		} else if(mintype == FLOAT) {
			if(obj1.type == FLOAT) {
				if(obj2.type == LONG || obj2.type == INT || obj2.type == SHORT || obj2.type == CHAR) {
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) < (float)(*(int*)(obj2.content_ptr));
				} else if(obj2.type == U_LONG || obj2.type == U_INT || obj2.type == U_SHORT || obj2.type == U_CHAR) {
					*(int*)(ret.content_ptr) = *(float*)(obj1.content_ptr) < (float)(*(uint*)(obj2.content_ptr));
				}
			} else {
				if(obj1.type == LONG || obj1.type == INT || obj1.type == SHORT || obj1.type == CHAR) {
					*(int*)(ret.content_ptr) = (float)(*(int*)(obj1.content_ptr)) < *(float*)(obj2.content_ptr);
				} else if(obj1.type == U_LONG || obj1.type == U_INT || obj1.type == U_SHORT || obj1.type == U_CHAR) {
					*(int*)(ret.content_ptr) = (float)(*(uint*)(obj1.content_ptr)) < *(float*)(obj2.content_ptr);
				}
			}
		}
	}
	return ret;
}

varity_info& operator>=(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO_RET_INT(>=);
}

varity_info& operator<=(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO_RET_INT(<=);
}

varity_info& operator==(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO_RET_INT(==);
}

varity_info& operator!=(varity_info& obj1, varity_info& obj2)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	OPERATOR_OVERLOAD_MACRO_RET_INT(!=);
}

varity_info& operator~(varity_info& obj)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;//bit revert return int type.
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
		*(int*)(ret.content_ptr) = ~*(int*)(obj.content_ptr);
	} else if(ret.type == U_SHORT || ret.type == SHORT) {
		*(int*)(ret.content_ptr) = ~*(short*)(obj.content_ptr);
	} else if(ret.type == U_CHAR || ret.type == CHAR) {
		*(int*)(ret.content_ptr) = ~*(char*)(obj.content_ptr);
	}
	return ret;
}

varity_info& operator!(varity_info& obj)
{
	static varity_info ret;
	ret.reset();
	ret.type = INT;//bit revert return int type.
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
		*(int*)(ret.content_ptr) = !*(int*)(obj.content_ptr);
	} else if(ret.type == U_SHORT || ret.type == SHORT) {
		*(int*)(ret.content_ptr) = !*(short*)(obj.content_ptr);
	} else if(ret.type == U_CHAR || ret.type == CHAR) {
		*(int*)(ret.content_ptr) = !*(char*)(obj.content_ptr);
	}
	return ret;
}

varity_info& operator-(varity_info& obj)
{
	static varity_info ret;
	ret.reset();
	ret.type = obj.type;//bit revert return int type.
	ret.size = sizeof_type[ret.type];
	ret.apply_space();
	if(ret.type == U_LONG || ret.type == LONG || ret.type == U_INT || ret.type == INT) {
		INT_VALUE(ret.content_ptr) = -INT_VALUE(obj.content_ptr);
	} else if(ret.type == U_SHORT || ret.type == SHORT) {
		SHORT_VALUE(ret.content_ptr) = -SHORT_VALUE(obj.content_ptr);
	} else if(ret.type == U_CHAR || ret.type == CHAR) {
		CHAR_VALUE(ret.content_ptr) = -CHAR_VALUE(obj.content_ptr);
	} else if(ret.type == FLOAT) {
		FLOAT_VALUE(ret.content_ptr) = -FLOAT_VALUE(ret.content_ptr);
	} else if(ret.type == DOUBLE) {
		DOUBLE_VALUE(ret.content_ptr) = -DOUBLE_VALUE(ret.content_ptr);
	}
	return ret;
}

varity_info& varity_info::operator++(void)
{
	if(this->type == CHAR || this->type == U_CHAR)
		CHAR_VALUE(this->content_ptr)++;
	else if(this->type == SHORT || this->type == U_SHORT)
		SHORT_VALUE(this->content_ptr)++;
	else if(this->type == INT || this->type == U_INT)
		INT_VALUE(this->content_ptr)++;
	return *this;
}

varity_info& varity_info::operator++(int)
{
	static varity_info ret;
	ret.reset();
	ret = *this;
	if(this->type == CHAR || this->type == U_CHAR)
		CHAR_VALUE(this->content_ptr)++;
	else if(this->type == SHORT || this->type == U_SHORT)
		SHORT_VALUE(this->content_ptr)++;
	else if(this->type == INT || this->type == U_INT)
		INT_VALUE(this->content_ptr)++;
	return ret;
}

varity_info& varity_info::operator--(void)
{
	if(this->type == CHAR || this->type == U_CHAR)
		CHAR_VALUE(this->content_ptr)--;
	else if(this->type == SHORT || this->type == U_SHORT)
		SHORT_VALUE(this->content_ptr)--;
	else if(this->type == INT || this->type == U_INT)
		INT_VALUE(this->content_ptr)--;
	return *this;
}

varity_info& varity_info::operator--(int)
{
	static varity_info ret;
	ret.reset();
	ret = *this;
	if(this->type == CHAR || this->type == U_CHAR)
		CHAR_VALUE(this->content_ptr)--;
	else if(this->type == SHORT || this->type == U_SHORT)
		SHORT_VALUE(this->content_ptr)--;
	else if(this->type == INT || this->type == U_INT)
		INT_VALUE(this->content_ptr)--;
	return ret;
}