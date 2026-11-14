#include "SharedPtr.h"

inline ControlBlockBase::ControlBlockBase(int size) noexcept
    : references_(1), size_(size) {
}

inline void ControlBlockBase::AddRef() noexcept {
    ++references_;
}

inline void ControlBlockBase::ReleaseRef() noexcept {
    if (--references_ == 0) {
        DestroyObject();
        delete this;
    }
}

inline std::size_t ControlBlockBase::UseCount() const noexcept {
    return references_;
}

inline int ControlBlockBase::GetSize() const noexcept {
    return size_;
}

template <class T, class Deleter>
ControlBlockPtr<T, Deleter>::ControlBlockPtr(
    T* ptr, const Deleter& deleter, int size)
    : ControlBlockBase(size), ptr_(ptr), deleter_(deleter) {
}

template <class T, class Deleter>
ControlBlockPtr<T, Deleter>::ControlBlockPtr(
    T* ptr, Deleter&& deleter, int size)
    : ControlBlockBase(size), ptr_(ptr), deleter_(std::move(deleter)) {
}

template <class T, class Deleter>
void ControlBlockPtr<T, Deleter>::DestroyObject() noexcept {
    deleter_(ptr_);
}

template <class T>
SharedPtr<T>::SharedPtr() noexcept
    : ptr_(nullptr), block_(nullptr) {
}

template <class T>
SharedPtr<T>::SharedPtr(std::nullptr_t) noexcept
    : SharedPtr() {
}

template <class T>
template <class U, class>
SharedPtr<T>::SharedPtr(U* ptr)
    : SharedPtr(ptr, DefaultDelete<U>()) {
}

template <class T>
template <class U, class Deleter, class>
SharedPtr<T>::SharedPtr(U* ptr, const Deleter& deleter)
    : ptr_(ptr), block_(nullptr) {
    if (ptr == nullptr) return;
    try {
        block_ = new ControlBlockPtr<U, Deleter>(ptr, deleter);
    }
    catch (...) {
        deleter(ptr);
        throw;
    }
}

template <class T>
SharedPtr<T>::SharedPtr(const SharedPtr<T>& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    if (block_ != nullptr) block_->AddRef();
}

template <class T>
SharedPtr<T>::SharedPtr(SharedPtr<T>&& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    other.ptr_ = nullptr;
    other.block_ = nullptr;
}

template <class T>
template <class U, class>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    if (block_ != nullptr) block_->AddRef();
}

template <class T>
template <class U, class>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    other.ptr_ = nullptr;
    other.block_ = nullptr;
}

template <class T>
template <class U, class Deleter, class>
SharedPtr<T>::SharedPtr(UniquePtr<U, Deleter>&& other)
    : ptr_(nullptr), block_(nullptr) {
    if (!other) return;
    block_ = new ControlBlockPtr<U, Deleter>(
        other.Get(), std::move(other.GetDeleter()));
    ptr_ = other.Release();
}

template <class T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<T>& other) noexcept {
    if (this != &other) {
        SharedPtr<T> incoming(other);
        Swap(incoming);
    }
    return *this;
}

template <class T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<T>&& other) noexcept {
    if (this != &other) {
        SharedPtr<T> incoming(std::move(other));
        Swap(incoming);
    }
    return *this;
}

template <class T>
template <class U, class>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& other) noexcept {
    SharedPtr<T> incoming(other);
    Swap(incoming);
    return *this;
}

template <class T>
template <class U, class>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<U>&& other) noexcept {
    SharedPtr<T> incoming(std::move(other));
    Swap(incoming);
    return *this;
}

template <class T>
SharedPtr<T>::~SharedPtr() {
    Reset();
}

