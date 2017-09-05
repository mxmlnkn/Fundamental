#pragma once

#include <iostream>
#include <map>
#include <utility>
#include <vector>


template< typename T, typename V >
inline std::ostream & operator<<( std::ostream & out, std::pair<T,V> x )
{
    out << "( " << x.first << ", " << x.second << " )";
    return out;
}

template< typename T >
inline std::ostream & operator<<( std::ostream & out, std::vector<T> x )
{
    out << "{ ";
    for ( auto const & xi : x )
        out << xi << ", ";
    out << "}";
    return out;
}

template< typename T, typename V >
inline std::ostream & operator<<( std::ostream & out, std::map<T,V> x )
{
    out << "{ ";
    for ( auto const & xi : x )
        out << xi.first << " -> " << xi.second << ", ";
    out << "}";
    return out;
}
