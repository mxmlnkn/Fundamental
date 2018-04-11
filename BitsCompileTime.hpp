/*
g++ -x c++ -Wall -Wextra -O3 -std=c++11 BitsCompileTime.hpp -DMAIN_TEST_BITSCOMPILETIME && ./a.out
*/
#pragma once

#include <climits>                  // CHAR_BIT
#include <limits>                   // std::numeric_limits



namespace CompileTimeFunctions {


template< typename T, typename S >
inline T constexpr ceilDiv ( T a, S b )
{
    return ( a + b - T(1) ) / b;
}

template< typename T >
inline bool constexpr isPowerOfTwo( T const x )
{
    return ! ( x == T(0) ) && ! ( x & ( x - T(1) ) );
}

template< long long int B, unsigned char E >
struct Pow { static auto constexpr value = B * Pow<B,E-1>::value; };
template< long long int B >
struct Pow<B,0> { static auto constexpr value = 1; };

/**
 * Returns floor( log_B(X) )
 */
template< unsigned long long int B, unsigned long long int X >
struct FloorLog {
    static_assert( B != 0, "" );
    static auto constexpr value = 1 + FloorLog< B, X/B >::value;
};
template< unsigned long long int B >
struct FloorLog<B,0> { static auto constexpr value = 0; };
template< unsigned long long int B >
struct FloorLog<B,1> { static auto constexpr value = 0; };

/**
 * Returns ceil( log_B(X) )
 */
template< unsigned long long int B, unsigned long long int X >
struct CeilLog {
    static_assert( B != 0, "" );
    static auto constexpr value = 1 + CeilLog< B, ceilDiv(X,B) >::value;
};
template< unsigned long long int B >
struct CeilLog<B,0> { static auto constexpr value = 0; };
template< unsigned long long int B >
struct CeilLog<B,1> { static auto constexpr value = 0; };



} // CompileTimeFunctions


/**
 * Provides some compile-time bit patterns.
 * Longest supported patterns are 64-Bit currently.
 */
namespace BitPatterns {


using Longest = unsigned long long int;
using NBits   = unsigned char; /**< type for storing amounts bounded by bits of Longest */
auto constexpr NBitsMax = std::numeric_limits< NBits >::max();
static_assert( sizeof( Longest ) * CHAR_BIT <= NBitsMax, "" );

/**
 * Returns bit step function, i.e. N 1s to the right like 0b0000111
 */
template< typename T, NBits N > struct Step;
template< typename T > struct Step<T,0> { static auto constexpr value = 0; };
template< typename T > struct Step<T,1> { static auto constexpr value = 1; };
template< typename T, NBits N > struct Step {
    static T constexpr value = T( Step<T,N-1>::value << 1 ) | T(1);
};


/**
 * Returns rectangular wave bit pattern in largest available type,
 * e.g. 00001110000111 for (L,M,N)=(3,4,2). Everthing left to it is
 * filled with zeros therefore degenerating to a step function for M=0
 * With only the type given, all available bits will be filled with the pattern
 *
 * @tparam T type to return
 * @tparam L length of rectangles (number of 1s)
 * @tparam M spacing between the rectangles (number of 0s)
 * @tparam N number of rectangles
 */
template<
    typename T,
    NBits L,
    NBits M = L,
    NBits N = ( L+M == 0 ? 0 : CompileTimeFunctions::ceilDiv( sizeof(T) * CHAR_BIT, L+M ) )
>
struct RectangularWave
{
    static Longest constexpr value = ( RectangularWave<T,L,M,N-1>::value << (L+M) )
                                     | RectangularWave<T,L,M,1>::value;
};
template< typename T, NBits L, NBits M > struct RectangularWave<T,L,M,0> { /* no rectangles */
    static Longest constexpr value = 0;
};
template< typename T, NBits L, NBits M > struct RectangularWave<T,L,M,1> { /* one rectangle */
    static Longest constexpr value = Step<T,L>::value;
};

/* some aliases */

template< typename T, NBits N > struct Ones { static T constexpr value = Step<T,N>::value; };


} // namespace BitPatterns


#include <bitset>
#include <cassert>
#include <iomanip>
#include <iostream>


