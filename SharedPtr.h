#pragma once

#include "UniquePtr.h"

class ControlBlockBase {
public:
    explicit ControlBlockBase(int size = 0) noexcept;
    virtual ~ControlBlockBase() = default;

    ControlBlockBase(const ControlBlockBase& other) = delete;
    ControlBlockBase& operator=(const ControlBlockBase& other) = delete;

    void AddRef() noexcept;
    void ReleaseRef() noexcept;
    std::size_t UseCount() const noexcept;
    int GetSize() const noexcept;

protected:
    virtual void DestroyObject() noexcept = 0;

private:
    std::size_t references_;
    int size_;
};

template <class T, class Deleter>
class ControlBlockPtr : public ControlBlockBase {
public:
    ControlBlockPtr(T* ptr, const Deleter& deleter, int size = 0);
    ControlBlockPtr(T* ptr, Deleter&& deleter, int size = 0);

protected:
    void DestroyObject() noexcept override;

private:
    T* ptr_;
    Deleter deleter_;
};

template <class T>
class SharedPtr {
    static_assert(!std::is_array<T>::value, "Use SharedPtr<T[]> for arrays");

    template <class U> friend class SharedPtr;

public:
    SharedPtr() noexcept;
    SharedPtr(std::nullptr_t) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    explicit SharedPtr(U* ptr);

    template <class U, class Deleter, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    SharedPtr(U* ptr, const Deleter& deleter);

    SharedPtr(const SharedPtr<T>& other) noexcept;
    SharedPtr(SharedPtr<T>&& other) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    SharedPtr(const SharedPtr<U>& other) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    SharedPtr(SharedPtr<U>&& other) noexcept;

    template <class U, class Deleter, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        std::is_nothrow_move_constructible<Deleter>::value>>
    SharedPtr(UniquePtr<U, Deleter>&& other);

    SharedPtr<T>& operator=(const SharedPtr<T>& other) noexcept;
    SharedPtr<T>& operator=(SharedPtr<T>&& other) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    SharedPtr<T>& operator=(const SharedPtr<U>& other) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    SharedPtr<T>& operator=(SharedPtr<U>&& other) noexcept;

    ~SharedPtr();

    template <class... Args>
    static SharedPtr<T> Make(Args&&... args);

    T* Get() const noexcept;
    std::size_t UseCount() const noexcept;
    void Reset() noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    void Reset(U* ptr);

    template <class U, class Deleter, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    void Reset(U* ptr, const Deleter& deleter);

    void Swap(SharedPtr<T>& other) noexcept;
    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const noexcept;

private:
    T* ptr_;
    ControlBlockBase* block_;
};

template <class T>
class SharedPtr<T[]> {
public:
    SharedPtr() noexcept;
    SharedPtr(std::nullptr_t) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    SharedPtr(U* data, int count);

    template <class U, class Deleter, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    SharedPtr(U* data, int count, const Deleter& deleter);

    SharedPtr(const SharedPtr<T[]>& other) noexcept;
    SharedPtr(SharedPtr<T[]>&& other) noexcept;

    template <class Deleter, class = std::enable_if_t<std::is_nothrow_move_constructible<Deleter>::value>>
    SharedPtr(UniquePtr<T[], Deleter>&& other);

    SharedPtr<T[]>& operator=(const SharedPtr<T[]>& other) noexcept;
    SharedPtr<T[]>& operator=(SharedPtr<T[]>&& other) noexcept;
    ~SharedPtr();

    static SharedPtr<T[]> MakeArray(int count);

    T* Get() const noexcept;
    int GetSize() const noexcept;
    std::size_t UseCount() const noexcept;
    void Reset() noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    void Reset(U* data, int count);

    template <class U, class Deleter, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    void Reset(U* data, int count, const Deleter& deleter);

    void Swap(SharedPtr<T[]>& other) noexcept;
    T& operator[](int index) const;
    explicit operator bool() const noexcept;

private:
    T* ptr_;
    ControlBlockBase* block_;

    void CheckIndex(int index) const;
    static void CheckArray(const T* data, int count);
};

#include "SharedPtr.tpp"
