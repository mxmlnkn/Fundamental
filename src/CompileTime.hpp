#pragma once


#include <list>
#include <map>
#include <string>
#include <sstream>
#include <cassert>


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

