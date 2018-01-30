#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
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
inline std::vector<T> normalizeTimeSeries
(
    std::vector< T > const & x,
    size_t           const   nBarsMax = std::numeric_limits< size_t >::max(),
    int              const   iNormalizationStrategy = 0
)
{
    /**
     * About whether to use [0,1] or [-1,1]
     * @see https://visualstudiomagazine.com/articles/2014/01/01/how-to-standardize-data-for-neural-networks.aspx
     * When dealing with categorical x-data, it's useful to distinguish between binary x-data, such as sex, which can take one of two possible values, and regular categorical data, such as location, which can take one of three or more possible values. Experience has shown that it's better to encode binary x-data using a -1, +1 scheme rather than a 0, 1 scheme.
     * @see https://stats.stackexchange.com/a/231330/130265
     * @see http://scikit-learn.org/stable/modules/preprocessing.html#preprocessing-normalization
     */
    if ( x.size() <= 1 )
        return x;
    if ( nBarsMax == 0 )
        throw std::invalid_argument( "[normalizeTimeSeries] nBarsMax must be > 0!" );
    if ( iNormalizationStrategy == 1 && nBarsMax < 2 )
        throw std::invalid_argument( "[normalizeTimeSeries] nBarsMax must be >= 2 for a strategy which uses the standard deviation!" );

    if ( not ( 0 <= iNormalizationStrategy and iNormalizationStrategy <= 1 ) )
        throw std::invalid_argument( "[normalizeTimeSeries] unsupported normalization strategy!" );

    std::vector<T> result( x.size() );

    auto curMax      = -std::numeric_limits<T>::infinity();
    auto curMin      = +std::numeric_limits<T>::infinity();
    auto curSum      = T(0);
    auto curSum2     = T(0); // sum of squares for stddev
    size_t iStartMax = 0u;

    for ( size_t i = 0u; i < x.size(); ++i )
    {
        /* assume here we have min and max over last nBarsMax bars */
        if ( iNormalizationStrategy == 0 ) /* min max normalization */
        {
            if ( curMin == curMax )
                result[i] = std::numeric_limits< T >::quiet_NaN();
            else
                result[i] = ( x[i] - curMin )/( curMax - curMin );
        }
        else if ( iNormalizationStrategy == 1 ) /* tanh normalization */
        {
            size_t curN = i - iStartMax;
            if ( curN < 2 ) /* at least this many needed for stddev */
            {
                result[i] = std::numeric_limits< T >::quiet_NaN();
            }
            else
            {
                auto const curAverage = 1. * curSum / curN;
                /* 1/(N-1) sum (x_i-<x>)^2 = 1/(N-1) sum [ x_i^2 - 2 x_i <x> +
                 * <x>^2 ] = 1/(N-1) sum x_i^2 + 1/(N-1)*[ N <x>^2 - 2 N <x> <x>]
                 * ( sx2 - N <x>^2 )/( N-1 ) */
                auto const curStdDev  = ( curSum2 - 1. * curSum / curN * curSum ) / ( curN - 1 );
                result[i] = 0.5 + 0.5 * std::tanh( ( x[i] - curAverage )/( 100. * curStdDev ) );
            }
        }


        if ( iNormalizationStrategy == 0 )
        {
            /* update minimum and maximum */
            assert( i >= iStartMax );
            if ( size_t( i - iStartMax ) >= nBarsMax )
            {
                /* if maximum already encompasses too many values, begin to
                 * reduce it again. We only need to recalculate it, if the value
                 * fading out of range is the maximum! */

                /* @see https://stackoverflow.com/a/30915238/2191065 */
                /* if we are more out of range than one value, then we
                 * also won't take a shortcut and just recalculate the max */
                if ( x[ iStartMax ] == curMax || size_t( i - iStartMax ) - nBarsMax > 1u )
                {
                    /* ! >= instead of < basically ignores NaN */
                    curMax = * std::max_element( x.begin() + iStartMax + 1u,
                        x.begin() + i, /* less functor: */
                        []( T x, T y ){ return !( x >= y ); }
                    );
                }

                if ( x[ iStartMax ] == curMin || size_t( i - iStartMax ) - nBarsMax > 1u )
                {
                    /* ! >= instead of < basically ignores NaN */
                    curMin = * std::min_element( x.begin() + iStartMax + 1u,
                        x.begin() + i, /* less functor: */
                        []( T x, T y ){ return !( x >= y ); }
                    );
                }

                iStartMax += 1;
            }
            curMax = std::max( curMax, x[i] );
            curMin = std::min( curMin, x[i] );
        }

        if ( iNormalizationStrategy == 0 )
        {
            /* update mean and stddev */
            if ( size_t( i - iStartMax ) >= nBarsMax )
            {
                /* we assume that it can't be out of range more than one value! */
                if ( nBarsMax < std::numeric_limits< size_t >::max() )
                    assert( size_t( i - iStartMax ) <= nBarsMax + 1u );

                curSum  -= x[ iStartMax ];
                curSum2 -= x[ iStartMax ] * x[ iStartMax ];

                iStartMax += 1;
            }
            curSum  += x[i];
            curSum2 += x[i] * x[i];
        }
    }

    return result;
}



} // namespace Fundamental
