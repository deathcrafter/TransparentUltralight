#pragma once
#include <atomic>

template <class T>
class RefCountedImpl
{
public:
    RefCountedImpl()
        : ref_count_(1)
    {
    }

    virtual ~RefCountedImpl() {
    }

    virtual void AddRef() const {
        ++ref_count_;
    }

    virtual void Release() const {
        if (--ref_count_ == 0) {
            delete static_cast<const T*>(this);
        }
    }

    virtual int ref_count() const {
        return ref_count_;
    }

private:
    mutable std::atomic<int> ref_count_;
};

// A macro to disallow the copy constructor and operator= functions.
// This should be used in the private: declarations for a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

#define REF_COUNTED_IMPL(TypeName) \
  virtual void AddRef() const override { RefCountedImpl<TypeName>::AddRef(); } \
  virtual void Release() const override { RefCountedImpl<TypeName>::Release(); } \
  virtual int ref_count() const override { return RefCountedImpl<TypeName>::ref_count(); } \
  friend class RefCountedImpl<TypeName>