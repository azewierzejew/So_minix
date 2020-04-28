#include <lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <minix/rs.h>

int main(int argc, char** argv)
{
        message m;
        endpoint_t ipc_ep;
        minix_rs_lookup("ipc", &ipc_ep);
        _syscall(ipc_ep, IPC_PRINTMESSAGE, &m);
}
