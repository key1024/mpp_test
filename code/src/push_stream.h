#ifndef _PUSH_STREAM_H_
#define _PUSH_STREAM_H_

#include "UdpClient.h"

class PushStream
{
private:

public:
    PushStream();
    ~PushStream();

    int PushOneFrame(void* frame, int len);

private:
    UdpClient m_udp_client;
};

#endif // end of _PUSH_STREAM_H_