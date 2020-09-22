#include "push_stream.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define PUSH_STREAM_IP "192.168.5.216"
#define PUSH_STREAM_PORT 23003

PushStream::PushStream()
{
    m_udp_client.Open(PUSH_STREAM_IP, PUSH_STREAM_PORT);
}

PushStream::~PushStream()
{
}

int PushStream::PushOneFrame(void* frame, int len)
{
    int send_len = 0;
    while (len > 8192)
    {
        int ret = m_udp_client.Write((uint8_t*)frame+send_len, 8192);
        send_len += ret;
        len -= ret;
    }
    
    m_udp_client.Write((uint8_t*)frame+send_len, len);
    
    return 0;
}