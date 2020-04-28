#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
        int x;
        x = storage(4);
        assert(x == 0);
        x = storage(10);
        assert(x == 4);
        x = storage(7);
        assert(x == 10);
        switch (fork())
        {
        case -1:
                perror("fork");
                exit(1);
        case 0:
                x = storage(5);
                assert(x == 7);
                break;
        default:
                wait(0);
                x = storage(0);
                assert(x == 5);
                break;
        }
        return 0;
}
