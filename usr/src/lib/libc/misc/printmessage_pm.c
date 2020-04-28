#include <lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <minix/rs.h>

static int get_ipc_endpt(endpoint_t *pt)
{
        return minix_rs_lookup("pm", pt);
}

int printmessage_pm(void)
{
        endpoint_t ipc_pt;
        message m;
        if (get_ipc_endpt(&ipc_pt) != 0)
        {
                errno = ENOSYS;
                return -1;
        }
        return (_syscall(ipc_pt, PM_PRINTMESSAGE, &m));
}
