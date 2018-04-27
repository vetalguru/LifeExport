#ifndef _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_
#define _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_


#include <Windows.h>
#include <string>
#include <FltUser.h>


class FilterCommunicationPort
{
	public:
		FilterCommunicationPort();
		~FilterCommunicationPort();

		HRESULT connect(const std::wstring& aPortName, const DWORD aOptions, LPCVOID aContext, WORD aSizeOfContext);
		bool disconnect();

		HRESULT sendMessage(LPVOID aInBuffer, DWORD aInBufferSize,
			LPVOID aOutBuffer, DWORD aOutBufferSize,
			LPDWORD aBytesReturned);

		HRESULT replyMessage(PFILTER_REPLY_HEADER aReplyBuffer, DWORD aReplyBufferSize);

		HRESULT getMessage(PFILTER_MESSAGE_HEADER aMessageBuffer, DWORD aMessageBufferSize, LPOVERLAPPED aOverlapped);

		const std::wstring portName() const;

		bool isConnected() const;

	private:
		FilterCommunicationPort(const FilterCommunicationPort&) = delete;
		FilterCommunicationPort& operator = (const FilterCommunicationPort&) = delete;

	private:
		HANDLE       m_portHandle;
		std::wstring m_portName;
};


#endif // _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_
