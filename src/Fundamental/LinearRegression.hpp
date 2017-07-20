#pragma once

#include <cmath>
#include <limits>
#include <vector>


namespace Fundamental {



/**
 * @return vector with 3 elements. In this order: slope, offset, correlation
 *         coefficient
 */
template< typename T >
inline
std::vector< double > fitLine
(
    std::vector< T > const & rX,
    std::vector< T > const & rY
)
{
    size_t const n = std::min( rX.size(), rY.size() );
    if ( n < 1u )
        return {};

    auto const nan = std::numeric_limits< double >::quiet_NaN();
    std::vector< double > result( 3, nan );

    /**
     * @see https://de.wikipedia.org/wiki/Methode_der_kleinsten_Quadrate#Lineare_Modellfunktion
     * Ansatz: f(x) = a x + b
     * Data: x_i, y_i for i = 1..n
     * Idea: Minimize C(a,b) = \sum_i ( a x_i + b - y_i )^2
     *   C_a = 2 \sum_i x_i ( a x_i + b - y_i ) = 0
     *   C_b = 2 \sum_i     ( a x_i + b - y_i ) = 0
     *                  <=>
     *   a \sum_i x_i^2 + b \sum_i x_i = \sum_i x_i y_i =: sxy
     *   a \sum_i x_i   + b n          = \sum_i y_i     =: sy
     *           <=>
     *   a sxx + b sx = sxy   | * n  |    | * sx  |
     *   a sx  + b n  = sy    | * sx v-   | * sxx v-
     *           <=>
     *   a sx^2 - a n sxx = n sxy - sx sy
     *   b n sxx - b sx^2 = sy sxx - sxy sx
     *           <=>
     *   a = ( n  sxy - sx  sy )/( sx^2  - n sxx )
     *   b = ( sy sxx - sxy sx )/( n sxx - sx^2  )
     *     = ( sy - a sx ) / n
     *  => calculation for b can be seen as average of all offsets per point
     *     pair with a given slope!
     * C_min = \sum_i ( a^2 x_i + b^2 + y_i^2 + 2( a b x_i - a x_i y_i - b y_i ) )
     *       = a^2 sx + n b^2 + syy + 2 a ( b sx - sxy ) - 2 b sy
     *       = ...
     */
    double sx, sxx, sxy, sy, syy;    // sums
    sx = sxx = sxy = sy = syy = 0;

    for ( size_t i = 0u; i < n; ++i )
    {
        auto const & x = rX.at(i);
        auto const & y = rY.at(i);
        sx  += x;
        sxx += x * x;
        sy  += y;
        syy += y * y;
        sxy += x * y;
    }

    result[0] = (n * sxy - sx * sy) / (n * sxx - sx * sx);   // slope
    result[1] = (sy * sxx - sx * sxy)/ (n * sxx - sx * sx);  // offset
    /* https://en.wikipedia.org/wiki/Pearson_correlation_coefficient */
    result[2] = (n * sxy - sx * sy)/ std::sqrt((n * sxx - sx * sx)*(n * syy - sy * sy)); // correlation

    return result;
}


/**
 * Will fit parallel lines. I.e. fitting multiple lines with the constraint
 * that the slope of all lines is to be equal
 *
 * If x and y data length for one line do not match, then the shortest length
 * will be chosen. One or more line data sets being effectively zero length
 * is allowed. Only if there is no effective data for any line will there be
 * no fitting done.
 *
 * @todo allow weights for each data per line and for each line as a whole, i.e.
 *       in respect to the other lines. Maybe do that in another function as it
 *       affects performance quite a bit
 *
 * @param[in] rX a "list" of x data sets for each line. The argument uses
 *            a vector of const references to vectors, so specifying the
 *            same x data set for each line isn't a performance problem, it
 *            won't be copied n times
 * @param[in] rY a vector of y data set vectors. One vector for each line
 * @return vector containing in this order: slope offset1 offset2 offset3 ...
 *         offset_n correlation
 */
template< typename T >
inline
std::vector< double > fitParallelLines
(
    std::vector< std::vector< T > * > const & rX,
    std::vector< std::vector< T > * > const & rY
)
{
    /**
     * Ansatz: f_j(x) = a x + b_j for j = 1..m the index for each separate
     *                                         but parallel line
     * Data: x_j_i, y_j_i for i = 1..n_j and j = 1..m
     * Idea: Minimize C(a,b_0,..,b_m) = \sum_j \sum_i ( a x_j_i + b_j - y_j_i )^2
     * This gives an m-d linear equation system:
     *   C_a   = 2 \sum_j \sum_i ( a x_j_i + b_j - y_j_i ) x_j_i = 0
     *   C_b_j = 2        \sum_i ( a x_j_i + b_j - y_j_i )       = 0
     *     -> all sums with wrong b_k (with k!=j) derivate to 0!
     *                  <=>
     *   a ssxx         + \sum_j b_j \sum_i x_j_i = ssxy            (1)
     *   a \sum_i x_j_i + n_j b_j                 = \sum_i y_j_i    (2_j)
     * Some shorthands: \sum_i x_j_i =: sxj, similar for syj
     * Eliminate b_j by:
     *   (1) - \sum_j (2_j) / n_j \sum_i x_j_i  == (1) - \sum_j (2_j) / n_j sxj
     *                  <=>
     *   a ( ssxx - \sum_j sxj^2 / n_j ) = ssxy - \sum_j syj sxj / n_j
     * => a = ( ssxy - \sum_j syj sxj / n_j )/( ssxx - \sum_j sxj^2 / n_j )
     * Now that we have the slope the offset should only depend on the data
     * per line without interdependency. And exactly that can be observed in
     * (2_j), i.e. knowing a we can simply solve for b_j:
     *    b_j = ( syj - a sxj ) / n_j
     *
     * C_min = \sum_j \sum_i ( a^2 x_j_i^2 + b_j^2 + y_j_i^2  +
     *                         2 a x_j_i b_j - 2 a x_j_i y_j_i - 2 b_j y_j_i )
     * C_min = a^2 ssx + n_j sbb + ssyy + 2a ( \sum_j b_j sxj - ssxy )
     *       - 2 \sum_j b_j syj
     */

    size_t const nLines = std::min( rX.size(), rY.size() );
    if ( nLines < 1u )
        return {};

    auto const nan = std::numeric_limits< double >::quiet_NaN();
    std::vector< double > result( 1 + nLines + 1, nan );

    std::vector< double > sx ( nLines, 0 );
    std::vector< double > sy ( nLines, 0 );
    std::vector< double > sxy( nLines, 0 );
    std::vector< double > sxx( nLines, 0 );
    std::vector< double > syy( nLines, 0 );
    double ssxx = 0, ssxy = 0;
    //double ssx = 0, ssy = 0, ssyy = 0;
    double sumSxjSquared = 0;
    double sumSxjSyj     = 0;

    for ( size_t j = 0u; j < nLines; ++j )
    {
        auto const nValuesPerLine = std::min( rX.at(j)->size(), rY.at(j)->size() );
        for ( size_t i = 0u; i < nValuesPerLine; ++i )
        {
            auto const & x = rX.at(j)->at(i);
            auto const & y = rY.at(j)->at(i);
            sx .at(j) += x;
            sy .at(j) += y;
            sxy.at(j) += x*y;
            sxx.at(j) += x*x;
            syy.at(j) += y*y;
        }
        //ssx  += sx .at(j);
        //ssy  += sy .at(j);
        ssxy += sxy.at(j);
        ssxx += sxx.at(j);
        //ssyy += syy.at(j);
        sumSxjSquared += sx[j] * sx[j] / nValuesPerLine;
        sumSxjSyj     += sx[j] * sy[j] / nValuesPerLine;
    }
    // ( ssxy - \sum_j syj sxj / n_j )/( ssxx - \sum_j sxj^2 / n_j )
    result[0] = ( ssxy - sumSxjSyj )/( ssxx - sumSxjSquared );

    /* Calculate offsets: b_j = ( syj - a sxj ) / n_j */
    for ( size_t j = 0u; j < nLines; ++j )
    {
        auto const nValuesPerLine = std::min( rX.at(j)->size(), rY.at(j)->size() );
        result.at(1+j) = ( sy[j] - result[0] * sx[j] ) / nValuesPerLine;
    }

    return result;
}


/**
 * missing weights are handled like missing x or y data. I.e. the minimum
 * length vector of x,y and w will be used
 * Note that weight per line is just a user-feature and basically redundant,
 * as it can just be applied to each data weights.
 */
template< typename T >
inline
std::vector< double > fitParallelLines
(
    std::vector< std::reference_wrapper< std::vector< T > > > const & rX,
    std::vector< std::reference_wrapper< std::vector< T > > > const & rY,
    std::vector< std::reference_wrapper< std::vector< T > > > const & rW,
    std::vector< T > const & rWeightsPerLine = {}
)
{
    return {};
}



} // namespace Fundamental
