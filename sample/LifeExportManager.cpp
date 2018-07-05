#include "LifeExportManager.h"
#include "LifeExportDriverManager.h"


#ifndef NDEBUG
#include <iostream>
#endif // !NDEBUG


namespace LifeExportManagement
{
    LifeExportManager::LifeExportManager()
        : m_driverManager(nullptr)
    {
    }


    LifeExportManager::~LifeExportManager()
    {
        delete m_driverManager;
        m_driverManager = nullptr;
    }


    const HRESULT LifeExportManager::exec()
    {
        m_driverManager = new (std::nothrow) LifeExportDriverManagement::LifeExportDriverManager(reinterpret_cast<IDriverHandler*>(this));
        if (m_driverManager == nullptr)
        {
            return E_INVALIDARG;
        }

        HRESULT result = m_driverManager->exec();
        if (FAILED(result))
        {
            delete m_driverManager;
            m_driverManager = nullptr;

            return result;
        }

        return result;
    }

    const HRESULT LifeExportManager::stop() const
    {
        if (m_driverManager == nullptr)
        {
            return E_INVALIDARG;
        }

        return m_driverManager->stop();
    }


    HRESULT LifeExportManager::CreatingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
        LIFE_EXPORT_CONNECTION_RESULT& aResult)
    {
        if (aFileIdInfo == NULL)
        {
            return E_INVALIDARG;
        }

        aResult = CREATE_RESULT_SKIP_FILE;

#ifndef NDEBUG
        //std::wcout << L"CREATE File id: " << std::hex << aFileIdInfo->FileId_64.Value << std::endl;

        ULONGLONG testFileId = 0x0003000000009BD9;
        if (aFileIdInfo->FileId_64.Value == testFileId)
        {
            aResult = CREATE_RESULT_EXPORT_FILE;
            //std::wcout << L"THIS IS THE TEST FILE" << std::endl;
        }
#endif // !NDEBUG


        return S_OK;
    }


    HRESULT LifeExportManager::PreReadingLifeTrackingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
        const PULONGLONG aBlockFileOffset,
        const PULONGLONG aBlockLength,
        IDriverHandler::LIFE_EXPORT_USER_BUFFER &aUserBuffer)
    {
        if (aFileIdInfo == NULL)
        {
            return E_INVALIDARG;
        }

        if (aBlockFileOffset == NULL)
        {
            return E_INVALIDARG;
        }

        if (aBlockLength == NULL)
        {
            return E_INVALIDARG;
        }

#ifndef NDEBUG
        std::wcout << L"PRE READ File id: " << std::hex << aFileIdInfo->FileId_64.Value;
        std::wcout << L" file offset: " << std::dec << *aBlockFileOffset;
        std::wcout << L" block size: " << std::dec << *aBlockLength << std::endl;
#endif // !NDEBUG

        // TODO: Need download part of file HERE if need

        // ONLY FOR TEST======================
        char *buffer = new (std::nothrow) char[*aBlockLength];
        if (buffer)
        {
            std::wcout << L"user buffer pointer: " << std::hex << (void*)buffer;
            std::wcout << L" user buffer size: " << std::dec << *aBlockLength << std::endl;

            memset(buffer, 'c', *aBlockLength);

            std::string buff(buffer, *aBlockLength);

            std::cout << "BUFFER: " << buff.c_str() << std::endl;

            aUserBuffer.BufferPtr = buffer;
            aUserBuffer.BufferSize = (ULONG)*aBlockLength;
        }

        //====================================

        return S_OK;
    }

    HRESULT LifeExportManager::PostReadingLifeTrackingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
        const PULONGLONG aBlockFileOffset,
        const PULONGLONG aBlockLength,
        IDriverHandler::LIFE_EXPORT_USER_BUFFER &aUserBuffer)
    {
        if (aFileIdInfo == NULL)
        {
            return E_INVALIDARG;
        }

        if (aBlockFileOffset == NULL)
        {
            return E_INVALIDARG;
        }

        if (aBlockLength == NULL)
        {
            return E_INVALIDARG;
        }

#ifndef NDEBUG
        std::wcout << L"POST READ File id: " << std::hex << aFileIdInfo->FileId_64.Value;
        std::wcout << L" file offset: " << *aBlockFileOffset;
        std::wcout << L" block size: " << *aBlockLength << std::endl;
#endif  // !NDEBUG

        // TODO: Need to send buffer with changes HERE if need

        // ONLY FOR TEST======================
        if (aUserBuffer.BufferPtr)
        {
            std::wcout << L"POST user buffer pointer: " << std::hex << aUserBuffer.BufferPtr;
            std::wcout << L" user buffer size: " << std::dec << aUserBuffer.BufferSize << std::endl;

            std::string str((char*)aUserBuffer.BufferPtr, aUserBuffer.BufferSize);

            std::cout << "Buffer: " << str.c_str() << std::endl;

            delete[] aUserBuffer.BufferPtr;

            aUserBuffer.BufferPtr = nullptr;
            aUserBuffer.BufferSize = 0;
        }

        //====================================

        return S_OK;
    }

    void LifeExportManager::DriverUnloadingCallback()
    {
#ifndef NDEBUG
        std::wcout << L"DriverUnloadingCallback called" << std::endl;
#endif // !NDEBUG
    }


}; // namespace LifeExportManagement

