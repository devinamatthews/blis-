#ifndef _BLISPP_MATRIX_HPP_
#define _BLISPP_MATRIX_HPP_

#include "blis++.hpp"

namespace blis
{

struct trans_op_t
{
    objbits_t info;

    constexpr trans_op_t(trans_t trans = BLIS_NO_TRANSPOSE) : info(trans) {}

    bool transpose() const
    {
        return bli_obj_onlytrans_status(*this);
    }

    bool conjugate() const
    {
        return bli_obj_conj_status(*this);
    }

    operator trans_t() const { return (trans_t)bli_obj_conjtrans_status(*this); }
};

namespace transpose
{
    constexpr trans_op_t T(BLIS_TRANSPOSE);
    constexpr trans_op_t C(BLIS_CONJ_NO_TRANSPOSE);
    constexpr trans_op_t H(BLIS_CONJ_TRANSPOSE);
}

namespace detail
{
    template <typename T> using if_complex =
        typename std::enable_if<is_complex<T>::value>::type;
}

template <typename T>
class Matrix : private obj_t
{
    public:
        typedef T type;
        typedef typename real_type<T>::type real_type;

    private:
        bool _is_view;

    protected:
        void create()
        {
            _is_view = true;
            memset(static_cast<obj_t*>(this), 0, sizeof(obj_t));
        }

        void create(const Matrix& other)
        {
            _is_view = false;

            create(other.length(), other.width(),
                   other.row_stride(), other.col_stride());

            Matrix other_no_trans(length(), width(),
                                  const_cast<type*>(static_cast<const type*>(other)),
                                  row_stride(), col_stride());

            bli_copym(other_no_trans, this);

            this->conjtrans(other.conjtrans());
        }

        void create(Matrix&& other)
        {
            memcpy(static_cast<obj_t*>(this),
                   static_cast<obj_t*>(&other), sizeof(obj_t));

            _is_view = other._is_view;
            other._is_view = true;
        }

        void create(double r, double i)
        {
            _is_view = false;
            bli_obj_scalar_init_detached(datatype<type>::value, this);
            bli_setsc(r, i, this);
        }

        void create(dim_t m, dim_t n)
        {
            _is_view = false;
            bli_obj_create(datatype<type>::value, m, n, 0, 0, this);
        }

        void create(dim_t m, dim_t n, inc_t rs, inc_t cs)
        {
            _is_view = false;
            bli_obj_create(datatype<type>::value, m, n, rs, cs, this);
        }

        void create(dim_t m, dim_t n, type* p)
        {
            _is_view = true;
            bli_obj_create_with_attached_buffer(datatype<type>::value, m, n, p, 1, m, this);
        }

        void create(dim_t m, dim_t n, type* p, inc_t rs, inc_t cs)
        {
            _is_view = true;
            bli_obj_create_with_attached_buffer(datatype<type>::value, m, n, p, rs, cs, this);
        }

        void free()
        {
            mem_t* mem_p = bli_obj_pack_mem(*this);

            if (!bli_mem_is_unalloc(mem_p))
            {
                bli_mem_release(mem_p);
            }

            if (!_is_view) bli_obj_free(this);
        }

        Matrix(double r, double i)
        {
            create(r, i);
        }

    public:
        Matrix()
        {
            create();
        }

        Matrix(const Matrix& other)
        {
            create(other);
        }

        Matrix(Matrix&& other)
        {
            create(std::move(other));
        }

        Matrix(dim_t m, dim_t n)
        {
            create(m, n);
        }

        Matrix(dim_t m, dim_t n, inc_t rs, inc_t cs)
        {
            create(m, n, rs, cs);
        }

        explicit Matrix(type* p)
        {
            create(1, 1, p);
        }

        Matrix(dim_t m, dim_t n, type* p)
        {
            create(m, n, p);
        }

        Matrix(dim_t m, dim_t n, type* p, inc_t rs, inc_t cs)
        {
            create(m, n, p, rs, cs);
        }

        ~Matrix()
        {
            free();
        }

        Matrix& operator=(const Matrix& other)
        {
            reset(other);
            return *this;
        }

        Matrix& operator=(Matrix&& other)
        {
            reset(std::move(other));
            return *this;
        }

        Matrix& operator=(const type& val)
        {
            Matrix<T> s(real(val), imag(val));
            bli_setm(s, this);
            return *this;
        }

        void reset()
        {
            free();
            create();
        }

        void reset(const Matrix& other)
        {
            if (&other == this) return;
            free();
            create(other);
        }

        void reset(Matrix&& other)
        {
            if (&other == this) return;
            free();
            create(std::move(other));
        }

        void reset(real_type r, real_type i)
        {
            free();
            create(r, i);
        }

