#pragma once

#include <stdlib.h>
#include "slist.h"
#include "types.h"

#define bool _Bool
#define true 1
#define false 0

int parse_API_root (json_value const *root_obj, SListInfo *out);
