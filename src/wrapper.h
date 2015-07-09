#ifndef WRAPPER_H
#define WRAPPER_H
#include <QSharedPointer>

template<typename T>
class Wrapper
{
private:
    QSharedPointer<T> ptr;
public:
    Wrapper(QSharedPointer<T> p) : ptr(p) {}
    Wrapper() : ptr(QSharedPointer<T>(nullptr)) {}

    bool isNull() const
    {
        return ptr.isNull();
    }

    T& ref() const
    {
        return *ptr.data();
    }

    operator T&() const
    {
        return *ptr.data();
    }

    Wrapper<T>& operator =(const Wrapper<T> &other)
    {
        ptr = other.ptr;
        return *this;
    }
    Wrapper<T> copy() const
    {
        return Wrapper<T>(ptr);
    }
};


static inline bool valid()
{
    return true;
}

template<typename T, typename ...Args>
static inline bool valid(Wrapper<T> &&w, Wrapper<Args>&&... args)
{
    return !w.isNull() && valid(std::forward<Wrapper<Args> >(args)...);
}


template<typename Func, typename ...Args>
static inline void call_with_ref(Func &&f, Wrapper<Args>&&... w)
{
    if(valid(std::forward<Wrapper<Args> >(w)...))
    {
        f(std::forward<Wrapper<Args> >(w)...);
    }
}

template<typename KT, typename KF, typename ...Args>
static inline void call_with_ref2(KT &&kt, KF &&kf, Wrapper<Args>&&... w)
{
    if(valid(std::forward<Wrapper<Args> >(w)...))
    {
        kt(std::forward<Wrapper<Args> >(w)...);
    }
    else
    {
        kf();
    }
}

template<typename Func, typename Ret, typename ...Args>
static inline Ret call_with_def(Func &&f, Ret &&ret, Wrapper<Args>&&... w)
{
    if(valid(std::forward<Wrapper<Args> >(w)...))
    {
        return f(std::forward<Wrapper<Args> >(w)...);
    }
    else
    {
        return ret;
    }
}

#endif // WRAPPER_H

