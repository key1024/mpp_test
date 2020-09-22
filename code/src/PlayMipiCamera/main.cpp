#include <unistd.h>
#include "play_mipi_camera.h"

int main(void)
{
    PlayMipiCamera play_mipi_camera;
    play_mipi_camera.Init();

    while (getchar() != 'q')
    {
        sleep(1);
    }

    play_mipi_camera.UnInit();

    return 0;
}