#include <lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <minix/rs.h>

static int get_ipc_endpt(endpoint_t *pt)
{
        return minix_rs_lookup("ipc", pt);
}

int storage(int new)
{
        endpoint_t ipc_pt;
        message m;
        if (get_ipc_endpt(&ipc_pt) != 0)
        {
                errno = ENOSYS;
                return -1;
        }
        m.m1_i1 = new;
        _syscall(ipc_pt, IPC_STORAGE, &m);
		return m.m1_i1;
}
