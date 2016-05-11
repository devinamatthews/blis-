#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "blis++.hpp"

using namespace std;
using namespace blis;

template <typename T>
using AlignedMatrix = Matrix<T,AlignedAllocator<T,MEMORY_HBM_2M>>;

#define NREPEAT 5

class range
{
    public:
        class range_iterator : std::iterator<std::random_access_iterator_tag,dim_t,inc_t,dim_t*,dim_t>
        {
            private:
                typedef std::iterator<std::random_access_iterator_tag,dim_t,inc_t,dim_t*,dim_t> iterator_base_;

            public:
                using typename iterator_base_::value_type;
                using typename iterator_base_::difference_type;
                using typename iterator_base_::pointer;
                using typename iterator_base_::reference;
                using typename iterator_base_::iterator_category;

                range_iterator()
                : pos_(0), delta_(0) {}

                range_iterator(dim_t pos, inc_t delta)
                : pos_(pos), delta_(delta) {}

                range_iterator(const range_iterator& other) = default;

                range_iterator& operator=(const range_iterator& other) = default;

                range_iterator& operator++()
                {
                    pos_ += delta_;
                    return *this;
                }

                range_iterator& operator--()
                {
                    pos_ -= delta_;
                    return *this;
                }

                range_iterator operator++(int x)
                {
                    range_iterator old(*this);
                    ++old;
                    return old;
                }

                range_iterator operator--(int x)
                {
                    range_iterator old(*this);
                    --old;
                    return old;
                }

                reference operator*() const
                {
                    return pos_;
                }

                pointer operator->() const
                {
                    return nullptr;
                }

                range_iterator& operator+=(difference_type n)
                {
                    pos_ += n*delta_;
                    return *this;
                }

                range_iterator& operator-=(difference_type n)
                {
                    pos_ -= n*delta_;
                    return *this;
                }

                range_iterator operator+(difference_type n) const
                {
                    range_iterator ret(*this);
                    ret += n;
                    return ret;
                }

                friend range_iterator operator+(difference_type n, const range_iterator& x)
                {
                    range_iterator ret(x);
                    ret += n;
                    return ret;
                }

                range_iterator operator-(difference_type n) const
                {
                    range_iterator ret(*this);
                    ret -= n;
                    return ret;
                }

                difference_type operator-(const range_iterator& x) const
                {
                    return pos_ - x.pos_;
                }

                reference operator[](difference_type i) const
                {
                    return pos_ + i*delta_;
                }

                bool operator==(const range_iterator& other) const
                {
                    return pos_ == other.pos_;
                }

                bool operator!=(const range_iterator& other) const
                {
                    return !(*this == other);
                }

                bool operator<(const range_iterator& other) const
                {
                    return pos_ < other.pos_;
                }

                bool operator>(const range_iterator& other) const
                {
                    return other < *this;
                }

                bool operator<=(const range_iterator& other) const
                {
                    return !(other < *this);
                }

                bool operator>=(const range_iterator& other) const
                {
                    return !(*this < other);
                }

            private:
                dim_t pos_;
                inc_t delta_;
        };

        range(dim_t start, dim_t stop, inc_t delta)
        : start_(start), stop_(stop), delta_(delta == 0 ? stop-start : delta) {}

        range_iterator begin() const
        {
            return range_iterator(start_, delta_);
        }

        range_iterator end() const
        {
            return range_iterator(stop_+delta_, delta_);
        }

        range_iterator cbegin() const
        {
            return range_iterator(stop_, -delta_);
        }

        range_iterator cend() const
        {
            return range_iterator(start_-delta_, -delta_);
        }

    private:
        dim_t start_, stop_;
        inc_t delta_;
};

range parse_range(const string& s)
{
    dim_t mn, mx;
    inc_t delta = 1;

    size_t colon1 = s.find(':');
    size_t colon2 = s.find(':', colon1 == string::npos ? colon1 : colon1+1);

    if (colon1 == string::npos)
    {
        mn = mx = stol(s);
    }
    else if (colon2 == string::npos)
    {
        mn = stol(s.substr(0,colon1));
        mx = stol(s.substr(colon1+1));
    }
    else
    {
        mn = stol(s.substr(0,colon1));
        mx = stol(s.substr(colon1+1,colon2-colon1-1));
        delta = stol(s.substr(colon2+1));
    }

    return range(mn, mx, delta);
}

template <typename T>
double run_trial(dim_t m, dim_t n, dim_t k)
{
    double bias = numeric_limits<double>::max();
    for (dim_t r = 0;r < NREPEAT;r++)
    {
        double t0 = bli_clock();
        double t1 = bli_clock();
        bias = min(bias, t1-t0);
    }

    AlignedMatrix<T> A(m,k), B(k,n), C(m,n);
    Scalar<T> alpha(1.0), beta(0.0);

    A = 0.0;
    B = 0.0;
    C = 0.0;

    double dt = numeric_limits<double>::max();
    for (dim_t r = 0;r < NREPEAT;r++)
    {
        double t0 = bli_clock();
        bli_gemm(alpha, A, B, beta, C);
        double t1 = bli_clock();
        dt = min(dt, t1-t0);
    }

    return 2*m*n*k*1e-9/(dt-bias);
}

void run_experiment(char dt, range m_range, range n_range, range k_range)
{
    for (dim_t m : m_range)
    {
        for (dim_t n : n_range)
        {
            for (dim_t k : k_range)
            {
                double gflops;
                switch (dt)
                {
                    case 's': gflops = run_trial<   float>(m, n, k); break;
                    case 'd': gflops = run_trial<  double>(m, n, k); break;
                    case 'c': gflops = run_trial<sComplex>(m, n, k); break;
                    case 'z': gflops = run_trial<dComplex>(m, n, k); break;
                }
                printf("%d %d %d %f\n", m, n, k, gflops);
            }
        }
    }
    cout << endl;
}

int main(int argc, char** argv)
{
    bli_init();

    string line;
    char dt;
    string m_range, n_range, k_range;
    while (getline(cin, line) && !line.empty())
    {
        istringstream(line) >> dt >> m_range >> n_range >> k_range;

        if (string("sdcz").find(dt) == string::npos)
        {
            cerr << "Unknown datatype: " << dt << endl;
            exit(1);
        }

        dim_t m_min, m_max, n_min, n_max, k_min, k_max;
        inc_t m_delta, n_delta, k_delta;
        range m = parse_range(m_range);
        range n = parse_range(n_range);
        range k = parse_range(k_range);

        run_experiment(dt, m, n, k);
    }

    bli_finalize();

    return 0;
}