        void reset(const type& val)
        {
            free();
            create(real(val), imag(val));
        }

        void reset(dim_t m, dim_t n)
        {
            free();
            create(m, n, 1, m);
        }

        void reset(dim_t m, dim_t n, inc_t rs, inc_t cs)
        {
            free();
            create(m, n, rs, cs);
        }

        void reset(type* p)
        {
            free();
            create(1, 1, p, 1, 1);
        }

        void reset(dim_t m, dim_t n, type* p)
        {
            free();
            create(m, n, p, 1, m);
        }

        void reset(dim_t m, dim_t n, type* p, inc_t rs, inc_t cs)
        {
            free();
            create(m, n, p, rs, cs);
        }

        bool is_view() const
        {
            return _is_view;
        }

        bool is_transposed() const
        {
            return bli_obj_has_trans(*this);
        }

        bool transpose()
        {
            bool old = is_transposed();
            bli_obj_toggle_trans(*this);
            return old;
        }

        bool transpose(bool trans)
        {
            bool old = is_transposed();
            bli_obj_set_onlytrans(trans ? BLIS_TRANSPOSE : BLIS_NO_TRANSPOSE, *this);
            return old;
        }

        trans_t transpose(trans_t trans)
        {
            trans_t old = bli_obj_onlytrans_status(*this);
            bli_obj_set_onlytrans(trans, *this);
            return old;
        }

        bool is_conjugated() const
        {
            return bli_obj_has_conj(*this);
        }

        bool conjugate()
        {
            bool old = is_conjugated();
            bli_obj_toggle_conj(*this);
            return old;
        }

        bool conjugate(bool conj)
        {
            bool old = is_conjugated();
            bli_obj_set_conj(conj ? BLIS_CONJUGATE : BLIS_NO_CONJUGATE, *this);
            return old;
        }

        conj_t conjugate(conj_t conj)
        {
            conj_t old = bli_obj_conj_status(*this);
            bli_obj_set_conj(conj, *this);
            return old;
        }

        trans_t conjtrans() const
        {
            return (trans_t)bli_obj_conjtrans_status(*this);
        }

        trans_t conjtrans(trans_op_t conjtrans)
        {
            trans_t old = this->conjtrans();
            bli_obj_set_conjtrans(conjtrans, *this);
            return old;
        }

        dim_t length() const
        {
            return bli_obj_length(*this);
        }

        dim_t length(dim_t m)
        {
            dim_t old = length();
            bli_obj_set_length(m, *this);
            return old;
        }

        dim_t width() const
        {
            return bli_obj_width(*this);
        }

        dim_t width(dim_t n)
        {
            dim_t old = width();
            bli_obj_set_width(n, *this);
            return old;
        }

        inc_t row_stride() const
        {
            return bli_obj_row_stride(*this);
        }

        inc_t row_stride(inc_t rs)
        {
            inc_t old = row_stride();
            bli_obj_set_strides(rs, col_stride(), *this);
            return old;
        }

        inc_t col_stride() const
        {
            return bli_obj_col_stride(*this);
        }

        inc_t col_stride(inc_t cs)
        {
            inc_t old = col_stride();
            bli_obj_set_strides(row_stride(), cs, *this);
            return old;
        }

        void shift_down(dim_t m)
        {
            type* p = *this;
            bli_obj_set_buffer(p+m*row_stride(), *this);
        }

        void shift_up(dim_t m)
        {
            shift_down(-m);
        }

        void shift_right(dim_t n)
        {
            type* p = *this;
            bli_obj_set_buffer(p+n*col_stride(), *this);
        }

        void shift_left(dim_t n)
        {
            shift_right(-n);
        }

        void shift_down()
        {
            shift_down(length());
        }

        void shift_up()
        {
            shift_up(length());
        }

        void shift_right()
        {
            shift_right(width());
        }

        void shift_left()
        {
            shift_left(width());
        }

        operator type*()
        {
            return (type*)bli_obj_buffer(*this);
        }

        operator const type*() const
        {
            return (const type*)bli_obj_buffer(*this);
        }

        operator obj_t*()
        {
            return this;
        }

        operator const obj_t*() const
        {
            return this;
        }

        Matrix operator^(trans_op_t trans)
        {
            Matrix view(length(), width(), *this, row_stride(), col_stride());

            if (trans.transpose()) view.transpose();
            if (trans.conjugate()) view.conjugate();

            return view;
        }

        friend void swap(Matrix& a, Matrix& b)
        {
            char tmp[sizeof(Matrix)];
            memcpy(tmp,  &a, sizeof(Matrix));
            memcpy( &a,  &b, sizeof(Matrix));
            memcpy( &b, tmp, sizeof(Matrix));
        }
};

}

#endif
