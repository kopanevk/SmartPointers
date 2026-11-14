#pragma once

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <class T>
struct DefaultDelete {
    DefaultDelete() noexcept = default;

    // Базовый тип должен корректно удалять объект производного класса.
    template <class U, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        (std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value ||
         std::has_virtual_destructor<T>::value)>>
    DefaultDelete(const DefaultDelete<U>&) noexcept {}

    void operator()(T* ptr) const noexcept {
        delete ptr;
    }
};

template <class T>
struct DefaultDelete<T[]> {
    DefaultDelete() noexcept = default;

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    DefaultDelete(const DefaultDelete<U[]>&) noexcept {}

    void operator()(T* ptr) const noexcept {
        delete[] ptr;
    }
};

template <class T, class Deleter = DefaultDelete<T>>
class UniquePtr {
    static_assert(!std::is_array<T>::value, "Use UniquePtr<T[]> for arrays");

    template <class U, class D> friend class UniquePtr;

public:
    UniquePtr() noexcept;
    UniquePtr(std::nullptr_t) noexcept;

    template <class U, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        (!std::is_same<Deleter, DefaultDelete<T>>::value ||
         std::is_constructible<DefaultDelete<T>, DefaultDelete<U>>::value)>>
    explicit UniquePtr(U* ptr) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    UniquePtr(U* ptr, const Deleter& deleter) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    UniquePtr(U* ptr, Deleter&& deleter) noexcept;

    UniquePtr(const UniquePtr<T, Deleter>& other) = delete;
    UniquePtr<T, Deleter>& operator=(const UniquePtr<T, Deleter>& other) = delete;

    UniquePtr(UniquePtr<T, Deleter>&& other) noexcept;
    UniquePtr<T, Deleter>& operator=(UniquePtr<T, Deleter>&& other) noexcept;

    template <class U, class D, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        std::is_nothrow_constructible<Deleter, D&&>::value>>
    UniquePtr(UniquePtr<U, D>&& other) noexcept;

    template <class U, class D, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        std::is_nothrow_constructible<Deleter, D&&>::value &&
        std::is_nothrow_swappable<Deleter>::value>>
    UniquePtr<T, Deleter>& operator=(UniquePtr<U, D>&& other) noexcept;

    ~UniquePtr();

    template <class... Args>
    static UniquePtr<T, Deleter> Make(Args&&... args);

    UniquePtr<T, Deleter> Clone() const;
    T* Get() const noexcept;
    Deleter& GetDeleter() noexcept;
    const Deleter& GetDeleter() const noexcept;
    T* Release() noexcept;
    void Reset() noexcept;
    void Reset(std::nullptr_t) noexcept;

    template <class U, class = std::enable_if_t<
        std::is_convertible<U*, T*>::value &&
        (!std::is_same<Deleter, DefaultDelete<T>>::value ||
         std::is_constructible<DefaultDelete<T>, DefaultDelete<U>>::value)>>
    void Reset(U* ptr) noexcept;
    void Swap(UniquePtr<T, Deleter>& other) noexcept;

    T& operator*() const;
    T* operator->() const;
    explicit operator bool() const noexcept;

private:
    T* ptr_;
    Deleter deleter_;
    // После преобразования к базовому типу Clone не должен срезать объект.
    bool canClone_;
};

template <class T, class Deleter>
class UniquePtr<T[], Deleter> {
public:
    UniquePtr() noexcept;
    UniquePtr(std::nullptr_t) noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    UniquePtr(U* data, int count);

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    UniquePtr(U* data, int count, const Deleter& deleter);

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    UniquePtr(U* data, int count, Deleter&& deleter);

    UniquePtr(const UniquePtr<T[], Deleter>& other) = delete;
    UniquePtr<T[], Deleter>& operator=(const UniquePtr<T[], Deleter>& other) = delete;

    UniquePtr(UniquePtr<T[], Deleter>&& other) noexcept;
    UniquePtr<T[], Deleter>& operator=(UniquePtr<T[], Deleter>&& other) noexcept;
    ~UniquePtr();

    static UniquePtr<T[], Deleter> MakeArray(int count);

    UniquePtr<T[], Deleter> Clone() const;
    T* Get() const noexcept;
    int GetSize() const noexcept;
    Deleter& GetDeleter() noexcept;
    const Deleter& GetDeleter() const noexcept;
    T* Release() noexcept;
    void Reset() noexcept;

    template <class U, class = std::enable_if_t<std::is_convertible<U(*)[], T(*)[]>::value>>
    void Reset(U* data, int count);

    void Swap(UniquePtr<T[], Deleter>& other) noexcept;
    T& operator[](int index) const;
    explicit operator bool() const noexcept;

private:
    T* ptr_;
    Deleter deleter_;
    int size_;

    void CheckIndex(int index) const;
    static void CheckArray(const T* data, int count);
};

#include "UniquePtr.tpp"
