#include "inc.h"

int do_storage(message *m)
{
		static int val = 0;
		int old = val;
		val = m->m1_i1;
		printf("Syscall storage called: old = %d, new = %d\r\n", old, val);
		m->m1_i1 = old;
        return OK;
}

