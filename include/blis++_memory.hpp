#ifndef _BLISPP_MEMORY_HPP_
#define _BLISPP_MEMORY_HPP_

#include <stdexcept>
#include "blis++.hpp"

#if BLISPP_HAVE_MEMKIND
#include "memkind.h"
#endif

namespace blis
{

template <typename T, typename Allocator=std::allocator<T>>
class Memory : private Allocator
{
    public:
        typedef T type;

    private:
        type* _ptr = nullptr;
        siz_t _size = 0;

        typedef std::allocator_traits<Allocator> _traits;

    public:
        Memory(const Memory&) = delete;

        Memory(Memory&& other)
        : _ptr(other._ptr), _size(other._size)
        {
            other._ptr = nullptr;
        }

        Memory& operator=(const Memory&) = delete;

        Memory& operator=(Memory&& other)
        {
            std::swap(_ptr, other._ptr);
            std::swap(_size, other._size);
            return *this;
        }

        explicit Memory(siz_t size, Allocator alloc = Allocator())
        : Allocator(alloc)
        {
            reset(size);
        }

        explicit Memory(Allocator alloc = Allocator())
        : Allocator(alloc) {}

        ~Memory()
        {
            reset();
        }

        type* reset(siz_t size = 0)
        {
            if (_ptr) _traits::deallocate(*this, _ptr, _size);
            _ptr = nullptr;
            _size = 0;

            if (size > 0)
            {
                _ptr = _traits::allocate(*this, size);
                _size = size;
            }

            return _ptr;
        }

        operator type*() { return _ptr; }

        operator const type*() const { return _ptr; }
};

template <typename T>
class PooledMemory : private mem_t
{
    public:
        typedef T type;

    private:
        packbuf_t _packbuf;

        void init()
        {
            memset(static_cast<mem_t*>(this), 0, sizeof(mem_t));
        }

    public:
        PooledMemory(const PooledMemory&) = delete;

        PooledMemory(PooledMemory&& other)
        {
            *this = std::move(other);
        }

        PooledMemory& operator=(const PooledMemory&) = delete;

        PooledMemory& operator=(PooledMemory&& other)
        {
            if (this != &other)
            {
                memcpy(static_cast<mem_t*>(this), static_cast<mem_t*>(other), sizeof(mem_t));
                _packbuf = other._packbuf;
                other.init();
            }
            return *this;
        }

        explicit PooledMemory(packbuf_t packbuf) : _packbuf(packbuf)
        {
            init();
        }

        explicit PooledMemory(siz_t size, packbuf_t packbuf) : _packbuf(packbuf)
        {
            init();
            reset(size);
        }

        ~PooledMemory()
        {
            free();
        }

        void reset() {}

        void reset(siz_t size)
        {
            if (bli_mem_is_unalloc(this))
            {
                bli_mem_acquire_m(size, _packbuf, this);
            }
            else if (size > bli_mem_size(this))
            {
                bli_mem_release(this);
                bli_mem_acquire_m(size, _packbuf, this);
            }
        }

        void free()
        {
            if (bli_mem_is_alloc(this)) bli_mem_release(this);
        }

        operator type*() { return (type*)bli_mem_buffer(this); }

        operator const type*() const { return (type*)bli_mem_buffer(this); }
};

template <typename T, size_t Alignment=BLIS_HEAP_ADDR_ALIGN_SIZE>
class AlignedAllocator
{
    public:
        typename T value_type;

        template <typename U, size_t UAlignment>
        AlignedAllocator(AlignedAllocator<U,UAlignment> other) {}

        T* allocate(size_t n) const
        {
            T* ptr;
            int ret = posix_memalign(&ptr, Alignment, n*sizeof(T));
            if (ret != 0) throw std::bad_alloc();
            return ptr;
        }

        void deallocate(const T* ptr, size_t n) const
        {
            free(ptr);
        }

        bool operator==(const AlignedAllocator& other) const { return true; }

        bool operator!=(const AlignedAllocator& other) const { return false; }
};

#if BLISPP_HAVE_MEMKIND

enum MemkindType
{
    MEMKIND_DDR_4K,
    MEMKIND_HBM_4K,
    MEMKIND_DDR_2M,
    MEMKIND_HBM_2M,
    MEMKIND_DDR_1G,
    MEMKIND_HBM_1G,
    MEMKIND_DDR = MEMKIND_DDR_4K,
    MEMKIND_HBM = MEMKIND_HBM_4K
};

template <typename T, MemkindType Type=MEMKIND_DDR_4K, size_t Alignment=BLIS_HEAP_ADDR_ALIGN_SIZE>
class MemkindAllocator
{
    public:
        typename T value_type;

        template <typename U, MemkindType UType, size_t UAlignment>
        MemkindAllocator(MemkindAllocator<U,UType,UAlignment> other) {}

        T* allocate(size_t n) const
        {
            T* ptr;
            int ret;

            switch (Type)
            {
                case MEMKIND_DDR_4K: ret = memkind_posix_memalign(MEMKIND_DEFAULT,     &ptr, Alignment, n*sizeof(T)); break;
                case MEMKIND_DDR_2M: ret = memkind_posix_memalign(MEMKIND_HUGETLB,     &ptr, Alignment, n*sizeof(T)); break;
                case MEMKIND_DDR_1G: ret = memkind_posix_memalign(MEMKIND_GBTLB,       &ptr, Alignment, n*sizeof(T)); break;
                case MEMKIND_HBM_4K: ret = memkind_posix_memalign(MEMKIND_HBW,         &ptr, Alignment, n*sizeof(T)); break;
                case MEMKIND_HBM_2M: ret = memkind_posix_memalign(MEMKIND_HBW_HUGETLB, &ptr, Alignment, n*sizeof(T)); break;
                case MEMKIND_HBM_1G: ret = memkind_posix_memalign(MEMKIND_HBW_GBTLB,   &ptr, Alignment, n*sizeof(T)); break;
            }

            if (ret != 0) throw std::bad_alloc();

            return ptr;
        }

        void deallocate(const T* ptr, size_t n) const
        {
            switch (Type)
            {
                case MEMKIND_DDR_4K: memkind_free(MEMKIND_DEFAULT,     ptr); break;
                case MEMKIND_DDR_2M: memkind_free(MEMKIND_HUGETLB,     ptr); break;
                case MEMKIND_DDR_1G: memkind_free(MEMKIND_GBTLB,       ptr); break;
                case MEMKIND_HBM_4K: memkind_free(MEMKIND_HBW,         ptr); break;
                case MEMKIND_HBM_2M: memkind_free(MEMKIND_HBW_HUGETLB, ptr); break;
                case MEMKIND_HBM_1G: memkind_free(MEMKIND_HBW_GBTLB,   ptr); break;
            }
        }

        bool operator==(const MemkindAllocator& other) const { return true; }

        bool operator!=(const MemkindAllocator& other) const { return false; }
};

#endif

}

#endif
