#include "UniquePtr.h"

template <class T, class Deleter>
UniquePtr<T, Deleter>::UniquePtr() noexcept
    : ptr_(nullptr), deleter_(), canClone_(false) {
    static_assert(std::is_nothrow_default_constructible<Deleter>::value,
                  "Default deleter construction must not throw");
}

template <class T, class Deleter>
UniquePtr<T, Deleter>::UniquePtr(std::nullptr_t) noexcept
    : UniquePtr() {
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T, Deleter>::UniquePtr(U* ptr) noexcept
    : ptr_(ptr), deleter_(),
      canClone_(std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value) {
    static_assert(std::is_nothrow_default_constructible<Deleter>::value,
                  "Default deleter construction must not throw");
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T, Deleter>::UniquePtr(U* ptr, const Deleter& deleter) noexcept
    : ptr_(ptr), deleter_(deleter),
      canClone_(std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value) {
    static_assert(std::is_nothrow_copy_constructible<Deleter>::value,
                  "Deleter copy construction must not throw");
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T, Deleter>::UniquePtr(U* ptr, Deleter&& deleter) noexcept
    : ptr_(ptr), deleter_(std::move(deleter)),
      canClone_(std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value) {
    static_assert(std::is_nothrow_move_constructible<Deleter>::value,
                  "Deleter move construction must not throw");
}

template <class T, class Deleter>
UniquePtr<T, Deleter>::UniquePtr(UniquePtr<T, Deleter>&& other) noexcept
    : ptr_(other.ptr_), deleter_(std::move(other.deleter_)), canClone_(other.canClone_) {
    static_assert(std::is_nothrow_move_constructible<Deleter>::value,
                  "Deleter move construction must not throw");
    other.ptr_ = nullptr;
    other.canClone_ = false;
}

template <class T, class Deleter>
UniquePtr<T, Deleter>& UniquePtr<T, Deleter>::operator=(UniquePtr<T, Deleter>&& other) noexcept {
    if (this != &other) {
        UniquePtr<T, Deleter> incoming(std::move(other));
        Swap(incoming);
    }
    return *this;
}

template <class T, class Deleter>
template <class U, class D, class>
UniquePtr<T, Deleter>::UniquePtr(UniquePtr<U, D>&& other) noexcept
    : ptr_(other.ptr_), deleter_(std::move(other.deleter_)),
      canClone_(other.canClone_ &&
          std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value) {
    other.ptr_ = nullptr;
    other.canClone_ = false;
}

template <class T, class Deleter>
template <class U, class D, class>
UniquePtr<T, Deleter>& UniquePtr<T, Deleter>::operator=(UniquePtr<U, D>&& other) noexcept {
    UniquePtr<T, Deleter> incoming(std::move(other));
    Swap(incoming);
    return *this;
}

template <class T, class Deleter>
UniquePtr<T, Deleter>::~UniquePtr() {
    Reset();
}

template <class T, class Deleter>
template <class... Args>
UniquePtr<T, Deleter> UniquePtr<T, Deleter>::Make(Args&&... args) {
    return UniquePtr<T, Deleter>(new T(std::forward<Args>(args)...));
}

template <class T, class Deleter>
UniquePtr<T, Deleter> UniquePtr<T, Deleter>::Clone() const {
    if constexpr (std::is_nothrow_copy_constructible<Deleter>::value) {
        if (ptr_ == nullptr)
            return UniquePtr<T, Deleter>(static_cast<T*>(nullptr), deleter_);
        if (!canClone_)
            throw std::logic_error("Object cannot be cloned after type conversion");

        if constexpr (std::is_copy_constructible<T>::value) {
            return UniquePtr<T, Deleter>(new T(static_cast<const T&>(*ptr_)), deleter_);
        }
        else {
            throw std::logic_error("Object type cannot be copied");
        }
    }
    else {
        throw std::logic_error("Deleter cannot be copied without exceptions");
    }
}

template <class T, class Deleter>
T* UniquePtr<T, Deleter>::Get() const noexcept {
    return ptr_;
}

template <class T, class Deleter>
Deleter& UniquePtr<T, Deleter>::GetDeleter() noexcept {
    return deleter_;
}

template <class T, class Deleter>
const Deleter& UniquePtr<T, Deleter>::GetDeleter() const noexcept {
    return deleter_;
}

template <class T, class Deleter>
T* UniquePtr<T, Deleter>::Release() noexcept {
    T* result = ptr_;
    ptr_ = nullptr;
    canClone_ = false;
    return result;
}

template <class T, class Deleter>
void UniquePtr<T, Deleter>::Reset() noexcept {
    T* oldPtr = Release();
    if (oldPtr != nullptr) deleter_(oldPtr);
}

template <class T, class Deleter>
void UniquePtr<T, Deleter>::Reset(std::nullptr_t) noexcept {
    Reset();
}

template <class T, class Deleter>
template <class U, class>
void UniquePtr<T, Deleter>::Reset(U* ptr) noexcept {
    if (ptr == ptr_) return;
    T* oldPtr = ptr_;
    ptr_ = ptr;
    canClone_ = std::is_same<std::remove_cv_t<U>, std::remove_cv_t<T>>::value;
    if (oldPtr != nullptr) deleter_(oldPtr);
}

template <class T, class Deleter>
void UniquePtr<T, Deleter>::Swap(UniquePtr<T, Deleter>& other) noexcept {
    static_assert(std::is_nothrow_swappable<Deleter>::value,
                  "Deleter swap must not throw");
    std::swap(ptr_, other.ptr_);
    std::swap(deleter_, other.deleter_);
    std::swap(canClone_, other.canClone_);
}

template <class T, class Deleter>
T& UniquePtr<T, Deleter>::operator*() const {
    if (ptr_ == nullptr)
        throw std::logic_error("UniquePtr is empty");
    return *ptr_;
}

template <class T, class Deleter>
T* UniquePtr<T, Deleter>::operator->() const {
    if (ptr_ == nullptr)
        throw std::logic_error("UniquePtr is empty");
    return ptr_;
}

template <class T, class Deleter>
UniquePtr<T, Deleter>::operator bool() const noexcept {
    return ptr_ != nullptr;
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>::UniquePtr() noexcept
    : ptr_(nullptr), deleter_(), size_(0) {
    static_assert(std::is_nothrow_default_constructible<Deleter>::value,
                  "Default deleter construction must not throw");
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>::UniquePtr(std::nullptr_t) noexcept
    : UniquePtr() {
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T[], Deleter>::UniquePtr(U* data, int count)
    : ptr_(data), deleter_(), size_(count) {
    static_assert(std::is_nothrow_default_constructible<Deleter>::value,
                  "Default deleter construction must not throw");
    CheckArray(data, count);
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T[], Deleter>::UniquePtr(U* data, int count, const Deleter& deleter)
    : ptr_(data), deleter_(deleter), size_(count) {
    static_assert(std::is_nothrow_copy_constructible<Deleter>::value,
                  "Deleter copy construction must not throw");
    CheckArray(data, count);
}

template <class T, class Deleter>
template <class U, class>
UniquePtr<T[], Deleter>::UniquePtr(U* data, int count, Deleter&& deleter)
    : ptr_(data), deleter_(std::move(deleter)), size_(count) {
    static_assert(std::is_nothrow_move_constructible<Deleter>::value,
                  "Deleter move construction must not throw");
    CheckArray(data, count);
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>::UniquePtr(UniquePtr<T[], Deleter>&& other) noexcept
    : ptr_(other.ptr_), deleter_(std::move(other.deleter_)), size_(other.size_) {
    static_assert(std::is_nothrow_move_constructible<Deleter>::value,
                  "Deleter move construction must not throw");
    other.ptr_ = nullptr;
    other.size_ = 0;
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>& UniquePtr<T[], Deleter>::operator=(UniquePtr<T[], Deleter>&& other) noexcept {
    if (this != &other) {
        UniquePtr<T[], Deleter> incoming(std::move(other));
        Swap(incoming);
    }
    return *this;
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>::~UniquePtr() {
    Reset();
}

template <class T, class Deleter>
UniquePtr<T[], Deleter> UniquePtr<T[], Deleter>::MakeArray(int count) {
    if (count < 0)
        throw std::invalid_argument("Count cannot be negative");
    if (count == 0) return UniquePtr<T[], Deleter>();
    return UniquePtr<T[], Deleter>(new T[count](), count);
}

template <class T, class Deleter>
UniquePtr<T[], Deleter> UniquePtr<T[], Deleter>::Clone() const {
    if constexpr (std::is_nothrow_copy_constructible<Deleter>::value) {
        if (ptr_ == nullptr)
            return UniquePtr<T[], Deleter>(static_cast<T*>(nullptr), 0, deleter_);

        if constexpr (std::is_default_constructible<T>::value &&
                      std::is_copy_assignable<T>::value) {
            UniquePtr<T[], Deleter> copy(new T[size_](), size_, deleter_);
            for (int i = 0; i < size_; ++i)
                copy[i] = static_cast<const T&>(ptr_[i]);
            return copy;
        }
        else {
            throw std::logic_error("Array elements cannot be copied");
        }
    }
    else {
        throw std::logic_error("Deleter cannot be copied without exceptions");
    }
}

template <class T, class Deleter>
T* UniquePtr<T[], Deleter>::Get() const noexcept {
    return ptr_;
}

template <class T, class Deleter>
int UniquePtr<T[], Deleter>::GetSize() const noexcept {
    return size_;
}

template <class T, class Deleter>
Deleter& UniquePtr<T[], Deleter>::GetDeleter() noexcept {
    return deleter_;
}

template <class T, class Deleter>
const Deleter& UniquePtr<T[], Deleter>::GetDeleter() const noexcept {
    return deleter_;
}

template <class T, class Deleter>
T* UniquePtr<T[], Deleter>::Release() noexcept {
    T* result = ptr_;
    ptr_ = nullptr;
    size_ = 0;
    return result;
}

template <class T, class Deleter>
void UniquePtr<T[], Deleter>::Reset() noexcept {
    T* oldPtr = Release();
    if (oldPtr != nullptr) deleter_(oldPtr);
}

template <class T, class Deleter>
template <class U, class>
void UniquePtr<T[], Deleter>::Reset(U* data, int count) {
    CheckArray(data, count);
    if (data == ptr_) {
        if (count != size_)
            throw std::invalid_argument("Array size cannot change without a new allocation");
        return;
    }
    T* oldPtr = ptr_;
    ptr_ = data;
    size_ = count;
    if (oldPtr != nullptr) deleter_(oldPtr);
}

template <class T, class Deleter>
void UniquePtr<T[], Deleter>::Swap(UniquePtr<T[], Deleter>& other) noexcept {
    static_assert(std::is_nothrow_swappable<Deleter>::value,
                  "Deleter swap must not throw");
    std::swap(ptr_, other.ptr_);
    std::swap(deleter_, other.deleter_);
    std::swap(size_, other.size_);
}

template <class T, class Deleter>
void UniquePtr<T[], Deleter>::CheckIndex(int index) const {
    if (index < 0 || index >= size_)
        throw std::out_of_range("Index is out of range");
}

template <class T, class Deleter>
void UniquePtr<T[], Deleter>::CheckArray(const T* data, int count) {
    if (count < 0)
        throw std::invalid_argument("Count cannot be negative");
    if (data == nullptr && count > 0)
        throw std::invalid_argument("Data cannot be null");
}

template <class T, class Deleter>
T& UniquePtr<T[], Deleter>::operator[](int index) const {
    CheckIndex(index);
    return ptr_[index];
}

template <class T, class Deleter>
UniquePtr<T[], Deleter>::operator bool() const noexcept {
    return ptr_ != nullptr;
}
