#ifndef _BLISPP_HPP_
#define _BLISPP_HPP_

#include "blis/blis.h"
#include "blis/bli_obj_macro_defs.h"
#include "blis/bli_scalar_macro_defs.h"

#include <complex>
#include <type_traits>

namespace blis
{

template <typename T> struct real_type                  { typedef T type; };
template <typename T> struct real_type<std::complex<T>> { typedef T type; };
template <typename T> using real_type_t = typename real_type<T>::type;

template <typename T> struct complex_type                  { typedef std::complex<T> type; };
template <typename T> struct complex_type<std::complex<T>> { typedef std::complex<T> type; };
template <typename T> using complex_type_t = typename complex_type<T>::type;

template <typename T> struct is_real                  { static const bool value =  true; };
template <typename T> struct is_real<std::complex<T>> { static const bool value = false; };

template <typename T> struct is_complex                  { static const bool value = false; };
template <typename T> struct is_complex<std::complex<T>> { static const bool value =  true; };

namespace detail
{
    template <typename T, typename U,
              typename V = std::complex<typename std::common_type<real_type_t<T>,
                                                                  real_type_t<U>>::type>,
              bool can_be_same=false>
    using complex_op =
        typename std::enable_if<(!std::is_same<T,U>::value ||
                                 can_be_same) &&
                                (is_complex<T>::value ||
                                 is_complex<U>::value),V>::type;
}

typedef std::complex< float> sComplex;
typedef std::complex<double> dComplex;

inline sComplex cmplx(const scomplex& x) { return sComplex(bli_creal(x), bli_cimag(x)); }

inline dComplex cmplx(const dcomplex& x) { return dComplex(bli_creal(x), bli_cimag(x)); }

template <typename T, typename U>
detail::complex_op<T,U> operator+(const T& a, const U& b)
{
    typedef detail::complex_op<T,U> V;
    return V(a) + V(b);
}

template <typename T, typename U>
detail::complex_op<T,U> operator-(const T& a, const U& b)
{
    typedef detail::complex_op<T,U> V;
    return V(a) - V(b);
}

template <typename T, typename U>
detail::complex_op<T,U> operator*(const T& a, const U& b)
{
    typedef detail::complex_op<T,U> V;
    return V(a) * V(b);
}

template <typename T, typename U>
detail::complex_op<T,U> operator/(const T& a, const U& b)
{
    typedef detail::complex_op<T,U> V;
    return V(a) / V(b);
}

template <typename T> struct datatype;
template <> struct datatype<   float> { static const num_t value =    BLIS_FLOAT; };
template <> struct datatype<  double> { static const num_t value =   BLIS_DOUBLE; };
template <> struct datatype<scomplex> { static const num_t value = BLIS_SCOMPLEX; };
template <> struct datatype<dcomplex> { static const num_t value = BLIS_DCOMPLEX; };
template <> struct datatype<sComplex> { static const num_t value = BLIS_SCOMPLEX; };
template <> struct datatype<dComplex> { static const num_t value = BLIS_DCOMPLEX; };

template <typename T>
typename std::enable_if<!is_complex<T>::value,T>::type
conj(T val) { return val; }

template <typename T>
typename std::enable_if<is_complex<T>::value,T>::type
conj(T val) { return std::conj(val); }

using std::real;
using std::imag;

template <typename T>
typename std::enable_if<!is_complex<T>::value,T>::type
norm2(T val) { return val*val; }

template <typename T>
typename std::enable_if<is_complex<T>::value,T>::type
norm2(T val) { return real(val)*real(val) + imag(val)*imag(val); }

template <typename T, typename U>
detail::complex_op<T,U,bool> operator==(const T& a, const U& b)
{
    return real(a) == real(b) && imag(a) == imag(b);
}

template <typename T, typename U>
detail::complex_op<T,U,bool> operator!=(const T& a, const U& b)
{
    return !(a == b);
}

template <typename T, typename U>
detail::complex_op<T,U,bool,true> operator<(const T& a, const U& b)
{
    return real(a)+imag(a) < real(b)+imag(b);
}

template <typename T, typename U>
detail::complex_op<T,U,bool,true> operator>(const T& a, const U& b)
{
    return b < a;
}

template <typename T, typename U>
detail::complex_op<T,U,bool,true> operator<=(const T& a, const U& b)
{
    return !(b < a);
}

template <typename T, typename U>
detail::complex_op<T,U,bool,true> operator>=(const T& a, const U& b)
{
    return !(a < b);
}

}

#endif
