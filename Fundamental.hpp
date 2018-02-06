#pragma once


#include <cassert>


#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif
#define INF (1.0/0.0)


template< typename T, typename S >
inline T ceilDiv( T a, S b )
{
    assert( b != S(0) );
    assert( a == a );
    assert( b == b );
    return ( a + b - T(1) ) / b;
}

template< typename T >
inline bool constexpr isPowerOfTwo( T const x )
{
    return ! ( x == T(0) ) && ! ( x & ( x - T(1) ) );
}


/**
 * @todo use stirling formula for large n
 **/
inline long double factorial( int n )
{
    assert( n > 0 );
    long double res = 1; // double is exact for integer values! meaning integer is a subset of floating point (at least up to a certain size)
    for ( int i = 2; i <= n; ++i )
        res *= i;
    return res;
}

#include <cmath>
#include <vector>

template< typename T_Prec >
inline
T_Prec mean( std::vector<T_Prec> const vec )
{
    auto sum = T_Prec(0);
    for ( auto const & elem : vec )
        sum += elem;
    return sum / vec.size();
}

template< typename T >
inline double relErr( T const & x, T const & y )
{
    if ( x == y )   /* necessary to avoid .../0 */
        return 0;
    return ( x - y ) / std::max( std::abs( x ), std::abs( y ) );
}

#include <limits>

/**
 * @param[in] nanStrategy basically it boils down to a truth table:
 *            @verbatim
 *                                                        nanStrategy
 *            Possibilities:                    Bit  0  1  2  3  4  5  6  7
 *              x=nan, y=nan => relErr=0,inf    0/1  0  0  0  0  1  1  1  1
 *              x=nan, y=123 => relErr=0,inf    0/1  0  0  1  1  0  0  1  1
 *              x=123, y=nan => relErr=0,inf    0/1  0  1  0  1  0  1  0  1
 *            @endverbatim
 *            So nanStrategy=0 means, that any nan will be ignored, i.e. 0
 *            difference assumed
 *            nanStrategy=7 will penalize any nan with infinite relativ error
 *            Normally, permutations of the last two bits might be chose
 *            symmetrically, but there might be use-cases where there is a
 *            truth and another array amsked with NaN
 *            nanStrategy= 0b011 = 3 is default, because it corresponds to
 *            string representations being equal, i.e. nan == nan is what we
 *            want
 */
template< typename T >
inline double maxRelErr
(
    std::vector<T> const & x,
    std::vector<T> const & y,
    int            const nanStrategy = 3
)
{
    if ( x.size() != y.size() )
        return std::numeric_limits< double >::infinity();

    double max = 0;
    for ( size_t i = 0u; i < x.size(); ++i )
    {
        if ( std::isnan( x[i] ) && std::isnan( y[i] ) )
        {
            if ( nanStrategy & ( 1 << 0 ) )
            {
                max = std::numeric_limits< double >::infinity();
                break;
            }
            else
                continue;
        }
        if ( std::isnan( x[i] ) and not std::isnan( y[i] ) )
        {
            if ( nanStrategy & ( 1 << 1 ) )
            {
                max = std::numeric_limits< double >::infinity();
                break;
            }
            else
                continue;
        }
        if ( not std::isnan( x[i] ) and std::isnan( y[i] ) )
        {
            if ( nanStrategy & ( 1 << 2 ) )
            {
                max = std::numeric_limits< double >::infinity();
                break;
            }
            else
                continue;
        }
        max = std::max( max, std::abs( relErr( x[i], y[i] ) ) );
    }

    return max;
}


/**
 * < (x - <x>)^2 > = < x^2 + <x>^2 - 2x<x> > = <x^2> - <x>^2
 **/
template< typename T_Prec >
inline
T_Prec stddev( std::vector<T_Prec> const vec )
{
    auto sum2 = T_Prec(0);
    for ( auto const elem : vec )
        sum2 += elem*elem;
    auto avg = mean( vec );
    auto const N = T_Prec( vec.size() );
    return std::sqrt( ( sum2/N - avg*avg )*N/(N-1) );
}


namespace compileTime {

/* Compile time power (also exact for integers) */
template<typename T>
inline constexpr T pow( const T base, unsigned const exponent )
{
    return exponent == 0 ? 1 : base * pow<T>(base, exponent-1);
}

} // compileTime

/* Some simplifications when using std::list */

#define CONTAINS(list,value) (std::find(list.begin(), list.end(), value) != list.end() )


