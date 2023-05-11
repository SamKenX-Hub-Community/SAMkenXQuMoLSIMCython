/*
* Simd Library (http://ermig1979.github.io/Simd).
*
* Copyright (c) 2011-2022 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdMemory.h"
#include "Simd/SimdStore.h"
#include "Simd/SimdExtract.h"
#include "Simd/SimdArray.h"
#include "Simd/SimdUnpack.h"
#include "Simd/SimdDescrInt.h"
#include "Simd/SimdDescrIntCommon.h"
#include "Simd/SimdCpu.h"

namespace Simd
{
#ifdef SIMD_AVX2_ENABLE    
    namespace Avx2
    {
        static void MinMax(const float* src, size_t size, float& min, float& max)
        {
            size_t sizeF = AlignLo(size, F), sizeHF = AlignLo(size, HF);
            __m256 _min256 = _mm256_set1_ps(FLT_MAX);
            __m256 _max256 = _mm256_set1_ps(-FLT_MAX);
            size_t i = 0;
            for (; i < sizeF; i += F)
            {
                __m256 _src = _mm256_loadu_ps(src + i);
                _min256 = _mm256_min_ps(_src, _min256);
                _max256 = _mm256_max_ps(_src, _max256);
            }
            __m128 _min = _mm_min_ps(_mm256_castps256_ps128(_min256), _mm256_extractf128_ps(_min256, 1));
            __m128 _max = _mm_max_ps(_mm256_castps256_ps128(_max256), _mm256_extractf128_ps(_max256, 1));
            for (; i < sizeHF; i += HF)
            {
                __m128 _src = _mm_loadu_ps(src + i);
                _min = _mm_min_ps(_src, _min);
                _max = _mm_max_ps(_src, _max);
            }
            for (; i < size; i += 1)
            {
                __m128 _src = _mm_load_ss(src + i);
                _min = _mm_min_ss(_src, _min);
                _max = _mm_max_ss(_src, _max);
            }
            _min = _mm_min_ps(_min, Sse41::Shuffle32f<0x22>(_min));
            _max = _mm_max_ps(_max, Sse41::Shuffle32f<0x22>(_max));
            _min = _mm_min_ss(_min, Sse41::Shuffle32f<0x11>(_min));
            _max = _mm_max_ss(_max, Sse41::Shuffle32f<0x11>(_max));
            _mm_store_ss(&min, _min);
            _mm_store_ss(&max, _max);
        }

        //-------------------------------------------------------------------------------------------------

        SIMD_INLINE __m256i Encode(const float* src, __m256 scale, __m256 min, __m256i& sum, __m256i& sqsum)
        {
            __m256i value = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_sub_ps(_mm256_loadu_ps(src), min), scale));
            sum = _mm256_add_epi32(value, sum);
            sqsum = _mm256_add_epi32(_mm256_madd_epi16(value, value), sqsum);
            return value;
        }

        static SIMD_INLINE __m128i Encode6x1(const float* src, __m256 scale, __m256 min, __m256i& sum, __m256i& sqsum)
        {
            static const __m128i SHIFT = SIMD_MM_SETR_EPI16(256, 64, 16, 4, 256, 64, 16, 4);
            static const __m128i SHFL0 = SIMD_MM_SETR_EPI8(0x1, 0x3, 0x5, 0x9, 0xB, 0xD, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
            static const __m128i SHFL1 = SIMD_MM_SETR_EPI8(0x2, 0x4, 0x6, 0xA, 0xC, 0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
            __m256i i0 = Encode(src + 0, scale, min, sum, sqsum);
            __m128i s0 = _mm_mullo_epi16(_mm256_castsi256_si128(PackU32ToI16(i0, _mm256_setzero_si256())), SHIFT);
            return _mm_or_si128(_mm_shuffle_epi8(s0, SHFL0), _mm_shuffle_epi8(s0, SHFL1));
        }

        static SIMD_INLINE __m128i Encode6x2(const float* src, __m256 scale, __m256 min, __m256i& sum, __m256i& sqsum)
        {
            static const __m256i SHIFT = SIMD_MM256_SETR_EPI16(256, 64, 16, 4, 256, 64, 16, 4, 256, 64, 16, 4, 256, 64, 16, 4);
            static const __m256i SHFL0 = SIMD_MM256_SETR_EPI8(
                0x1, 0x3, 0x5, 0x9, 0xB, 0xD, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, 0x1, 0x3, 0x5, 0x9, 0xB, 0xD, -1, -1, -1, -1);
            static const __m256i SHFL1 = SIMD_MM256_SETR_EPI8(
                0x2, 0x4, 0x6, 0xA, 0xC, 0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, 0x2, 0x4, 0x6, 0xA, 0xC, 0xE, -1, -1, -1, -1);
            __m256i i0 = Encode(src + 0, scale, min, sum, sqsum);
            __m256i i8 = Encode(src + 8, scale, min, sum, sqsum);
            __m256i s0 = _mm256_mullo_epi16(PackU32ToI16(i0, i8), SHIFT);
            __m256i e0 = _mm256_or_si256(_mm256_shuffle_epi8(s0, SHFL0), _mm256_shuffle_epi8(s0, SHFL1));
            return _mm_or_si128(_mm256_castsi256_si128(e0), _mm256_extracti128_si256(e0, 1));
        }

        static void Encode6(const float* src, float scale, float min, size_t size, int32_t& sum, int32_t& sqsum, uint8_t* dst)
        {
            assert(size % 8 == 0);
            size_t i = 0, main = size - 8, main16 = AlignLo(main, 16);
            __m256 _scale = _mm256_set1_ps(scale);
            __m256 _min = _mm256_set1_ps(min);
            __m256i _sum = _mm256_setzero_si256();
            __m256i _sqsum = _mm256_setzero_si256();
            for (; i < main16; i += 16, src += 16, dst += 12)
                _mm_storeu_si128((__m128i*)dst, Encode6x2(src, _scale, _min, _sum, _sqsum));
            for (; i < main; i += 8, src += 8, dst += 6)
                _mm_storel_epi64((__m128i*)dst, Encode6x1(src, _scale, _min, _sum, _sqsum));
            for (; i < size; i += 8, src += 8, dst += 6)
            {
                __m128i d0 = Encode6x1(src, _scale, _min, _sum, _sqsum);
                *(uint32_t*)(dst + 0) = _mm_extract_epi32(d0, 0);
                *(uint16_t*)(dst + 4) = _mm_extract_epi16(d0, 2);
            }
            sum = ExtractSum<uint32_t>(_sum);
            sqsum = ExtractSum<uint32_t>(_sqsum);
        }

        static SIMD_INLINE __m128i Encode7x1(const float* src, __m256 scale, __m256 min, __m256i& sum, __m256i& sqsum)
        {
            static const __m128i SHIFT = SIMD_MM_SETR_EPI16(256, 128, 64, 32, 16, 8, 4, 2);
            static const __m128i SHFL0 = SIMD_MM_SETR_EPI8(0x1, 0x3, 0x5, 0x7, 0x9, 0xB, 0xD, -1, -1, -1, -1, -1, -1, -1, -1, -1);
            static const __m128i SHFL1 = SIMD_MM_SETR_EPI8(0x2, 0x4, 0x6, 0x8, 0xA, 0xC, 0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1);
            __m256i i0 = Encode(src + 0, scale, min, sum, sqsum);
            __m128i s0 = _mm_mullo_epi16(_mm256_castsi256_si128(PackU32ToI16(i0, _mm256_setzero_si256())), SHIFT);
            return _mm_or_si128(_mm_shuffle_epi8(s0, SHFL0), _mm_shuffle_epi8(s0, SHFL1));
        }

        static SIMD_INLINE __m128i Encode7x2(const float* src, __m256 scale, __m256 min, __m256i& sum, __m256i& sqsum)
        {
            static const __m256i SHIFT = SIMD_MM256_SETR_EPI16(256, 128, 64, 32, 16, 8, 4, 2, 256, 128, 64, 32, 16, 8, 4, 2);
            static const __m256i SHFL0 = SIMD_MM256_SETR_EPI8(
                0x1, 0x3, 0x5, 0x7, 0x9, 0xB, 0xD, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, 0x1, 0x3, 0x5, 0x7, 0x9, 0xB, 0xD, -1, -1);
            static const __m256i SHFL1 = SIMD_MM256_SETR_EPI8(
                0x2, 0x4, 0x6, 0x8, 0xA, 0xC, 0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, 0x2, 0x4, 0x6, 0x8, 0xA, 0xC, 0xE, -1, -1);
            __m256i i0 = Encode(src + 0, scale, min, sum, sqsum);
            __m256i i8 = Encode(src + 8, scale, min, sum, sqsum);
            __m256i s0 = _mm256_mullo_epi16(PackU32ToI16(i0, i8), SHIFT);
            __m256i e0 = _mm256_or_si256(_mm256_shuffle_epi8(s0, SHFL0), _mm256_shuffle_epi8(s0, SHFL1));
            return _mm_or_si128(_mm256_castsi256_si128(e0), _mm256_extracti128_si256(e0, 1));
        }

        static void Encode7(const float* src, float scale, float min, size_t size, int32_t& sum, int32_t& sqsum, uint8_t* dst)
        {
            assert(size % 8 == 0);
            size_t i = 0, main = size - 8, main16 = AlignLo(main, 16);
            __m256 _scale = _mm256_set1_ps(scale);
            __m256 _min = _mm256_set1_ps(min);
            __m256i _sum = _mm256_setzero_si256();
            __m256i _sqsum = _mm256_setzero_si256();
            for (; i < main16; i += 16, src += 16, dst += 14)
                _mm_storeu_si128((__m128i*)dst, Encode7x2(src, _scale, _min, _sum, _sqsum));
            for (; i < main; i += 8, src += 8, dst += 7)
                _mm_storel_epi64((__m128i*)dst, Encode7x1(src, _scale, _min, _sum, _sqsum));
            for (; i < size; i += 8, src += 8, dst += 7)
            {
                __m128i d0 = Encode7x1(src, _scale, _min, _sum, _sqsum);
                *(uint32_t*)(dst + 0) = _mm_extract_epi32(d0, 0);
                *(uint16_t*)(dst + 4) = _mm_extract_epi16(d0, 2);
                *(uint8_t*)(dst + 6) = _mm_extract_epi8(d0, 6);
            }
            sum = ExtractSum<uint32_t>(_sum);
            sqsum = ExtractSum<uint32_t>(_sqsum);
        }

        static void Encode8(const float* src, float scale, float min, size_t size, int32_t& sum, int32_t& sqsum, uint8_t* dst)
        {
            assert(size % 8 == 0);
            size_t sizeA = AlignLo(size, A), i = 0;
            __m256 _scale = _mm256_set1_ps(scale);
            __m256 _min = _mm256_set1_ps(min);
            __m256i _sum = _mm256_setzero_si256();
            __m256i _sqsum = _mm256_setzero_si256();
            for (; i < sizeA; i += A)
            {
                __m256i d0 = Encode(src + i + 0 * F, _scale, _min, _sum, _sqsum);
                __m256i d1 = Encode(src + i + 1 * F, _scale, _min, _sum, _sqsum);
                __m256i d2 = Encode(src + i + 2 * F, _scale, _min, _sum, _sqsum);
                __m256i d3 = Encode(src + i + 3 * F, _scale, _min, _sum, _sqsum);
                _mm256_storeu_si256((__m256i*)(dst + i), PackI16ToU8(PackI32ToI16(d0, d1), PackI32ToI16(d2, d3)));
            }
            for (; i < size; i += F)
            {
                __m256i d0 = Encode(src + i, _scale, _min, _sum, _sqsum);
                _mm_storel_epi64((__m128i*)(dst + i), _mm256_castsi256_si128(PackI16ToU8(PackI32ToI16(d0, _mm256_setzero_si256()), _mm256_setzero_si256())));
            }
            sum = ExtractSum<uint32_t>(_sum);
            sqsum = ExtractSum<uint32_t>(_sqsum);
        }

        //-------------------------------------------------------------------------------------------------

        DescrInt::DescrInt(size_t size, size_t depth)
            : Sse41::DescrInt(size, depth)
        {
            _minMax = MinMax;
            //_microM = 2;
            //_microN = 4;
            switch (depth)
            {
            case 6:
            {
                _encode = Encode6;
            //    _decode = Decode6;
            //    _cosineDistance = Sse41::CosineDistance<6>;
            //    _macroCosineDistances = Sse41::MacroCosineDistances<6>;
                break;
            }
            case 7:
            {
                _encode = Encode7;
            //    _decode = Decode7;
            //    _cosineDistance = Sse41::CosineDistance<7>;
            //    _macroCosineDistances = Sse41::MacroCosineDistances<7>;
                break;
            }
            case 8:
            {
                _encode = Encode8;
            //    _decode = Decode8;
            //    _cosineDistance = Sse41::CosineDistance<8>;
            //    _macroCosineDistances = Sse41::MacroCosineDistances<8>;
                break;
            }
            default:
                assert(0);
            }
        }

        //-------------------------------------------------------------------------------------------------

        void* DescrIntInit(size_t size, size_t depth)
        {
            if (!Base::DescrInt::Valid(size, depth))
                return NULL;
            return new Avx2::DescrInt(size, depth);
        }
    }
#endif
}
