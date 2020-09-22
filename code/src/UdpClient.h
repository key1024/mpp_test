#ifndef __UDP_CLIENT_H__
#define __UDP_CLIENT_H__

#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class UdpClient
{
public:

	UdpClient();
	~UdpClient();

	bool Open(std::string bindIp = "", int bindPort = 0);

	int  Read(uint8_t* pBuffer, uint16_t bufferSize);

	int  Write(uint8_t* pBuffer, uint16_t bufferSize);

	void Close();

protected:

    int     m_UdpSocket;
    bool	m_UdpisOpen;
	sockaddr_in m_TargetAddr;
};

#endif //__UDP_CLIENT_H__
