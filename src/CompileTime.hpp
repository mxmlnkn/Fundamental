#pragma once


#include <cassert>
#include <ctime>
#include <iomanip>              // get_time
#include <list>
#include <map>
#include <string>               // getline
#include <sstream>
#include <vector>


#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif
#define INF (1.0/0.0)


/**
 * \todo use stirling formula for large n
 **/
inline double factorial(int n)
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
inline constexpr T pow(const T base, unsigned const exponent) {
    return exponent == 0 ? 1 : base * pow<T>(base, exponent-1);
}

} // compileTime

/* Some simplifications when using std::list */

#define CONTAINS(list,value) (std::find(list.begin(), list.end(), value) != list.end() )

template<typename T_DTYPE>
inline std::ostream& operator<<( std::ostream& out, const std::list<T_DTYPE> ls ) {
    out << "{";
    for ( typename std::list<T_DTYPE>::const_iterator it = ls.begin();
    it != ls.end(); ++it )
        out << *it << ",";
    out << "}";
    return out;
}

template<typename T_KEY, typename T_VAL>
inline std::ostream& operator<<( std::ostream& out, std::map<T_KEY,T_VAL> val )
{
    for ( auto it = val.begin(); it != val.end(); ++it )
        out << "map[" << it->first << "] = " << it->second << "\n";
    return out;
}

template<class T>
inline std::ostream& operator<< (std::ostream &out, std::vector<T> data)
{
    //out << std::setprecision(7) << std::setfill('0') << std::setw(9) << std::showpos;
    out << "{";
    if ( data.size() > 0 )
        out << data[0];
    for ( unsigned int i = 1; i < data.size(); ++i )
        out << ", " << data[i];
    out << "}";
    return out;
}


template<typename T>
inline std::string toString( T a )
{
    std::stringstream tmp;
    tmp << a;
    return tmp.str();
}

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


namespace CompileTime {


/**
 * return time zone without daylight saving
 */
inline double getTimeZone( void )
{
    /* don't let the result of mktime become negative, you never know */
    std::time_t t = 48 * 60 * 60;
    /* mktime( gmtime( ... ) ) here mktime assumes local time i.e. when converting back to unix time it subtracts 'time zone shift' */
    return t - std::mktime( std::gmtime( &t ) );
    /* daylight saving is never included in this, because gmtime doesn't set it */
}

inline double timegm( std::tm time )
{
    static double timezone = getTimeZone();
    assert( time.tm_isdst == 0 );
    /* mktime  now tries again to subtract timezone from time, so we need to
     * readd it to get proper UTC / GMT time */
    return std::mktime( &time ) + timezone;
}

/**
 * @param[in] dateString time as string, e.g. "2017-06-01 12:34:56"
 * @param[in] formatString formatter for std::get_time e.g. "%Y-%m-%d %H:%M:%S"
 * @param[in] timeZone e.g. +2*60*60 for CEST or +1*60*60 for CET. This is the
 *            assumed time on dateString, i.e. the returned time is will have
 *            timeZone subtracted. timeZone is in seconds!
 * @param[in] locale not necessary if there are no string date parts like 'Wed'
 * @return unix time stamp for date string
 */
inline double parseTime
(
    char const * const dateString  ,
    char const * const formatString,
    double       const timeZone = 0,
    std::locale  const locale   = std::locale("")
)
{
    std::tm t = {};
    std::istringstream ss( dateString );
    ss.imbue( locale );
    ss >> std::get_time( &t, formatString );
    if ( ss.fail() )
        throw std::invalid_argument( "Given input date string couldn't be parsed with given format string." );
    return CompileTime::timegm( t ) - timeZone;
}


} // namespace compileTime
