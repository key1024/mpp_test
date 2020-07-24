#include "hi_comm_sys.h"
#include "mpi_sys.h"

int main(void)
{
    MPP_VERSION_S version;
    HI_S32 ret = HI_MPI_SYS_GetVersion(&version);
    printf("mpp version: %s\n", version.aVersion);

    return 0;
}