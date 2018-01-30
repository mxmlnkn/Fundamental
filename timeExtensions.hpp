#pragma once

#include <ctime>                        // tm
#include <iomanip>                      // locale
#include <string>
#include <vector>


namespace Fundamental {


/**
 * return time zone without daylight saving
 */
double getTimeZone( void );

/**
 * Written because timegm isn't in the STL. It's in the Linux headers, but not
 * available on Windows
 */
double timegm( std::tm time );

/**
 * Written as a shorthand for get_time with some default values set,
 * reducing complexity of usage
 * Shit crashes because of g++ bug -.-
 *
 * @param[in] dateString time as string, e.g. "2017-06-01 12:34:56"
 * @param[in] formatString formatter for std::get_time e.g. "%Y-%m-%d %H:%M:%S"
 * @param[in] timeZone e.g. +2*60*60 for CEST or +1*60*60 for CET. This is the
 *            assumed time on dateString, i.e. the returned time is will have
 *            timeZone subtracted. timeZone is in seconds!
 * @param[in] locale not necessary if there are no string date parts like 'Wed'
 * @return unix time stamp for date string
 */
/* double parseTime
(
    char const * const dateString  ,
    char const * const formatString,
    double       const timeZone = 0,
    std::locale  const locale   = std::locale("")
); */

/**
 * Helper function for parseTime below
 */
std::string dateFormatterToRegex
(
    std::string s,
    std::vector< std::string > * const pNamedCaptureGroups = NULL
);

/**
 * Written because get_time can't handle dates without leading zeros -.-
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45896
 */
double parseTime
(
    std::string const & sDate,
    std::string const & dateFormatter,
    double      const   timeZone = 0
);


} // namespace compileTime
