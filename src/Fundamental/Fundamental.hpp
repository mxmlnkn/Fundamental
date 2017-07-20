#pragma once


#include <cassert>

#include "timeExtensions.hpp"
#include "toString.hpp"


#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif
#define INF (1.0/0.0)


/**
 * \todo use stirling formula for large n
 **/
inline double factorial( int n )
{
    assert( n > 0 );
    double res = 1; // double is exact for integer values! meaning integer is a subset of floating point
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
    return ( x - y ) / std::abs( std::max( x, y ) );
}

#include <limits>

template< typename T >
inline double maxRelErr
(
    std::vector<T> const & x,
    std::vector<T> const & y
)
{
    if ( x.size() != y.size() )
        return std::numeric_limits< double >::infinity();

    double max = -std::numeric_limits< double >::infinity();
    for ( size_t i = 0u; i < x.size(); ++i )
    {
        if ( std::isnan( x[i] ) && std::isnan( y[i] ) )
            continue;
        if ( std::isnan( x[i] ) || std::isnan( y[i] ) )
        {
            max = std::numeric_limits< double >::infinity();
            break;
        }
        max = std::max( max, relErr( x[i], y[i] ) );
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
