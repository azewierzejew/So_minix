#include <lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <minix/rs.h>

int get_ipc_endpt(endpoint_t *pt)
{
        return minix_rs_lookup("ipc", pt);
}

int printmessage(void)
{
        endpoint_t ipc_pt;
        message m;
        if (get_ipc_endpt(&ipc_pt) != 0)
        {
                errno = ENOSYS;
                return -1;
        }
        return (_syscall(ipc_pt, IPC_PRINTMESSAGE, &m));
}