namespace BitFunctions {


/**
 * Introduces 0 bits at equal intervals, i.e. 0b111 becomes 0b10101 for N=1
 * or 0b1001001 for N=2
 *
 * @tparam N Number of padding spacing zeros to introduce, N=0 returns
 *           the input number unchanged.
 *           unsigned char limits this function's usability to 256-bit types
 */
template< typename T, unsigned char N >
T inline diluteBitsNaive( T const & x )
{
    auto result = T(0);
    auto nShift = (unsigned char)(0);
    while ( x != T(0) )
    {
        result |= ( x & T(1) ) << nShift;
        nShift += N+1;
        x >>= 1;
    }
    return result;
}

/**
 * Interleave bits using a recursive approach with "magic" numbers
 * appearing after loop unrolling
 *
 * @see http://graphics.stanford.edu/~seander/bithacks.html
 *
 * Let's think this through starting with the smallest cases:
 * 1 Bit: do nothing
 * 2 Bits: ( ( x << 1 ) & 0b01 ) | ( x & 0b10 )
 * What we already can see from thinking this through is that bit i needs
 * to be shifted i*Spacing bits to the left.
 *
 * Also we might already begin to formulate the idea, that some bits can be
 * shifted in groups. E.g. bits 8 and higher can all be shifted 8*nSpacing
 * bits to the left in one go. Then in the next step we only need to shift
 * some of the bits just a bit more.
 *     0000 0000 1111 1111  (a)
 * mask out the right half
 *               1111 0000  mask1 (left half does not matter)
 *     0000 0000 1111 0000  (b) = (a) & mask1
 * shift half of the bits left as far as minimum we have to
 *     0000 1111 0000 0000  (c) = (b) << 4
 * temporarily store these into the original array by using 'or'
 *               0000 1111  mask2 = ~mask1
 *     0000 1111 0000 1111  (d) = (a) = ( (b) & mask2 ) | (c)
 *
 * Next iteration:
 * Mask out right half of each subvector of length 8 bits now left by bits/2*nSpacing
 *          1100      1100  mask3
 *     0000 1100 0000 1100  (e) = (d) & mask3
 * Move half the left halves to the left (half the step amount as in last iteration)
 *     0011 0000 0011 0000  (f) = (e) << 2
 * and "store" them back into with what we started with except for the moved
 * bits being masked out.
 *          0011      0011  mask4 = ~mask3
 *     0011 0011 0011 0011  (f) = ( (d) & mask4 ) | (f)
 * And so on. This step in total:
 *     n = (f) = ( n & ~mask3 ) | ( ( n & mask3 ) << 2 )
 * With mask3 = RectangularWave< short, 2, 4, 2 > or we could use
 * RectangularWave< short, 2, 2, 4 > which us equal to
 * RectangularWave< short, 2 > = 0x3333, because half of the bits don't matter.
 *
 * As we can see, this would result in a different routine than in the link
 * above, but we are close. We also see, that some of the mask's bits don't
 * matter.
 * The above algorithm doesn't have to "clean" the spaces inbetween, with a
 * last and'ed mask, because it cleans everything beforehand. But this makes
 * this algorithm slightly less intensive.
 * Note that ( n & mask3 ) << 2 = ( n << 2 ) & mask3 only for this case,
 * because mask3 can be made periodic with wavelength 2 (0x3333 instead of
 * 0x0303)!
 *     n = ( n & ~mask3 ) | ( ( n << 2 ) & mask3 )
 * Also note that the masking in (f) and (e) is useless in ALL cases, not just
 * the example data chosen (chose only 1s, could have some more 0s, but not more
 * 1s ...), therefore we might save one mask. ...
 *
 * E.g with the mask reordering discussed the first steps would become:
 *     0000 0000 1111 1111  (a)
 * shift half of the bits left as far as minimum we have to
 *     0000 1111 1111 0000  (b) = (a) << 4
 * temporarily store these into the original array by using 'or'
 *     0000 1111 1111 1111  (c) = (b) | (a)
 * Do some house cleaning now. Note that we could 'or' this, becase the left
 * bits are known to be 0.
 *          1111 0000 1111  mask1'
 *     0000 1111 0000 1111  (d) = (c) & mask1'

 * => p := 2**( nSteps-1 - iStep )
 *    n = ( n | ( n << p ) ) & RectangularWave< short, p >
 * This recursive step is a kinda of half and move or a kind of crumbling
 * and neither is it a recursive interleaving nor would it need the same
 * amount of template parameters, because, because quitting templates at
 * type-depending parameters would be a hassle.
 *
 * For different dilutions we need to adjust some things:
 *  - in above example we had nSpacing = 1
 *  - each bit needs to be shifted iBit * nSpacing
 *    -> in first step we can therefore move half of it by nBits/2 * nSpacing
 *  - in general we need to multiply the shifts by nSpacing.
 *    -> for part1by2 is nSpacing = 2, meaning shifts are 16,8,4,2 instead of
 *       8,4,2,1 and might mistakenly look very similar! For nSpacing=3
 *       we have 24,12,6,3
 *  - The masks need to be adjusted to have a 0-spacing of iStep * nSpacing
 *    with unchanged length for the rectangles (???)
 *  - the number of steeps needed is always the same, i.e. log2( nInputBits )
 *  - the number of valid bits is sizeof(T)/nSpacing, because 1 bit more would
 *    be outside the bit range
 *  - the masks are of the form of rectangular waves of length 8,4,2,1 with
 *    spacings nSpacing*(8,4,2,1)
 */

template< typename T, unsigned char nSpacing, unsigned char nStepsNeeded, unsigned char iStep >
struct DiluteBitsCrumble { inline static T apply( T const & xLastStep )
{
    auto x = DiluteBitsCrumble<T,nSpacing,nStepsNeeded,iStep-1>::apply( xLastStep );
    #ifdef PRINT_DILUTEBITSCRUMBLE_STEPS
        auto const x0 = x;
    #endif
    auto constexpr iStep2Pow = 1llu << ( (nStepsNeeded-1) - iStep );
    auto constexpr mask = BitPatterns::RectangularWave< T, iStep2Pow, iStep2Pow * nSpacing >::value;
    x = ( x | ( x << ( iStep2Pow * nSpacing ) ) ) & mask;
    #ifdef PRINT_DILUTEBITSCRUMBLE_STEPS
        std::cout
        << "step " << (int) iStep << ": ( x=" << std::bitset< sizeof(T) * CHAR_BIT >( x0 )
        << ", x | ( x << " << std::setw(2) << iStep2Pow * nSpacing << " ) ) & "
        << std::bitset< sizeof(T) * CHAR_BIT >( mask ) << " -> "
        << std::bitset< sizeof(T) * CHAR_BIT >( x ) << "\n";
    #endif
    return x;
} };

template< typename T, unsigned char nSpacing, unsigned char nStepsNeeded >
struct DiluteBitsCrumble<T,nSpacing,nStepsNeeded,0> { inline static T apply( T const & x )
{
    auto constexpr nBitsAllowed = 1 + ( sizeof(T) * CHAR_BIT - 1 ) / ( nSpacing + 1 );
    //auto constexpr nBitsAllowed = sizeof(T) * CHAR_BIT / ( nSpacing + 1 );
    return x & BitPatterns::Ones< T, nBitsAllowed >::value;
} };


/**
 * Interleaves bits with specified amount N of 0-Bits.
 * Should be called * with the input casted to the expected output data type.
 *
 * @tparam N the spacing, i.e. 0 returns identity and 1 interleaves one 0
 *           and N=2 two 0s for each input bit.
 */
template< typename T, unsigned char nSpacing >
T inline diluteBitsRecursive( T const & rx )
{
    static_assert( nSpacing > 0, "" );
    /**
     * We are filling the zeros to the left, therefore we always can allow 1 bit
     * e.g. 000100010001 ... If we could rewrite the function to fill the 0s to
     * the right we could possibly save two shifts for interleaving in 3D!
     * Allowed bits for 1 bit in T: 1, 2 bits in T: 2 for nSpacing = 0,
     * 3 bits in T: 3 for nSpacing = 0, 2 for nSpacing=1, else 1, i.e. we need
     * (nBitsAllowed-1)*(nSpacing+1) <= nBits(T)-1
     *   => nBitsAllowed = 1+floor[( sizeof(T)*CHAR_BIT-1 )/( nSpacing+1 )]
     * nSpacing = 1, Bits 0..8: 1+(-1/2), 1, 1+(2-1)/2=1, 2, 2, 3, 3, 4, 4 for 8 bits
     * nSpacing = 2, Bits 0..8:         , 1, 1+(2-1)/3=1, 1, 2, 2, 2, 3, 3
     *   32 Bits => 1+31/3 = 11, for some reasong part1by2 masks 10, I guess
     *   when thinking about the interleaving later on it makes sense to require
     *   all zeros too actually fit in the result
     *   => looks correct now ...
     * Requiring all 0s to fit in, would simplify the calculation to
     *   nBitsAllowed2 = nBits/(nSpacing+1) which yields 10 for 32
     */
    auto constexpr nBitsAvailable = sizeof(T) * CHAR_BIT;
    static_assert( nBitsAvailable > 0, "" );
    /**
     * For example for char, i.e. nBitsAvailable = 8:
     * Expectation for nSpacing = 4: 0000 0111 -> 0010 0001
     * => nAllowed = 2 expected. How to calculate:
     * @verbatim
     *    0010 0001
     *      +-----+
     * @endverbatim
     * the 1 travels nSpacing to the left from its original position at 1,
     * therefore the max. travel distance is nSpacing * nAllowed, but the
     * max. position(!) / bits needed is (nSpacing+1) * nAllowed.
     * It's important to differentiate the two.
     *     nTravelMax = nSpacing * nAllowed
     * From the max. position / bits needed we can calculate the amount of
     * allowed bist:
     *     nAllowed = nBitsAvailable / ( nSpacing + 1 )
     * Fractions of bits don't make sense and we could round down, but the
     * rightmost bit is always allowed for nBitsAvailable > 0 and can
     * therefore be interpreted as the fractional part, therefore we need to
     * round up!
     *    nAllowed = ceil( nBitsAvailable / ( nSpacing + 1 ) )
     * The traveling, i.e. shifting in turn is done recursively which
     * basically boils down to redaing the binary representation of nTravel
     * and applying it. But as we always shift in multiples of nSpacing,
     * the actual binary representation which is important is the one for
     * nTravelMax / nSpacing, therefore the max. number of shifts to be
     * done is the amount of bits needed to represent that, which is:
     *    nShifts = ceil( log2( nTravelMax / nSpacing ) )
     *    nShifts = ceil( log2( nAllowed ) )
     * nStepsNeeded = 1 means that only one masking is done and no shifting,
     * therefore:
     *    nStepsNeeded = nShifts + 1
     */
    auto constexpr nBitsAllowed = CompileTimeFunctions::ceilDiv( nBitsAvailable, nSpacing + 1 );
    auto constexpr nStepsNeeded = 1 + CompileTimeFunctions::CeilLog< 2, nBitsAllowed >::value;
    /* else the result is trivial as only the last bit is kept and therefore
     * maybe unexpected for the user */
    //static_assert( nStepsNeeded > 0, "" );
    /*
    std::cout
    << "Dilute input " << (int)rx << " of size " << sizeof(T) << " Byte by " << (int) nSpacing << ":\n"
    << "  nBitsAvailable    = " << nBitsAvailable << "\n"
    << "  nBitsAllowed      = " << nBitsAllowed   << "\n"
    << "  nMaxTravel        = " << nBitsAllowed * nSpacing << "\n"
    << "  nBitsForMaxTravel = " << CompileTimeFunctions::CeilLog< 2, nBitsAllowed >::value << "\n"
    << "  nStepsNeeded      = " << nStepsNeeded   << "\n";
    */

    /**
     * for 32 Bit and nSpacing=1  this should expand to:
     *   n &= 0x0000ffff;
     *   n = (n | (n << 8)) & 0x00FF00FF; // 0b 0000 0000 1111 1111
     *   n = (n | (n << 4)) & 0x0F0F0F0F; // 0b 0000 1111 0000 1111
     *   n = (n | (n << 2)) & 0x33333333; // 0b 0011 0011 0011 0011
     *   n = (n | (n << 1)) & 0x55555555; // 0b 0101 0101 0101 0101
     * for 32 Bit and nSpacing=1  this should expand to:
     *   n&= 0x000003ff;
     *   n = (n ^ (n << 16)) & 0xFF0000FF; // 0b 0000 0000 1111 1111
     *   n = (n ^ (n <<  8)) & 0x0300F00F; // 0b 1111 0000 0000 1111
     *   n = (n ^ (n <<  4)) & 0x030C30C3; // 0b 0011 0000 1100 0011
     *   n = (n ^ (n <<  2)) & 0x09249249; // 0b 1001 0010 0100 1001
     */
#if 0
    /* This first steps just sanitizes the input e.g. if the number was too high */
    auto x = rx & BitPatterns::Ones< T, nBitsAllowed >::value;
    assert( rx == x && "Input argument is out of range!" );
    for ( unsigned short iStep2Pow = CompileTimeFunctions::Pow<2,nStepsNeeded-1>;
          iStep2Pow > 0; iStep2Pow /= 2;
    {
        x = ( x | ( x << ( iStep2Pow * (nSpacing+1) ) ) )
          & BitPatterns::RectangularWave< T, iStep2Pow, iStep2Pow * nSpacing >::value;
    }
    /* unfortunately we HAVE to unroll the loop using templates, because we
     * can't use runtime variables as template parameters -.-" */
    return x;
#endif

    return DiluteBitsCrumble< T, nSpacing, nStepsNeeded, ( nStepsNeeded > 0 ? nStepsNeeded-1 : 0 ) >::apply( rx );
}


} // namespace BitFunctions




#ifdef MAIN_TEST_BITSCOMPILETIME


#include <bitset>
#include <iomanip>
#include <iostream>


unsigned int part1by1(unsigned int n)
{
	n &= 0x0000ffff;
	n = (n | (n << 8)) & 0x00FF00FF /* 0b 0000 0000 1111 1111 */;
	n = (n | (n << 4)) & 0x0F0F0F0F /* 0b 0000 1111 0000 1111 */;
	n = (n | (n << 2)) & 0x33333333 /* 0b 0011 0011 0011 0011 */;
	n = (n | (n << 1)) & 0x55555555 /* 0b 0101 0101 0101 0101 */;
	return n;
}



unsigned int part1by2(unsigned int n)
{
    n&= 0x000003ff;
    n = (n ^ (n << 16)) & 0xFF0000FF /* 0b 0000 0000 1111 1111 */;
    n = (n ^ (n <<  8)) & 0x0300F00F /* 0b 1111 0000 0000 1111 */;
    n = (n ^ (n <<  4)) & 0x030C30C3 /* 0b 0011 0000 1100 0011 */;
    n = (n ^ (n <<  2)) & 0x09249249 /* 0b 1001 0010 0100 1001 */;
    return n;
}

uint32_t unpart1by1(uint32_t n)
{
	n&= 0x55555555;
	n = (n ^ (n >> 1)) & 0x33333333;
	n = (n ^ (n >> 2)) & 0x0f0f0f0f;
	n = (n ^ (n >> 4)) & 0x00ff00ff;
	n = (n ^ (n >> 8)) & 0x0000ffff;
	return n;
}

uint32_t unpart1by2(uint32_t n)
{
    n&= 0x09249249;
    n = (n ^ (n >>  2)) & 0x030c30c3;
    n = (n ^ (n >>  4)) & 0x0300f00f;
    n = (n ^ (n >>  8)) & 0xff0000ff;
    n = (n ^ (n >> 16)) & 0x000003ff;
    return n;
}

uint32_t interleave3(uint32_t x, uint32_t  y, uint32_t z)
{
	return part1by2(x) | (part1by2(y) << 1) | (part1by2(z) << 2);
}


uint32_t deinterleave3_X(uint32_t n)
{
    return unpart1by2(n);
}

uint32_t deinterleave3_Y(uint32_t n)
{
    return unpart1by2(n >> 1);
}

uint32_t deinterleave3_Z(uint32_t n)
{
    return unpart1by2(n >> 2);
}


template< typename T > void testRectangularWaves( void )
{
    #define TMP(L)                                                          \
    {                                                                       \
        auto const x = BitPatterns::RectangularWave<T,L>::value;            \
        std::cout                                                           \
            << std::bitset< sizeof(T) * CHAR_BIT >(x) << " = 0x"            \
            << std::setfill('0') << std::setw( sizeof(T) * CHAR_BIT / 2 )   \
            << std::hex << x << std::dec << std::setfill(' ') << "\n";      \
    }
    /* xD https://github.com/pfultz2/Cloak/wiki/C-Preprocessor-tricks,-tips,-and-idioms */
    TMP(1) TMP(2) TMP(4) TMP(5) TMP(8) TMP(16) TMP(32)
    #undef TMP
}

#include <cstdlib>                      // rand, srand, ...

template< typename T > void testDilution( void )
{
    #define TMP(X,L)                                                        \
    {                                                                       \
        auto const x = BitFunctions::diluteBitsRecursive<T,L>(X);           \
        std::cout                                                           \
            << std::bitset< sizeof(T) * CHAR_BIT >(X) << " => 0x"           \
            << std::bitset< sizeof(T) * CHAR_BIT >(x) << "\n";              \
    }
    TMP( ~T(0), 1 ) TMP( T( 2835127451ull ), 1 )
    TMP( ~T(0), 2 ) TMP( T( 2835127451ull ), 2 )
    TMP( ~T(0), 3 ) TMP( T( 2835127451ull ), 3 )
    TMP( ~T(0), 4 ) TMP( T( 2835127451ull ), 4 )
    #undef TMP
}

#include <chrono>

void testDilution2( void )
{
    using T = unsigned int;

    /* check against part1by1 */
    std::cout << std::setfill('0') << std::hex;
    for ( auto i = 0u; i < 10; ++i )
    {
        auto const x = std::rand();
        auto const x1 = x & 0xFFFFul;
        auto const x2 = x & 0x03FFul;
        auto const y1 = part1by1( x );
        auto const y2 = BitFunctions::diluteBitsRecursive< T, 1 >( x );
        auto const z1 = part1by2( x );
        auto const z2 = BitFunctions::diluteBitsRecursive< T, 2 >( x );
        std::cout
        << "0x" << std::setw( sizeof(T) * 2 ) << x1 << " -> "
        << "0x" << std::setw( sizeof(T) * 2 ) << y1 << " =? "
        << "0x" << std::setw( sizeof(T) * 2 ) << y2 << ( y1 != y2 ? " FAILED" : " OK" ) << "\n"
        << "0x" << std::setw( sizeof(T) * 2 ) << x2 << " -> "
        << "0x" << std::setw( sizeof(T) * 2 ) << z1 << " =? "
        << "0x" << std::setw( sizeof(T) * 2 ) << z2 << ( y1 != y2 ? " FAILED" : " OK" ) << "\n";
    }
    std::cout << std::setfill(' ') << std::dec;

    auto const nIterations = 1234567890lu;
    std::srand( std::chrono::duration_cast< std::chrono::duration< double > >(
                std::chrono::high_resolution_clock::now().time_since_epoch() ).count() );
    auto result = std::rand();

    /* benchmark part1by1 */
    auto const ta0 = std::chrono::high_resolution_clock::now();
    for ( auto i = 0lu; i < nIterations; ++i )
        result ^= part1by1( result ) | 0x12345;
    auto const ta1 = std::chrono::high_resolution_clock::now();
    std::cout
    << nIterations <<  " using part1by1 (OLD) took "
    << std::chrono::duration_cast< std::chrono::duration< double > >( ta1 - ta0 ).count()
    << "s\n";

    /* benchmark part1by1 using template function */
    auto const tb0 = std::chrono::high_resolution_clock::now();
    for ( auto i = 0lu; i < nIterations; ++i )
        result ^= BitFunctions::diluteBitsRecursive< T, 1 >( result ) | 0x12345;
    auto const tb1 = std::chrono::high_resolution_clock::now();
    std::cout
    << nIterations <<  " using part1by1 (NEW) took "
    << std::chrono::duration_cast< std::chrono::duration< double > >( tb1 - tb0 ).count()
    << "s\n";

    /* benchmark part1by2 */
    auto const tc0 = std::chrono::high_resolution_clock::now();
    for ( auto i = 0lu; i < nIterations; ++i )
        result ^= part1by2( result ) | 0x12345;
    auto const tc1 = std::chrono::high_resolution_clock::now();
    std::cout
    << nIterations <<  " using part1by2 (OLD) took "
    << std::chrono::duration_cast< std::chrono::duration< double > >( tc1 - tc0 ).count()
    << "s\n";

    /* benchmark part1by2 using template function */
    auto const td0 = std::chrono::high_resolution_clock::now();
    for ( auto i = 0lu; i < nIterations; ++i )
        result ^= BitFunctions::diluteBitsRecursive< T, 2 >( result ) | 0x12345;
    auto const td1 = std::chrono::high_resolution_clock::now();
    std::cout
    << nIterations <<  " using part1by2 (NEW) took "
    << std::chrono::duration_cast< std::chrono::duration< double > >( td1 - td0 ).count()
    << "s\n";

    std::cout << "(result = " << result << ")\n";

    /**
     * for GPP in g++-5 g++-6; do echo "$GPP:"; for flag in '   ' -O0 -O1 -O2 -O3; do echo "  $flag:"; $GPP -x c++ $flag -Wall -Wextra -std=c++11 BitsCompileTime.hpp -DMAIN_TEST_BITSCOMPILETIME 2>/dev/null && ./a.out 2>&1 | sed -nr '/ took /p'; done; done
     * @verbatim
     * g++-5:
     *   No Flags:
     *     1234567890 using part1by1 (OLD) took 19.1245s
     *     1234567890 using part1by1 (NEW) took 30.1481s
     *     1234567890 using part1by2 (OLD) took 19.1973s
     *     1234567890 using part1by2 (NEW) took 29.8871s
     *   -O0:
     *     1234567890 using part1by1 (OLD) took 19.2118s
     *     1234567890 using part1by1 (NEW) took 29.5715s
     *     1234567890 using part1by2 (OLD) took 19.1993s
     *     1234567890 using part1by2 (NEW) took 29.7324s
     *   -O1:
     *     1234567890 using part1by1 (OLD) took 5.66236s
     *     1234567890 using part1by1 (NEW) took 5.62838s
     *     1234567890 using part1by2 (OLD) took 5.65219s
     *     1234567890 using part1by2 (NEW) took 5.63415s
     *   -O2:
     *     1234567890 using part1by1 (OLD) took 5.66398s
     *     1234567890 using part1by1 (NEW) took 5.63201s
     *     1234567890 using part1by2 (OLD) took 5.64173s
     *     1234567890 using part1by2 (NEW) took 5.66007s
     *   -O3:
     *     1234567890 using part1by1 (OLD) took 5.68837s
     *     1234567890 using part1by1 (NEW) took 5.64835s
     *     1234567890 using part1by2 (OLD) took 5.64818s
     *     1234567890 using part1by2 (NEW) took 5.69607s
     * g++-6:
     *   No Flags:
     *     1234567890 using part1by1 (OLD) took 19.2545s
     *     1234567890 using part1by1 (NEW) took 29.9512s
     *     1234567890 using part1by2 (OLD) took 19.1396s
     *     1234567890 using part1by2 (NEW) took 29.6569s
     *   -O0:
     *     1234567890 using part1by1 (OLD) took 19.0805s
     *     1234567890 using part1by1 (NEW) took 29.5035s
     *     1234567890 using part1by2 (OLD) took 19.1712s
     *     1234567890 using part1by2 (NEW) took 29.5699s
     *   -O1:
     *     1234567890 using part1by1 (OLD) took 5.63171s
     *     1234567890 using part1by1 (NEW) took 5.63261s
     *     1234567890 using part1by2 (OLD) took 5.65188s
     *     1234567890 using part1by2 (NEW) took 6.12264s -> SLOWER than g++5
     *   -O2:
     *     1234567890 using part1by1 (OLD) took 5.67073s
     *     1234567890 using part1by1 (NEW) took 5.66181s
     *     1234567890 using part1by2 (OLD) took 6.1471s  -> SLOWER than -O1
     *     1234567890 using part1by2 (NEW) took 6.13563s
     *   -O3:
     *     1234567890 using part1by1 (OLD) took 5.7056s
     *     1234567890 using part1by1 (NEW) took 5.65694s
     *     1234567890 using part1by2 (OLD) took 6.13513s
     *     1234567890 using part1by2 (NEW) took 6.13791s
     * @endverbatim
     * Each of the above results were run 3 times to ensure we have not just
     * an outlier for whatever reason
     * => The inline keyword or -finline-small-functions as included in -O
     *    slows down the dilute by two bits for whatever reason since g++6 -.-
     * @see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84327
     * @see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84328
     *
     * Unfortunately it seems I won't ever find out where the 3x speedup
     * comes from, as even using -fno-optimization for all listed optimizations
     * is still 5.6s fast :/
     *
     * antiO1Flags=$( g++ -O1 -Q --help=optimizers 2>/dev/null | sed -nr 's| -f|-fno-|; s|[ \t]*\[disabled\]||p;' | sed '/stack-protector/d' )
     * g++ -x c++ -Wall -Wextra -O1 $antiO1Flags -std=c++11 BitsCompileTime.hpp -DMAIN_TEST_BITSCOMPILETIME && ./a.out
     *  1234567890 using part1by1 (OLD) took 5.64657s
     *  1234567890 using part1by1 (NEW) took 5.6359s
     *  1234567890 using part1by2 (OLD) took 5.66952s
     *  1234567890 using part1by2 (NEW) took 6.23457s
     *
     * The 'stack-protector' filter seems to be needed, because it can't be turned off:
     *   g++: error: unrecognized command line option ‘-fno-stack-protector-all’; did you mean ‘-fstack-protector-all’?
     *   g++: error: unrecognized command line option ‘-fno-stack-protector-explicit’; did you mean ‘-fstack-protector-explicit’?
     *   g++: error: unrecognized command line option ‘-fno-stack-protector-strong’; did you mean ‘-fstack-protector-strong’?
     */
}

template< typename T > void testLog( void )
{
    #define TMP(B,X) std::cout << X << "->" << CompileTimeFunctions::CeilLog<B,X>::value << ", ";
    std::cout << "ceil (Log2): ";
    TMP(2,1) TMP(2,2) TMP(2,3) TMP(2,4) TMP(2,5) TMP(2,6) TMP(2,7) TMP(2,8) TMP(2,16) TMP(2,31) TMP(2,32) TMP(2,33)
    std::cout << "\n";
    #undef TMP
    #define TMP(B,X) std::cout << X << "->" << CompileTimeFunctions::FloorLog<B,X>::value << ", ";
    std::cout << "floor(Log2): ";
    TMP(2,1) TMP(2,2) TMP(2,3) TMP(2,4) TMP(2,5) TMP(2,6) TMP(2,7) TMP(2,8) TMP(2,16) TMP(2,31) TMP(2,32) TMP(2,33)
    std::cout << "\n";
    #undef TMP
    #define TMP(B,X) std::cout << X << "->" << CompileTimeFunctions::CeilLog<B,X>::value << ", ";
    std::cout << "ceil (Log3): ";
    TMP(3,1) TMP(3,2) TMP(3,3) TMP(3,4) TMP(3,5) TMP(3,6) TMP(3,7) TMP(3,8) TMP(3,9) TMP(3,26) TMP(3,27) TMP(3,28)
    std::cout << "\n";
    #undef TMP
    #define TMP(B,X) std::cout << X << "->" << CompileTimeFunctions::FloorLog<B,X>::value << ", ";
    std::cout << "floor(Log3): ";
    TMP(3,1) TMP(3,2) TMP(3,3) TMP(3,4) TMP(3,5) TMP(3,6) TMP(3,7) TMP(3,8) TMP(3,9) TMP(3,26) TMP(3,27) TMP(3,28)
    std::cout << "\n";
    #undef TMP
}

int main()
{
    std::cout << "== Rectangular Waves for unsigned char ==\n";
    testRectangularWaves< unsigned char  >();
    std::cout << "== Rectangular Waves for unsigned short ==\n";
    testRectangularWaves< unsigned short >();
    std::cout << "== Rectangular Waves for unsigned int ==\n";
    testRectangularWaves< unsigned int   >();
    std::cout << "== Rectangular Waves for unsigned long ==\n";
    testRectangularWaves< unsigned long  >();

    std::cout << "== Log for unsigned char ==\n";
    testLog< unsigned char >();
    std::cout << "== Log for unsigned short ==\n";
    testLog< unsigned short>();
    std::cout << "== Log for unsigned int ==\n";
    testLog< unsigned int  >();
    std::cout << "== Log for unsigned long ==\n";
    testLog< unsigned long >();

    std::cout << "== Bit Dilution for unsigned char ==\n";
    testDilution< unsigned char >();
    std::cout << "== Bit Dilution for unsigned short ==\n";
    testDilution< unsigned short>();
    std::cout << "== Bit Dilution for unsigned int ==\n";
    testDilution< unsigned int  >();
    std::cout << "== Bit Dilution for unsigned long ==\n";
    testDilution< unsigned long >();

    testDilution2();
}


#endif
