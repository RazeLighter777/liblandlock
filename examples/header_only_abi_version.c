#define LIBLANDLOCK_IMPLEMENTATION
#include "../dist/liblandlock.h"

#include <stdio.h>

int main(void)
{
    ll_abi_t abi = 0;
    const ll_error_t err = ll_get_abi_version(&abi);

    if (LL_ERRORED(err))
    {
        fprintf(stderr, "ll_get_abi_version failed: %s (%d)\n", ll_error_string(err), err);
        return 1;
    }

    printf("Landlock ABI version: %d\n", abi);
    return 0;
}
