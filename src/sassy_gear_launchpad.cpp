#include "sassy.h"

unsigned char launchpad_color(double r, double g)
{
    int rc = (int)(r * 3);
    int gc = (int)(g * 3);
    int flags = 12;
    return (unsigned char)(gc * 16 + rc + flags);
}

void launchpad_update()
{
    // do framebuffer update stuff

    if (gMidiCredits < 1)
    {
        return;
    }
    gMidiCredits--;

    // Update random button.
    // Let's call top row 0.
    int x = rand() % 9;
    int y = rand() % 9;
    unsigned char index = (unsigned char)((y - 1) * 16 + x);
    if (y == 0)
        index = (unsigned char)(104 + x);

    unsigned char data[3];
    data[0] = y == 0 ? 0xb0 : 0x90;
    data[1] = index;
    data[2] = launchpad_color((rand() % 256) / 256.0, (rand() % 256) / 256.0);
    gRtmidi_out->sendMessage(data, 3);
}
