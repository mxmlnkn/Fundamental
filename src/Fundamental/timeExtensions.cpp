
#include "timeExtensions.hpp"

#include <cassert>
#include <ctime>
#include <iostream>
#include <map>
#include <regex>
#include <string>               // getline
#include <sstream>
#include <vector>

#include "Fundamental.hpp"


namespace Fundamental {


/**
 * return time zone without daylight saving
 */
double getTimeZone( void )
{
    /* don't let the result of mktime become negative, you never know */
    std::time_t t = 48 * 60 * 60;
    /* mktime( gmtime( ... ) ) here mktime assumes local time i.e. when converting back to unix time it subtracts 'time zone shift' */
    return t - std::mktime( std::gmtime( &t ) );
    /* daylight saving is never included in this, because gmtime doesn't set it */
}

double timegm( std::tm time )
{
    static double timezone = getTimeZone();
    assert( time.tm_isdst == 0 );
    /* mktime  now tries again to subtract timezone from time, so we need to
     * readd it to get proper UTC / GMT time */
    return std::mktime( &time ) + timezone;
}

/*
double parseTime
(
    char const * const dateString  ,
    char const * const formatString,
    double       const timeZone    ,
    std::locale  const locale
)
{
    std::tm t = {};
    std::istringstream ss( dateString );
    ss.imbue( locale );
    ss >> std::get_time( &t, formatString );
    if ( ss.fail() )
        throw std::invalid_argument( "Given input date string couldn't be parsed with given format string." );
    return timegm( t ) - timeZone;
}
*/


std::string dateFormatterToRegex
(
    std::string s,
    std::vector< std::string > * pNamedCaptureGroups
)
{
    std::map< std::string, std::string > shorthands = {
        { "%D", "%m / %d / %y "   },
        { "%r", "%I : %M : %S %p" },
        { "%R", "%H : %M"         },
        { "%T", "%H : %M : %S"    },
    };
    for ( auto const & rule : shorthands )
        s = replace( s, rule.first, rule.second );

    /* Extract capture group names */
    std::vector< std::string > specifiers = { "Y", "y", "m", "j", "d", "e", "w", "H", "M", "S", "p" };
    if ( pNamedCaptureGroups != NULL )
    {
        pNamedCaptureGroups->clear();
        for ( size_t i = 0u; i < s.size(); ++i )
        {
            if ( ( s[i] != '%' ) or not ( i+1 < s.size() ) )
                continue;

            auto const toSearch = s.substr( i+1, 1 );
            auto const found = std::find( specifiers.begin(), specifiers.end(), toSearch );
            if ( found != specifiers.end() )
                pNamedCaptureGroups->push_back( toSearch );
        }
    }

    /* replace everything with regex rules and capture groups */
    /* Default regex is std::regex::ECMAScript */
    std::map< std::string, std::string > rules = {
        { "%%", "%"                         },
        { "%n", "t"                         },
        { "%t", "[ \\t]*"                   },
        { "%Y", "([0-9]{1,4})"              }, // 0 ... 3000
        { "%y", "([0-9]{2})"                }, // 00 ... 99
        { "%m", "(1[0-2]|0?[0-9])"          }, // 01,1 ... 12
        { "%j", "([0-9]{1,3})"              }, // 001.01,1 ... 365
        { "%e", "d"                         },
        { "%d", "(0?[0-9]|[12][0-9]|3[01])" }, // 01,1 ... 31
        { "%w", "([0-6])"                   }, // 1 ... 6
        { "%H", "(0?[0-9]|1[0-9]|2[0-3])"   }, // 00,0 ... 23
        { "%I", "(1[0-2]|0?[0-9])"          }, // 00,0 ... 12
        { "%M", "([0-5]?[0-9])"             }, // 00,0 ... 59
        { "%S", "([0-5]?[0-9])"             }, // 00,0 ... 59 (actually also 60, but I don't see why)
        { "%p", "([apAP]\\.?[mM]\\.?)"      }
    };
    for ( auto const & rule : rules )
        s = replace( s, rule.first, rule.second );

    return s;
}

double parseTime
(
    std::string const & sDate,
    std::string const & dateFormatter,
    double      const   timeZone
)
{
    std::vector< std::string > namedCaptures = {};
    auto const sDateRegex = dateFormatterToRegex( dateFormatter, &namedCaptures );
    std::regex dateRegex( sDateRegex );
    std::smatch matches;
    auto const found = std::regex_match( sDate, matches, dateRegex );
    if ( not found )
        throw std::invalid_argument( "Couldn't parse give string with given formatter." );
    /* first in match is always the whole match, then followed by capture groups */
    /*
    std::cout << "[parseTime] '" << sDate << "' using '" << dateFormatter << "'" << std::endl;
    std::cout << "  Regex        : " << sDateRegex    << std::endl;
    std::cout << "  Capture Names: " << namedCaptures << std::endl;
    std::cout << "  Found: " << found << std::endl;
    std::cout << "  Matches: ";
    for ( auto const & m : matches )
        std::cout << m << ", ";
    std::cout << std::endl;
    */

    std::tm date = {};
    // assert( matches.size() == namedCaptures.size() + 1 );
    for ( size_t i = 0u; i < namedCaptures.size(); ++i )
    {
        /* http://en.cppreference.com/w/cpp/chrono/c/tm */
        auto const & match = matches[i+1].str();
        if ( namedCaptures[i] == std::string( "Y" ) )
            date.tm_year = std::stoi( match ) - 1900;
        else if ( namedCaptures[i] == std::string( "y" ) )
        {
            auto const y = std::stoi( match );
            date.tm_year = ( y < 69 ? 2000 + y : 1900 + y ) - 1900;
        }
        else if ( namedCaptures[i] == std::string( "m" ) )
            date.tm_mon = std::stoi( match ) - 1;   // starts at 0
        else if ( namedCaptures[i] == std::string( "j" ) )
            date.tm_yday = std::stoi( match );
        else if ( ( namedCaptures[i] == std::string( "d" ) ) ||
                  ( namedCaptures[i] == std::string( "e" ) ) )
            date.tm_mday = std::stoi( match );
        else if ( ( namedCaptures[i] == std::string( "H" ) ) ||
                  ( namedCaptures[i] == std::string( "I" ) ) )
            date.tm_hour = std::stoi( match );
        else if ( namedCaptures[i] == std::string( "M" ) )
            date.tm_min = std::stoi( match );
        else if ( namedCaptures[i] == std::string( "S" ) )
            date.tm_sec = std::stoi( match );
        else if ( namedCaptures[i] == std::string( "p" ) )
        {
            if ( ( std::find( match.begin(), match.end(), 'P' ) != match.end() ) ||
                 ( std::find( match.begin(), match.end(), 'p' ) != match.end() ) )
            {
                if ( date.tm_hour < 12 )
                    date.tm_hour += 12;
            }
            if ( ( std::find( match.begin(), match.end(), 'A' ) != match.end() ) ||
                 ( std::find( match.begin(), match.end(), 'a' ) != match.end() ) )
            {
                /* 12:16 AM is 00:16 in 24h -.-
                 * @see https://www.italki.com/question/277978?hl=de
                 * @see Bittrex data set */
                if ( date.tm_hour >= 12 )
                    date.tm_hour -= 12;
            }
        }
    }

    //std::cout << date << std::endl;
    return timegm( date ) - timeZone;
}


} // namespace compileTime