#include <vector>
#include <string>                       // getline
#include <sstream>

inline std::vector< double > toDouble
(
    std::vector< std::string >::const_iterator first,
    std::vector< std::string >::const_iterator end
)
{
    std::vector< double > res;
    for ( auto it = first; it != end; ++it )
    {
        std::stringstream ss( *it );
        double x;
        ss >> x;
        res.push_back( x );
    }
    return res;
}


inline std::vector<std::string> split
(
    std::string const & src,
    char const delim
)
{
    std::stringstream ss( src );
    std::string item;
    std::vector< std::string > result;
    while ( std::getline( ss, item, delim ) )
        result.push_back( item );
    return result;
}

/**
 * Replaces all occurences of string 'from' to 'to' in string 'str'
 *
 * @see https://stackoverflow.com/a/24315631/2191065
 */
inline std::string replace
(
    std::string str,
    std::string const & from,
    std::string const & to
)
{
    size_t i = 0;
    while ( ( i = str.find( from, i ) ) != std::string::npos )
    {
        str.replace( i, from.length(), to );
        i += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}


#include <iomanip>              // setprecision, scientific, setw
#include <fstream>
#include <limits>               // max_digits10
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>              // pair

/**
 * Inspired by numpy.genfromtxt and writetotxt
 *
 * Can't use std::map as input, because it would lose the order -.-
 */
template< typename T_Prec >
inline void dumpData
(
    std::string const & filePath,
    std::vector< std::pair< std::string, std::vector< T_Prec > > > const & data
)
{
    std::ofstream file;
    file.open( filePath );
    if ( file.fail() )
        throw std::invalid_argument( "Couldn't open file!" );

    /* write out description / comment line : # t x v ... */
    /* precision 2 means 3.1, 1 means 3, 3 means 3.14, 0 means 3, -1 means 3.14159 (for some weird reason) */
    auto const prec  = std::numeric_limits< T_Prec >::max_digits10;
    auto const width = prec + 8;   // "+p.pppe+999 "

    file << "#";
    for ( auto const & kv : data )
        file << std::setw( width ) << kv.first;
    file << "\n";

    size_t iRow = 0u;
    bool nonEmptyColumnFound = false;
    do
    {
        nonEmptyColumnFound = false;
        /* print each column */
        for ( auto const & kv : data )
        {
            if ( iRow < kv.second.size() )
            {
                file << std::setw( width ) << std::scientific << std::setprecision( prec ) << kv.second[iRow];
                nonEmptyColumnFound = true;
            }
            //else if ( std::numeric_limits< T_Prec > has_quiet_NaN )
            //    file << std::setw( width ) << std::numeric_limits< T_Prec > quiet_NaN();
            else
                file << std::setw( width ) << " ";  // not really the best way, but I don't wanna add wrong data ...
        }
        file << "\n";
        ++iRow;
    } while ( nonEmptyColumnFound );

    file.close();
}

#include <chrono>

/* Can't simply use 'using', because now is a static method of
 * high_resolution_clock class.. */
inline std::chrono::time_point< std::chrono::high_resolution_clock > now( void )
{
    return std::chrono::high_resolution_clock::now();
}

inline double diffNow
(
    std::chrono::time_point< std::chrono::high_resolution_clock > const & t0,
    std::chrono::time_point< std::chrono::high_resolution_clock > const & t1
)
{
    return std::chrono::duration_cast< std::chrono::duration<double> >( t1 - t0 ).count();
}



class RandomBitGenerator
{
private:
    uint64_t           lastRandomNumber;
    unsigned int       nBitsUsed       ;
    unsigned int const nBitsUsable     ;

public:
    inline RandomBitGenerator()
    : lastRandomNumber( std::rand() ),
      nBitsUsed( 0 ),
      nBitsUsable( std::floor( std::log2( (uint64_t) RAND_MAX ) + 1 ) )  // max 3 (has 2 bits). meh hard to do correct, because I can't add +1 to the max range of the same value... would get overflow, mah in almost any cases RAND_MAX will be at som 2^n-1, so it should be correct -.- Can't cover every edge case
    {}

    inline bool decide()
    {
        if ( nBitsUsed >= nBitsUsable )
        {
            lastRandomNumber = std::rand();
            nBitsUsed        = 0;
        }

        ++nBitsUsed;
        bool const bit = lastRandomNumber & uint64_t(1);
        lastRandomNumber = lastRandomNumber >> 1;

        return bit;
    }
};
