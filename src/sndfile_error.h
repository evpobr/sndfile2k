#pragma once

#include <exception>

namespace sf
{

class sndfile_error : std::runtime_error
{
    int m_error = 0;

public:
    sndfile_error(int error)
        : runtime_error(sf_error_number(error))
    {
        m_error = error;
    }

    int error() const
    {
        return m_error;
    }
};

}
