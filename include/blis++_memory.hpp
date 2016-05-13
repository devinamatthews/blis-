#ifndef _BLISPP_MEMORY_HPP_
#define _BLISPP_MEMORY_HPP_

#include <stdexcept>
#include <memory>

#include "blis/blis.h"

#if BLISPP_HAVE_MEMKIND
#include "memkind.h"
#endif

#if BLISPP_HAVE_LIBHUGETLBFS
extern "C"
{
#include "hugetlbfs.h"
}
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
            if (_ptr) this->deallocate(_ptr, _size);
            _ptr = nullptr;
            _size = 0;

            if (size > 0)
            {
                _ptr = this->allocate(size);
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

enum MemoryType
{
    MEMORY_DDR_4K,
    MEMORY_HBM_4K,
    MEMORY_DDR_2M,
    MEMORY_HBM_2M,
    MEMORY_DDR_1G,
    MEMORY_HBM_1G,
    MEMORY_DDR = MEMORY_DDR_4K,
    MEMORY_HBM = MEMORY_HBM_4K
};

template <typename T, MemoryType Type=MEMORY_DDR_4K, size_t Alignment=BLIS_HEAP_ADDR_ALIGN_SIZE>
class AlignedAllocator
{
    protected:
        template <typename U>
        static U align(U len, size_t alignment)
        {
            return len + (alignment-1) - (len+alignment-1)%alignment;
        }

    public:
        typedef T value_type;

        AlignedAllocator() {}

        template <typename U, MemoryType UType, size_t UAlignment>
        AlignedAllocator(AlignedAllocator<U,UType,UAlignment> other) {}

        T* allocate(size_t n) const;

        void deallocate(T* ptr, size_t n) const;

        bool operator==(const AlignedAllocator& other) const { return true; }

        bool operator!=(const AlignedAllocator& other) const { return false; }
};

#if BLISPP_HAVE_MEMKIND

template <typename T, MemoryType Type, size_t Alignment>
T* AlignedAllocator<T,Type,Alignment>::allocate(size_t n) const
{
    T* ptr;
    int ret;

    switch (Type)
    {
        case MEMORY_DDR_4K: ret = memkind_posix_memalign(MEMKIND_DEFAULT,     (void**)&ptr, Alignment, n*sizeof(T)); break;
        case MEMORY_DDR_2M: ret = memkind_posix_memalign(MEMKIND_HUGETLB,     (void**)&ptr, Alignment, n*sizeof(T)); break;
        case MEMORY_DDR_1G: ret = memkind_posix_memalign(MEMKIND_GBTLB,       (void**)&ptr, Alignment, n*sizeof(T)); break;
        case MEMORY_HBM_4K: ret = memkind_posix_memalign(MEMKIND_HBW,         (void**)&ptr, Alignment, n*sizeof(T)); break;
        case MEMORY_HBM_2M: ret = memkind_posix_memalign(MEMKIND_HBW_HUGETLB, (void**)&ptr, Alignment, n*sizeof(T)); break;
        case MEMORY_HBM_1G: ret = memkind_posix_memalign(MEMKIND_HBW_GBTLB,   (void**)&ptr, Alignment, n*sizeof(T)); break;
    }

    if (ret != 0) throw std::bad_alloc();

    return ptr;
}

template <typename T, MemoryType Type, size_t Alignment>
void AlignedAllocator<T,Type,Alignment>::deallocate(T* ptr, size_t n) const
{
    switch (Type)
    {
        case MEMORY_DDR_4K: memkind_free(MEMKIND_DEFAULT,     ptr); break;
        case MEMORY_DDR_2M: memkind_free(MEMKIND_HUGETLB,     ptr); break;
        case MEMORY_DDR_1G: memkind_free(MEMKIND_GBTLB,       ptr); break;
        case MEMORY_HBM_4K: memkind_free(MEMKIND_HBW,         ptr); break;
        case MEMORY_HBM_2M: memkind_free(MEMKIND_HBW_HUGETLB, ptr); break;
        case MEMORY_HBM_1G: memkind_free(MEMKIND_HBW_GBTLB,   ptr); break;
    }
}

#elif BLISPP_HAVE_LIBHUGETLBFS

template <typename T, MemoryType Type, size_t Alignment>
T* AlignedAllocator<T,Type,Alignment>::allocate(size_t n) const
{
    T* ptr;

    if (Type == MEMORY_DDR_4K || Type == MEMORY_HBM_4K)
    {
        int ret = posix_memalign((void**)&ptr, Alignment, n*sizeof(T));
        if (ret != 0) throw std::bad_alloc();
    }
    else
    {
        char* orig_ptr = (char*)get_hugepage_region(n*sizeof(T) + Alignment + sizeof(char**), GHR_DEFAULT);
        if (!orig_ptr) throw std::bad_alloc();
        ptr = (T*)align((intptr_t)(orig_ptr+sizeof(char**)), Alignment);
        *((char**)ptr-1) = orig_ptr;
    }

    return ptr;
}

template <typename T, MemoryType Type, size_t Alignment>
void AlignedAllocator<T,Type,Alignment>::deallocate(T* ptr, size_t n) const
{
    if (Type == MEMORY_DDR_4K || Type == MEMORY_HBM_4K)
    {
        free(ptr);
    }
    else
    {
        char* orig_ptr = *((char**)ptr-1);
        free_hugepage_region(orig_ptr);
    }
}

#else

template <typename T, MemoryType Type, size_t Alignment>
T* AlignedAllocator<T,Type,Alignment>::allocate(size_t n) const
{
    T* ptr;
    int ret = posix_memalign((void**)&ptr, Alignment, n*sizeof(T));
    if (ret != 0) throw std::bad_alloc();
    return ptr;
}

template <typename T, MemoryType Type, size_t Alignment>
void AlignedAllocator<T,Type,Alignment>::deallocate(T* ptr, size_t n) const
{
    free(ptr);
}

#endif

}

#endif
