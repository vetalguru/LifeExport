#include "FilterCommunicationPort.h"



FilterCommunicationPort::FilterCommunicationPort()
    :m_portHandle(NULL)
    , m_portName(std::wstring())
{
}


FilterCommunicationPort::~FilterCommunicationPort()
{
    if (isConnected())
        disconnect();
}


HRESULT FilterCommunicationPort::connect(const std::wstring& aPortName, const DWORD aOptions, LPCVOID aContext, WORD aSizeOfContext)
{
    if (m_portHandle)
    {
        return E_ABORT;
    }

    m_portName = aPortName;

    HRESULT result = ::FilterConnectCommunicationPort(
        m_portName.c_str(),
        aOptions,
        aContext,
        aSizeOfContext,
        NULL,
        &m_portHandle);
    if (FAILED(result))
    {
        m_portHandle = NULL;
        m_portName.clear();
    }

    return result;
}


bool FilterCommunicationPort::disconnect()
{
    bool result = false;
    if (m_portHandle)
    {
        result = ::CloseHandle(m_portHandle);
        m_portHandle = NULL;
    }

    m_portName.clear();

    return result;
}


// Function sends a message to a kernel-mode minifilter.
HRESULT FilterCommunicationPort::sendMessage(LPVOID aInBuffer, DWORD aInBufferSize,
    LPVOID aOutBuffer, DWORD aOutBufferSize,
    LPDWORD aBytesReturned)
{
    if (!isConnected())
        return E_ABORT;

    return ::FilterSendMessage(m_portHandle, aInBuffer, aInBufferSize, aOutBuffer, aOutBufferSize, aBytesReturned);
}


// Function replies to a message from a kernel-mode minifilter.
HRESULT FilterCommunicationPort::replyMessage(PFILTER_REPLY_HEADER aReplyBuffer, DWORD aReplyBufferSize)
{
    if (!isConnected())
        return E_ABORT;

    return ::FilterReplyMessage(m_portHandle, aReplyBuffer, aReplyBufferSize);
}


// Function gets a message from a kernel-mode minifilter.
HRESULT FilterCommunicationPort::getMessage(PFILTER_MESSAGE_HEADER aMessageBuffer, DWORD aMessageBufferSize, LPOVERLAPPED aOverlapped)
{
    if (!isConnected())
        return E_ABORT;

    return ::FilterGetMessage(m_portHandle, aMessageBuffer, aMessageBufferSize, aOverlapped);
}


const std::wstring FilterCommunicationPort::portName() const
{
    return m_portName;
}


bool FilterCommunicationPort::isConnected() const
{
    return m_portHandle;
}

