
/*
 * Copyright (C) Jacob Budin
 */

#include <stdlib.h>
#include "pituner.h"

void
ptn_end(int sig)
{
	ptn_stop_station();
	ptn_free();
	exit(EXIT_SUCCESS);
}
