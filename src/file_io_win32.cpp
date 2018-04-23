#ifdef _WIN32

#include "sndfile2k/sndfile2k.hpp"
#include "common.h"
#include "sndfile_error.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>


class SF_FILE_STREAM: public SF_STREAM
{
    unsigned long m_ref = 0;
    HANDLE m_hFile = INVALID_HANDLE_VALUE;

    void close()
    {
        if (m_hFile !=  INVALID_HANDLE_VALUE)
            ::CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }

public:
    SF_FILE_STREAM(const char * filename, SF_FILEMODE mode)
    {
        DWORD dwDesiredAccess;
        DWORD dwCreationDisposition;
        DWORD dwShareMode;

        switch (mode)
        {
        case SFM_READ:
            dwDesiredAccess = GENERIC_READ;
            dwCreationDisposition = OPEN_EXISTING;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        case SFM_WRITE:
            dwDesiredAccess = GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreationDisposition = CREATE_ALWAYS;
            break;

        case SFM_RDWR:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreationDisposition = OPEN_ALWAYS;
            break;

        default:
            throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            break;
        };

        try
        {
            m_hFile = ::CreateFileA(
                filename,
                dwDesiredAccess,
                dwShareMode,
                nullptr,
                dwCreationDisposition,
                0,
                nullptr);

            if (m_hFile == INVALID_HANDLE_VALUE)
            {
                throw sf::sndfile_error(-SFE_BAD_FILE_PTR);
            }
        }
        catch (...)
        {
            close();
            throw;
        }
    }

    SF_FILE_STREAM(const wchar_t *filename, SF_FILEMODE mode)
    {
        DWORD dwDesiredAccess;
        DWORD dwCreationDisposition;
        DWORD dwShareMode;

        switch (mode)
        {
        case SFM_READ:
            dwDesiredAccess = GENERIC_READ;
            dwCreationDisposition = OPEN_EXISTING;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;

        case SFM_WRITE:
            dwDesiredAccess = GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreationDisposition = CREATE_ALWAYS;
            break;

        case SFM_RDWR:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            dwCreationDisposition = OPEN_ALWAYS;
            break;

        default:
            throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            break;
        };

        try
        {
            m_hFile = ::CreateFileW(
                filename,
                dwDesiredAccess,
                dwShareMode,
                nullptr,
                dwCreationDisposition,
                0,
                nullptr);

            if (m_hFile == INVALID_HANDLE_VALUE)
            {
                throw sf::sndfile_error(-SFE_BAD_FILE_PTR);
            }
        }
        catch (...)
        {
            close();
            throw;
        }
    }

    ~SF_FILE_STREAM()
    {
        close();
    }

    // Inherited via SF_STREAM
    unsigned long ref() override
    {
        unsigned long ref_counter = 0;

        ref_counter = ++m_ref;

        return ref_counter;
    }

    void unref() override
    {
        m_ref--;
        if (m_ref == 0)
            delete this;
    }

    sf_count_t seek(sf_count_t offset, int whence) override
    {
        LARGE_INTEGER liDistanceToMove;
        liDistanceToMove.QuadPart = offset;
        LARGE_INTEGER liNewFilePointer = { 0 };

        BOOL fRet = ::SetFilePointerEx(
            m_hFile,
            liDistanceToMove,
            &liNewFilePointer,
            static_cast<DWORD>(whence)
        );

        return fRet == TRUE ? liNewFilePointer.QuadPart : -1;
    }

    sf_count_t get_filelen() override
    {
        LARGE_INTEGER liFileSize = {0};

        BOOL fRet =::GetFileSizeEx(m_hFile, &liFileSize);

        return fRet == TRUE ? liFileSize.QuadPart : -1;
    }

    sf_count_t read(void * ptr, sf_count_t count) override
    {
        if (!ptr || count <= 0)
            return 0;

        DWORD dwNumberOfBytesToRead = static_cast<DWORD>(count);
        DWORD dwNumberOfBytesRead = 0;
        ::ReadFile(
            m_hFile,
            ptr,
            dwNumberOfBytesToRead,
            &dwNumberOfBytesRead,
            nullptr
            );
        
        return static_cast<sf_count_t>(dwNumberOfBytesRead);
    }

    sf_count_t write(const void * ptr, sf_count_t count) override
    {
        if (!ptr || count <= 0)
            return 0;

        DWORD dwNumberOfBytesToWrite = static_cast<DWORD>(count);
        DWORD dwNumberOfBytesWritten = 0;
        ::WriteFile(
            m_hFile,
            ptr,
            dwNumberOfBytesToWrite,
            &dwNumberOfBytesWritten,
            nullptr
        );

        return static_cast<sf_count_t>(dwNumberOfBytesWritten);
    }

    sf_count_t tell() override
    {
        LARGE_INTEGER liDistanceToMove = { 0 };
        LARGE_INTEGER liNewFilePointer = { 0 };


        BOOL fRet = ::SetFilePointerEx(
            m_hFile,
            liDistanceToMove,
            &liNewFilePointer,
            FILE_CURRENT
        );

        return fRet == TRUE ? liNewFilePointer.QuadPart : -1;
    }

    void flush() override
    {
        ::FlushFileBuffers(m_hFile);
    }

    int set_filelen(sf_count_t len) override
    {
        if (len < 0)
            return -1;

        LARGE_INTEGER liDistanceToMove;
        liDistanceToMove.QuadPart = len;
        BOOL fRet = ::SetFilePointerEx(
            m_hFile,
            liDistanceToMove,
            nullptr,
            FILE_BEGIN
        );

        if (fRet == FALSE)
            return -1;
        
        fRet = ::SetEndOfFile(m_hFile);
        
        return fRet == TRUE ? 0 : -1;
    }
};

int psf_open_file_stream(const char * filename, SF_FILEMODE mode, SF_STREAM **stream)
{
    if (!stream)
        return SFE_BAD_VIRTUAL_IO;

    *stream = nullptr;

    SF_FILE_STREAM *s = nullptr;
    try
    {
        s = new SF_FILE_STREAM(filename, mode);

        *stream = static_cast<SF_STREAM *>(s);
        s->ref();

        return SFE_NO_ERROR;
    }
    catch (const sf::sndfile_error &e)
    {
        delete s;
        return e.error();
    }
}

int psf_open_file_stream(const wchar_t * filename, SF_FILEMODE mode, SF_STREAM **stream)
{
    if (!stream)
        return SFE_BAD_VIRTUAL_IO;

    *stream = nullptr;

    SF_FILE_STREAM *s = nullptr;
    try
    {
        s = new SF_FILE_STREAM(filename, mode);
    }
    catch (const sf::sndfile_error &e)
    {
        delete s;
        return e.error();
    }

    *stream = static_cast<SF_STREAM *>(s);
    s->ref();

    return SFE_NO_ERROR;
}

#endif
