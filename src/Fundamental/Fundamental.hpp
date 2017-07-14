#pragma once


#include <cassert>
#include <ctime>
#include <iomanip>              // get_time
#include <list>
#include <map>
#include <string>               // getline
#include <sstream>
#include <vector>

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
