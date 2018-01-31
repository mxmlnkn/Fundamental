#pragma once

#include <cassert>
#include <vector>


/**
 * converts a vector index (i,j) to a linear index i*Nx+j
 *
 * If we have 2 slates of 3x4 length and we wanted the index  [i,j,k] =
 * [1,2,1] (begin with 0!), then the matrix lies in the memory like:
 *   [oooo|oooo|oooo] [oooo|oooo|oxoo]
 * This means to address that element (above marked x) we need to
 * calculate:
 *   lini = k + j*n_k + i*n_j*n_k
 *   21   = 1 + 2*4   + 1*3*4
 *
 * @param[in] rIndex vector index, i.e. (i) or (i,j) or (i,j,k) or
 *            (i0,i1,i2,i3,...)
 * @param[in] rnSize length of dimensions e.g. (4,5) for 2D i.e. 4 elements
 *            in the first dimension and 5 in the second, meaning always 5
 *            elements lie contiguous in memory
 * @return linear index
 **/
template<
    typename T_Vector1     = std::vector< unsigned int >,
    typename T_Vector2     = std::vector< unsigned int >,
    typename T_UintLinear = size_t
>
inline T_UintLinear convertVectorToLinearIndex
(
    T_Vector1 const & rIndex,
    T_Vector2 const & rnSize
)
{
    /* check sanity of input arguments */
    #ifndef NDEBUG
        assert( rIndex.size() == rnSize.size() );
        for ( size_t i = 0; i < rIndex.size(); ++i )
        {
            assert( rIndex[i] < rnSize[i] );
        }
    #endif

    /* convert vector index, e.g. for 10 dimensions:
     *   lini = i9 + i8*n9 + i7*n9*n8 + i6*n9*n8*n7 + ... + i0*n9*...*n1 */
    T_UintLinear linIndex  = 0;
    T_UintLinear prevRange = 1;
    for ( long int i = (long int) rnSize.size() - 1; i >= 0; --i )
    {
        linIndex  += rIndex[i] * prevRange;
        prevRange *= rnSize[i];
    }
    return linIndex;
}

/**
 * Reverses the effect of @see convertVectorToLinearIndex
 *
 * To reverse the equation
 *   lini = i9 + i8*n9 + i7*n9*n8 + i6*n9*n8*n7 + ... + i0*n9*...*n1
 *   21   = 1  + 2*4   + 1*3*4 + 0
 * we can use subsequent modulos
 *   k   = 21 mod (n9=4) = 1
 *   tmp = 21  /  (n9=4) = 5
 *   j   = 5  mod (n8=3) = 2
 *   tmp = 5   /  (n8=3) = 1
 *   i   = 1  mod (n7=2) = 1
 *      ...
 **/
template<
    typename T_Vector     = std::vector< unsigned int >,
    typename T_UintLinear = size_t
>
inline T_Vector convertLinearToVectorIndex
(
    T_UintLinear         rLinIndex,
    T_Vector     const & rnSize
)
{
    /* sanity checks for input parameters */
    #ifndef NDEBUG
        T_UintLinear maxRange = 1;
        for ( auto const & nDimI : rnSize )
        {
            assert( nDimI > 0 );
            maxRange *= nDimI;
        }
        assert( rLinIndex < maxRange );
    #endif

    T_Vector vecIndex( rnSize.size() );
    for ( long int i = (long int) rnSize.size() - 1; i >= 0; --i )
    {
        vecIndex[i]  = rLinIndex % rnSize[i];
        rLinIndex   /= rnSize[i];
    }
    assert( rLinIndex == 0 );

    return vecIndex;
}


namespace tests
{


    std::ostream & operator<<
    (
        std::ostream & rOut,
        const std::vector<unsigned> rVectorToPrint
    )
    {
        rOut << "{";
        for ( const auto & elem : rVectorToPrint )
            rOut << elem << " ";
        rOut << "}";
        return rOut;
    }

    bool testVectorIndex( void )
    {
        #ifndef NDEBUG
        using Vec = std::vector< unsigned int >;

        /* 1-D tests */
        unsigned int constexpr iMax = 10;
        for ( unsigned int i = 0u; i < iMax; ++i )
        {
            Vec vecIndex = {i};
            unsigned lini = convertVectorToLinearIndex( vecIndex, Vec{iMax} );
            assert( lini == convertVectorToLinearIndex( vecIndex, Vec{iMax+5} ) );
            assert( vecIndex == convertLinearToVectorIndex( lini, Vec{iMax} ) );
            assert( vecIndex == convertLinearToVectorIndex( lini, Vec{iMax+5} ) );
        }

        /* N-D tests */
        std::vector< std::pair< unsigned, std::pair<Vec,Vec> > > testValues =
        {
            /* linear index, {dimension size, vector index} */
            { 0, { {1,1},{0,0} } },
            { 0, { {1,3},{0,0} } },
            { 0, { {2,3},{0,0} } },

            { 0, { {1,1,1},{0,0,0} } },
            { 0, { {1,3,1},{0,0,0} } },
            { 0, { {1,3,6},{0,0,0} } },
            { 0, { {2,3,1},{0,0,0} } },

            { 2, { {1,3,1},{0,2,0} } },
            { 2, { {1,3,6},{0,0,2} } },
            { 2, { {2,3,1},{0,2,0} } },

            { 3, { {1,3,6},{0,0,3} } },
            { 3, { {2,3,1},{1,0,0} } },

            { 5, { {1,3,6},{0,0,5} } },
            { 5, { {2,3,1},{1,2,0} } },

            { 8, { {1,3,6},{0,1,2} } },

            { 2, { {1,3},{0,2} } },

            { 2, { {2,3},{0,2} } },
            { 3, { {2,3},{1,0} } },
            { 4, { {2,3},{1,1} } },
            { 5, { {2,3},{1,2} } },

            { 0, { {5,3},{0,0} } },
            { 1, { {5,3},{0,1} } },
            { 2, { {5,3},{0,2} } },
            { 3, { {5,3},{1,0} } },
            { 4, { {5,3},{1,1} } },
            { 5, { {5,3},{1,2} } },
            { 6, { {5,3},{2,0} } },
            { 7, { {5,3},{2,1} } },
            { 8, { {5,3},{2,2} } }
        };

        for ( auto const & value : testValues )
        {
            /*
            std::cout << "test lini=" << value.first
                << " =? " << convertVectorToLinearIndex( value.second.second,
                                                value.second.first )
                << " (vecIndex = " << value.second.second
                << ", dimSize = " << value.second.first << ")"
                << "\n" << std::flush;
            */
            assert( value.first ==
                    convertVectorToLinearIndex( value.second.second,
                                                value.second.first ) );
            assert( value.second.second ==
                    convertLinearToVectorIndex( value.first,
                                                value.second.first ) );
        }
        #endif
        return true;
    }


} // namespace tests
