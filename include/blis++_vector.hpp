#ifndef _BLISPP_VECTOR_HPP_
#define _BLISPP_VECTOR_HPP_

#include "blis++_matrix.hpp"

namespace blis
{

namespace detail
{
    enum {ROW, COLUMN};

    template <typename T, int Dir>
    class Vector : public Matrix<T>
    {
        public:
            using typename Matrix<T>::type;
            using typename Matrix<T>::real_type;

            Vector() {}

            Vector(const Vector& other)
            : Matrix<type>(other) {}

            Vector(Vector&& other)
            : Matrix<type>(std::move(other)) {}

            explicit Vector(dim_t n, inc_t inc = 1)
            : Matrix<type>(Dir == ROW ?     n :   1,
                           Dir == ROW ?     1 :   n,
                           Dir == ROW ?   inc :   1,
                           Dir == ROW ? n*inc : inc) {}

            Vector(dim_t n, type* p, inc_t inc = 1)
            : Matrix<type>(Dir == ROW ?     n :   1,
                           Dir == ROW ?     1 :   n,
                           p,
                           Dir == ROW ?   inc :   1,
                           Dir == ROW ? n*inc : inc) {}

            Vector& operator=(const Vector& other)
            {
                Matrix<type>::operator=(other);
                return *this;
            }

            Vector& operator=(Vector&& other)
            {
                Matrix<type>::operator=(std::move(other));
                return *this;
            }

            Vector& operator=(const type& val)
            {
                Matrix<T> s(val);
                bli_setv(s, this);
                return *this;
            }
    };

}

template <typename T> using RowVector = detail::Vector<T,detail::ROW>;
template <typename T> using ColumnVector = detail::Vector<T,detail::COLUMN>;

}

#endif