template <class T>
template <class... Args>
SharedPtr<T> SharedPtr<T>::Make(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
T* SharedPtr<T>::Get() const noexcept {
    return ptr_;
}

template <class T>
std::size_t SharedPtr<T>::UseCount() const noexcept {
    return block_ == nullptr ? 0 : block_->UseCount();
}

template <class T>
void SharedPtr<T>::Reset() noexcept {
    ControlBlockBase* oldBlock = block_;
    ptr_ = nullptr;
    block_ = nullptr;
    if (oldBlock != nullptr) oldBlock->ReleaseRef();
}

template <class T>
template <class U, class>
void SharedPtr<T>::Reset(U* ptr) {
    Reset(ptr, DefaultDelete<U>());
}

template <class T>
template <class U, class Deleter, class>
void SharedPtr<T>::Reset(U* ptr, const Deleter& deleter) {
    if (ptr == ptr_) return;
    SharedPtr<T> incoming(ptr, deleter);
    Swap(incoming);
}

template <class T>
void SharedPtr<T>::Swap(SharedPtr<T>& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(block_, other.block_);
}

template <class T>
T& SharedPtr<T>::operator*() const {
    if (ptr_ == nullptr)
        throw std::logic_error("SharedPtr is empty");
    return *ptr_;
}

template <class T>
T* SharedPtr<T>::operator->() const {
    if (ptr_ == nullptr)
        throw std::logic_error("SharedPtr is empty");
    return ptr_;
}

template <class T>
SharedPtr<T>::operator bool() const noexcept {
    return ptr_ != nullptr;
}

template <class T>
SharedPtr<T[]>::SharedPtr() noexcept
    : ptr_(nullptr), block_(nullptr) {
}

template <class T>
SharedPtr<T[]>::SharedPtr(std::nullptr_t) noexcept
    : SharedPtr() {
}

template <class T>
template <class U, class>
SharedPtr<T[]>::SharedPtr(U* data, int count)
    : SharedPtr(data, count, DefaultDelete<U[]>()) {
}

template <class T>
template <class U, class Deleter, class>
SharedPtr<T[]>::SharedPtr(U* data, int count, const Deleter& deleter)
    : ptr_(data), block_(nullptr) {
    CheckArray(data, count);
    if (data == nullptr) return;
    try {
        block_ = new ControlBlockPtr<U, Deleter>(data, deleter, count);
    }
    catch (...) {
        deleter(data);
        throw;
    }
}

template <class T>
SharedPtr<T[]>::SharedPtr(const SharedPtr<T[]>& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    if (block_ != nullptr) block_->AddRef();
}

template <class T>
SharedPtr<T[]>::SharedPtr(SharedPtr<T[]>&& other) noexcept
    : ptr_(other.ptr_), block_(other.block_) {
    other.ptr_ = nullptr;
    other.block_ = nullptr;
}

template <class T>
template <class Deleter, class>
SharedPtr<T[]>::SharedPtr(UniquePtr<T[], Deleter>&& other)
    : ptr_(nullptr), block_(nullptr) {
    if (!other) return;
    block_ = new ControlBlockPtr<T, Deleter>(
        other.Get(), std::move(other.GetDeleter()), other.GetSize());
    ptr_ = other.Release();
}

template <class T>
SharedPtr<T[]>& SharedPtr<T[]>::operator=(const SharedPtr<T[]>& other) noexcept {
    if (this != &other) {
        SharedPtr<T[]> incoming(other);
        Swap(incoming);
    }
    return *this;
}

template <class T>
SharedPtr<T[]>& SharedPtr<T[]>::operator=(SharedPtr<T[]>&& other) noexcept {
    if (this != &other) {
        SharedPtr<T[]> incoming(std::move(other));
        Swap(incoming);
    }
    return *this;
}

template <class T>
SharedPtr<T[]>::~SharedPtr() {
    Reset();
}

template <class T>
SharedPtr<T[]> SharedPtr<T[]>::MakeArray(int count) {
    if (count < 0)
        throw std::invalid_argument("Count cannot be negative");
    if (count == 0) return SharedPtr<T[]>();
    return SharedPtr<T[]>(new T[count](), count);
}

template <class T>
T* SharedPtr<T[]>::Get() const noexcept {
    return ptr_;
}

template <class T>
int SharedPtr<T[]>::GetSize() const noexcept {
    return block_ == nullptr ? 0 : block_->GetSize();
}

template <class T>
std::size_t SharedPtr<T[]>::UseCount() const noexcept {
    return block_ == nullptr ? 0 : block_->UseCount();
}

template <class T>
void SharedPtr<T[]>::Reset() noexcept {
    ControlBlockBase* oldBlock = block_;
    ptr_ = nullptr;
    block_ = nullptr;
    if (oldBlock != nullptr) oldBlock->ReleaseRef();
}

template <class T>
template <class U, class>
void SharedPtr<T[]>::Reset(U* data, int count) {
    Reset(data, count, DefaultDelete<U[]>());
}

template <class T>
template <class U, class Deleter, class>
void SharedPtr<T[]>::Reset(U* data, int count, const Deleter& deleter) {
    CheckArray(data, count);
    if (data == ptr_) {
        if (count != GetSize())
            throw std::invalid_argument("Array size cannot change without a new allocation");
        return;
    }
    SharedPtr<T[]> incoming(data, count, deleter);
    Swap(incoming);
}

template <class T>
void SharedPtr<T[]>::Swap(SharedPtr<T[]>& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(block_, other.block_);
}

template <class T>
void SharedPtr<T[]>::CheckIndex(int index) const {
    if (index < 0 || index >= GetSize())
        throw std::out_of_range("Index is out of range");
}

template <class T>
void SharedPtr<T[]>::CheckArray(const T* data, int count) {
    if (count < 0)
        throw std::invalid_argument("Count cannot be negative");
    if (data == nullptr && count > 0)
        throw std::invalid_argument("Data cannot be null");
}

template <class T>
T& SharedPtr<T[]>::operator[](int index) const {
    CheckIndex(index);
    return ptr_[index];
}

template <class T>
SharedPtr<T[]>::operator bool() const noexcept {
    return ptr_ != nullptr;
}
