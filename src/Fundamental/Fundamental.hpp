#pragma once


#include <cassert>
#include <ctime>
#include <iomanip>              // get_time, setprecision, scientific
#include <fstream>
#include <limits>               // max10digits
#include <list>
#include <map>
#include <stdexcept>
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


template< typename T_Prec >
inline void dumpData
(
    std::string const & filePath,
    std::map< std::string, std::vector< T_Prec > > const & data
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
