#pragma once


#include <array>
#include <cassert>
#include <ctime>
#include <iomanip>              // get_time
#include <list>
#include <map>
#include <string>               // getline
#include <sstream>
#include <utility>
#include <vector>

#include "timeExtensions.hpp"
#include "toString.hpp"


template< typename T >
inline std::string toString( T const & a );

template< typename T_DTYPE >
inline std::string toString( std::list<T_DTYPE> const & ls );

template< typename T_Key, typename T_Val >
inline std::string toString( std::map<T_Key,T_Val> const & val );

template< typename T_Key, typename T_Val >
inline std::string toString( std::pair< T_Key, T_Val > const & x );

template< typename T, std::size_t I >
inline std::string toString( std::array<T,I> const & data );

template< typename T >
inline std::string toString( std::vector<T> const & data );

inline std::string toString( std::tm const & data );



/* Doesn't work, because already overloaded */
/*
template< typename T >
inline std::ostream& operator<<( std::ostream & out, T const & data )
{
    out << toString( data );
    return out;
}
*/

/* https://stackoverflow.com/questions/13842468/comma-in-c-c-macro */
#define COMMA ,
#define REDIRECT_TO_STRING( TYPE )                                          \
inline std::ostream & operator<<                                            \
(                                                                           \
    std::ostream & out,                                                     \
    TYPE const & data                                                       \
)                                                                           \
{                                                                           \
    out << toString( data );                                                \
    return out;                                                             \
}

template< typename T, std::size_t I > REDIRECT_TO_STRING( std::array<T COMMA I> )
template< typename K, typename V > REDIRECT_TO_STRING( std::map<K COMMA V>  )
template< typename K, typename V > REDIRECT_TO_STRING( std::pair<K COMMA V> )
template< typename T             > REDIRECT_TO_STRING( std::list<T>         )
template< typename T             > REDIRECT_TO_STRING( std::vector<T>       )
REDIRECT_TO_STRING( std::tm )

#undef REDIRECT_TO_STRING
#undef COMMA


/**
 * Watch out, for types which don't have operator<< overloaded this gets
 * called recursively!
 */
template< typename T >
inline std::string toString( T const & a )
{
    std::stringstream tmp;
    tmp << a;
    return tmp.str();
}

template< typename T_DTYPE >
inline std::string toString( std::list<T_DTYPE> const & ls )
{
    std::stringstream out;
    out << "{";
    for ( typename std::list<T_DTYPE>::const_iterator it = ls.begin();
    it != ls.end(); ++it )
        out << *it << ",";
    out << "}";
    return out.str();
}

template< typename T_Key, typename T_Val >
inline std::string toString( std::map<T_Key,T_Val> const & val )
{
    std::stringstream out;
    for ( auto it = val.begin(); it != val.end(); ++it )
        out << "map[" << it->first << "] = " << it->second << "\n";
    return out.str();
}

template< typename T_Key, typename T_Val >
inline std::string toString( std::pair< T_Key, T_Val > const & x )
{
    std::stringstream out;
    out << "pair( " << x.first << ", " << x.second << " )";
    return out.str();
}

/* template deduction for this (and operator<<) fails unfortunately :(,
 * so operator<< not usable as tempalte paramaters would need to be specified! */
template< typename T, std::size_t I >
inline std::string toString( std::array<T,I> const & data )
{
    std::stringstream out;
    //out << std::setprecision(7) << std::setfill('0') << std::setw(9) << std::showpos;
    out << "(";
    if ( data.size() > 0 )
        out << data[0];
    for ( unsigned int i = 1; i < data.size(); ++i )
        out << ", " << data[i];
    out << ")";
    return out.str();
}

template< typename T >
inline std::string toString( std::vector<T> const & data )
{
    std::stringstream out;
    //out << std::setprecision(7) << std::setfill('0') << std::setw(9) << std::showpos;
    out << "{";
    if ( data.size() > 0 )
        out << data[0];
    for ( unsigned int i = 1; i < data.size(); ++i )
        out << ", " << data[i];
    out << "}";
    return out.str();
}

inline std::string toString( std::tm const & data )
{
    std::stringstream out;
    out
    << "tm\n{\n"
    << "    tm_sec   : " << data.tm_sec   << "\n"
    << "    tm_min   : " << data.tm_min   << "\n"
    << "    tm_hour  : " << data.tm_hour  << "\n"
    << "    tm_mday  : " << data.tm_mday  << "\n"
    << "    tm_mon   : " << data.tm_mon   << "\n"
    << "    tm_year  : " << data.tm_year  << "\n"
    << "    tm_wday  : " << data.tm_wday  << "\n"
    << "    tm_yday  : " << data.tm_yday  << "\n"
    << "    tm_isdst : " << data.tm_isdst << "\n"
    << "}\n";
    return out.str();
}
