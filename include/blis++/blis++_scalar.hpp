#ifndef _BLISPP_SCALAR_HPP_
#define _BLISPP_SCALAR_HPP_

#include "blis++_matrix.hpp"

namespace blis
{

template <typename T>
class Scalar : public Matrix<T>
{
    public:
        using typename Matrix<T>::type;
        using typename Matrix<T>::real_type;

        Scalar()
        : Matrix<type>(0.0, 0.0) {}

        Scalar(real_type r, real_type i)
        : Matrix<type>(r, i) {}

        Scalar(const type& val)
        : Matrix<type>(real(val), imag(val)) {}

        explicit Scalar(type* p)
        : Matrix<type>(p) {}

        Scalar(const Scalar& other)
        : Matrix<type>(other) {}

        Scalar(Scalar&& other)
        : Matrix<type>(std::move(other)) {}

        Scalar& operator=(const Scalar& other)
        {
            Matrix<type>::operator=(other);
            return *this;
        }

        Scalar& operator=(Scalar&& other)
        {
            Matrix<type>::operator=(std::move(other));
            return *this;
        }

        Scalar& operator=(const type& val)
        {
            (type&)*this = val;
            return *this;
        }

        operator type&()
        {
            return *(type*)(*this);
        }

        operator const type&() const
        {
            return *(type*)(*this);
        }
};

}

#endif
