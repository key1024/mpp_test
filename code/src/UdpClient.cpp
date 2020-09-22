#include "UdpClient.h"
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <string.h>

UdpClient::UdpClient()
{
    m_UdpSocket = -1;
	m_UdpisOpen = false;
}

UdpClient::~UdpClient()
{
}

void UdpClient::Close()
 {
	 if (m_UdpSocket >= 0)
	 {
		 close(m_UdpSocket);
	 }
 }

bool UdpClient::Open(std::string bindIp, int bindPort)
{
	if (m_UdpisOpen)
		return false;

	m_UdpisOpen = false;

	if (m_UdpSocket)
	{
		close(m_UdpSocket);
	}
	m_UdpSocket= socket(AF_INET, SOCK_DGRAM, 0);
	if ( m_UdpSocket < 0 )
	{
		return false;
	}

    bzero(&m_TargetAddr,sizeof(m_TargetAddr));  
    m_TargetAddr.sin_family = AF_INET;  
    m_TargetAddr.sin_port = htons(bindPort);  
    m_TargetAddr.sin_addr.s_addr = inet_addr(bindIp.c_str());

	m_UdpisOpen = true;
	return true;
}

int UdpClient::Read(uint8_t* pBuffer, uint16_t bufferSize)
{
	int iRead;

	if (!m_UdpisOpen)
		return -1;

	return iRead;
}

int UdpClient::Write(uint8_t* pBuffer, uint16_t bufferSize)
{
	int iWrite;

	if (!m_UdpisOpen)
		return -1;

	iWrite = sendto(m_UdpSocket, pBuffer, bufferSize, 0, (struct sockaddr *)&m_TargetAddr, sizeof(m_TargetAddr));

	return iWrite;
}