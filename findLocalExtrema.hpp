#pragma once

#include <cmath>
#include <utility>
#include <vector>


namespace Fundamental {



/**
 * Returns index and values of all value in original data set which are
 * extremal in index +- nBarsLeftRight
 *
 * @param[in] nBarsLeftRight see header description. The larger this value the
 *            fewer values bill returned. This basically controls how far
 *            out of fractal data we want to zoom.
 *            The larger the value the more values at the start and end of
 *            the input data set will have to be ignored!
 * @return min/max pair of index/value pairs
 */
template< typename T >
std::pair<
    std::pair< std::vector< size_t >, std::vector< T > >, /* Minimums */
    std::pair< std::vector< size_t >, std::vector< T > >  /* Maximums */
>
inline findLocalExtrema
(
    std::vector< T > const & x,
    unsigned int     const   nBarsLeftRight
)
{
    std::vector< size_t > viMin, viMax;
    std::vector< double >  vMin,  vMax;

    for ( size_t i = 0u; i < x.size(); ++i )
    {
        if ( not ( i >= nBarsLeftRight ) or not ( i + nBarsLeftRight < x.size() ) )
            continue;

        double tmpMax = 0;
        double tmpMin = std::numeric_limits< double >::infinity();
        for ( size_t j = i - nBarsLeftRight; j <= i + nBarsLeftRight; ++j )
        {
            tmpMax = std::max( tmpMax, x.at(j) );
            tmpMin = std::min( tmpMin, x.at(j) );
        }

        if ( tmpMin == x.at(i) )
        {
            viMin.push_back( i );
             vMin.push_back( tmpMin );
        }
        if ( tmpMax == x.at(i) )
        {
            viMax.push_back( i );
             vMax.push_back( tmpMax );
        }
    }

    return { { viMin, vMin }, { viMax, vMax } };
}



} // namespace Fundamental
