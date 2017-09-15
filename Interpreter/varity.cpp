#include "varity.h"
#include "data_struct.h"

varity_info varity_node[MAX_VARITY_NODE];
stack varity_list(sizeof(varity_info), varity_node);
