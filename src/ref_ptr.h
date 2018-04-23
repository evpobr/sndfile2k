#pragma once

#include <sndfile2k/sndfile2k.h>

#include <cassert>

namespace sf
{

template <typename iface>
class sw_object_hide_ref : public iface
{
    unsigned long ref();
    void unref();
};

template <typename iface>
class ref_ptr
{
public:
    template <typename T>
    friend class ref_ptr;

    ref_ptr() noexcept = default;

    ref_ptr(ref_ptr const & other) noexcept
        : m_ptr(other.m_ptr)
    {
        internal_ref();
    }

    template <typename T>
    ref_ptr(ref_ptr<T> const & other) noexcept
        : m_ptr(other.m_ptr)
    {
        internal_ref();
    }

    template <typename T>
    ref_ptr(ref_ptr<T> && other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    ~ref_ptr() noexcept
    {
        internal_unref();
    }

    iface * operator->() const noexcept
    {
        return m_ptr;
    }

    ref_ptr & operator=(ref_ptr const &other) noexcept
    {
        internal_copy(other.m_ptr);
        return *this;
    }

    template <typename T>
    ref_ptr & operator=(ref_ptr<T> const & other) noexcept
    {
        internal_copy(other.m_ptr);
        return *this;
    }

    template <typename T>
    ref_ptr & operator=(ref_ptr<T> && other) noexcept
    {
        internal_move(other);
        return *this;
    }

    template <typename T>
    void swap(ref_ptr<T> &left,
              ref_ptr<T> &right) noexcept
    {
        left.internal_swap(right);
    }

    explicit operator bool() const noexcept
    {
        return nullptr != m_ptr;
    }

    void reset() noexcept
    {
        internal_unref();
    }

    iface * get() const noexcept
    {
        return m_ptr;
    }

    iface * detach() noexcept
    {
        iface *temp = m_ptr;
        m_ptr = nullptr;
        return temp;
    }

    void copy(iface *other) noexcept
    {
        internal_copy(other);
    }

    void attach(iface *other) noexcept
    {
        internal_unref();
        m_ptr = other;
    }

    iface ** get_address_of() noexcept
    {
        assert(m_ptr == nullptr);
        return &m_ptr;
    }

    void copy_to(iface ** other) const noexcept
    {
        internal_ref();
        *other = m_ptr;
    }

private:
    iface * m_ptr = nullptr;

    void internal_ref() const noexcept
    {
        if (m_ptr)
        {
            m_ptr->ref();
        }
    }

    void internal_unref() noexcept
    {
        iface * temp = m_ptr;
        if (temp)
        {
            m_ptr = nullptr;
            temp->unref();
        }
    }

    void internal_copy(iface * other) noexcept
    {
        if (m_ptr != other)
        {
            internal_unref();
            m_ptr = other;
            internal_ref();
        }
    }

    template <typename T>
    void internal_move(ref_ptr<T> & other) noexcept
    {
        if (m_ptr != other.m_ptr)
        {
            internal_unref();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
    }

    void internal_swap(ref_ptr & other) noexcept
    {
        iface * temp = m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = temp;
    }
};

}
