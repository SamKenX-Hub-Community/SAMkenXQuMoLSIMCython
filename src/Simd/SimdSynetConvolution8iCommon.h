/*
* Simd Library (http://ermig1979.github.io/Simd).
*
* Copyright (c) 2011-2020 Yermalayeu Ihar.
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
#ifndef __SimdSynetConvolution8iCommon_h__
#define __SimdSynetConvolution8iCommon_h__

#include "Simd/SimdMath.h"
#include "Simd/SimdStore.h"
#include "Simd/SimdSynet.h"
#include "Simd/SimdExp.h"
#include "Simd/SimdExtract.h"
#include "Simd/SimdSynetConvolution8i.h"
#include "Simd/SimdSynetConvolution32fCommon.h"

namespace Simd
{
#if defined(SIMD_SSE41_ENABLE)   
    namespace Sse41
    {
        template <Base::SynetConvolution8iNhwcDirect::Term8iType term> struct Term8i
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t * dst, int32_t * buf, __m128i sum, 
                const __m128 * norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper);
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t * dst, int32_t * buf, __m128i sum, 
                const __m128 * norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail);
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iSingle8u>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                __m128 f32 = Sse2::Activate<type>(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(sum), norm[index]), bias[index]), params, index);
                __m128i i32 = _mm_cvtps_epi32(_mm_add_ps(_mm_mul_ps(f32, scale[index]), shift[index]));
                ((int32_t*)dst)[index] = _mm_cvtsi128_si32(_mm_min_epu8(_mm_packus_epi16(_mm_packs_epi32(i32, K_ZERO), K_ZERO), upper));
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                uint8_t tmp[F];
                Term8i::Save<type, index>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iSingle32f>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                __m128 f32 = Sse2::Activate<type>(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(sum), norm[index]), bias[index]), params, index);
                _mm_storeu_ps((float*)dst + index*F, f32);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                uint8_t tmp[A];
                Term8i::Save<type, index>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iFirst>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                _mm_storeu_si128((__m128i*)buf + index, sum);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                int32_t tmp[F];
                _mm_storeu_si128((__m128i*)tmp, sum);
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iIterim>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                _mm_storeu_si128((__m128i*)buf + index, _mm_add_epi32(_mm_loadu_si128((__m128i*)buf + index), sum));
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                int32_t tmp[F];
                _mm_storeu_si128((__m128i*)tmp, _mm_add_epi32(_mm_loadu_si128((__m128i*)buf + index), sum));
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iLast8u>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                sum = _mm_add_epi32(_mm_loadu_si128((__m128i*)buf + index), sum);
                __m128 f32 = Sse2::Activate<type>(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(sum), norm[index]), bias[index]), params, index);
                __m128i i32 = _mm_cvtps_epi32(_mm_add_ps(_mm_mul_ps(f32, scale[index]), shift[index]));
                ((int32_t*)dst)[index] = _mm_cvtsi128_si32(_mm_min_epu8(_mm_packus_epi16(_mm_packs_epi32(i32, K_ZERO), K_ZERO), upper));
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                uint8_t tmp[F];
                Term8i::Save<type, index>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iLast32f>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
            {
                sum = _mm_add_epi32(_mm_loadu_si128((__m128i*)buf + index), sum);
                __m128 f32 = Sse2::Activate<type>(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(sum), norm[index]), bias[index]), params, index);
                _mm_storeu_ps((float*)dst + index * F, f32);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, __m128i sum,
                const __m128* norm, const __m128* bias, const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
            {
                uint8_t tmp[A];
                Term8i::Save<type, index>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type>
        SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, __m128i sum, const __m128* norm, const __m128* bias,
            const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type>
        SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, __m128i sum, const __m128* norm, const __m128* bias,
            const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum, norm, bias, params, scale, shift, upper, tail);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type> 
        SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, __m128i sum0, __m128i sum1, const __m128* norm, const __m128* bias,
            const __m128* params, const __m128* scale, const __m128* shift, __m128i upper)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term8i<term>::template Save<type, 1>(dst, buf, sum1, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type>
        SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, __m128i sum0, __m128i sum1, const __m128* norm, const __m128* bias,
            const __m128* params, const __m128* scale, const __m128* shift, __m128i upper, size_t tail)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term8i<term>::template Save<type, 1>(dst, buf, sum1, norm, bias, params, scale, shift, upper, tail);
        }
    }
#endif//SIMD_SSE41_ENABLE

#if defined(SIMD_AVX2_ENABLE) && 0  
    namespace Avx2
    {
        template<::SimdConvolutionActivationType type> SIMD_INLINE __m256i Activate(__m256i value, const __m256* params, size_t index);

        template<> SIMD_INLINE __m256i Activate<::SimdConvolutionActivationIdentity>(__m256i value, const __m256* params, size_t index)
        {
            return value;
        }

        template<> SIMD_INLINE __m256i Activate<::SimdConvolutionActivationRelu>(__m256i value, const __m256* params, size_t index)
        {
            return _mm256_max_epi32(_mm256_setzero_si256(), value);
        }

        template<> SIMD_INLINE __m256i Activate<::SimdConvolutionActivationRestrictRange>(__m256i value, const __m256* params, size_t index)
        {
            return _mm256_min_epi32(_mm256_max_epi32(_mm256_castps_si256(params[0]), value), _mm256_castps_si256(params[1]));
        }

        template<> SIMD_INLINE __m256i Activate<::SimdConvolutionActivationPrelu>(__m256i value, const __m256* params, size_t index)
        {
            __m256i positive = _mm256_max_epi32(_mm256_setzero_si256(), value);
            __m256i negative = _mm256_min_epi32(_mm256_setzero_si256(), value);
            return _mm256_or_si256(positive, _mm256_cvtps_epi32(_mm256_mul_ps(params[index], _mm256_cvtepi32_ps(negative))));
        }

        template <Base::SynetConvolution8iNhwcDirect::Term8iType term> struct Term
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper);
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail);
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iSingle8u>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                __m256i i32 = Activate<type>(_mm256_add_epi32(_mm256_mullo_epi32(sum, norm), bias[index]), params, index);
                __m256 f32 = Fmadd<nofma>(_mm256_cvtepi32_ps(i32), scale[index], shift[index]);
                ((int64_t*)dst)[index] = Extract64i<0>(_mm256_min_epu8(PackI16ToU8(PackI32ToI16(_mm256_cvtps_epi32(f32), K_ZERO), K_ZERO), upper));
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                uint8_t tmp[F];
                Term::Save<type, index, nofma>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iSingle32f>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                __m256i i32 = Activate<type>(_mm256_add_epi32(_mm256_mullo_epi32(sum, norm), bias[index]), params, index);
                _mm256_storeu_ps((float*)dst + index * F, Fmadd<nofma>(_mm256_cvtepi32_ps(i32), scale[index], shift[index]));
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                uint8_t tmp[A];
                Save<type, index, nofma>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iFirst>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                _mm256_storeu_si256((__m256i*)buf + index, sum);
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                int32_t tmp[F];
                _mm256_storeu_si256((__m256i*)tmp, sum);
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iIterim>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                _mm256_storeu_si256((__m256i*)buf + index, _mm256_add_epi32(_mm256_loadu_si256((__m256i*)buf + index), sum));
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                int32_t tmp[F];
                _mm256_storeu_si256((__m256i*)tmp, _mm256_add_epi32(_mm256_loadu_si256((__m256i*)buf + index), sum));
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iLast8u>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                sum = _mm256_add_epi32(_mm256_loadu_si256((__m256i*)buf + index), sum);
                __m256i i32 = Activate<type>(_mm256_add_epi32(_mm256_mullo_epi32(sum, norm), bias[index]), params, index);
                __m256 f32 = Fmadd<nofma>(_mm256_cvtepi32_ps(i32), scale[index], shift[index]);
                ((int64_t*)dst)[index] = Extract64i<0>(_mm256_min_epu8(PackI16ToU8(PackI32ToI16(_mm256_cvtps_epi32(f32), K_ZERO), K_ZERO), upper));
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                uint8_t tmp[F];
                Save<type, index, nofma>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iLast32f>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
            {
                sum = _mm256_add_epi32(_mm256_loadu_si256((__m256i*)buf + index), sum);
                __m256i i32 = Activate<type>(_mm256_add_epi32(_mm256_mullo_epi32(sum, norm), bias[index]), params, index);
                _mm256_storeu_ps((float*)dst + index * F, Fmadd<nofma>(_mm256_cvtepi32_ps(i32), scale[index], shift[index]));
            }

            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m256i sum, __m256i norm, const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
            {
                uint8_t tmp[A];
                Save<type, index, nofma>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, __m256i sum, __m256i norm,
            const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, __m256i sum, __m256i norm,
            const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum, norm, bias, params, scale, shift, upper, tail);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, __m256i sum0, __m256i sum1, __m256i norm,
            const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term<term>::template Save<type, 1, nofma>(dst, buf, sum1, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, __m256i sum0, __m256i sum1, __m256i norm,
            const __m256i* bias, const __m256* params, const __m256* scale, const __m256* shift, __m256i upper, size_t tail)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term<term>::template Save<type, 1, nofma>(dst, buf, sum1, norm, bias, params, scale, shift, upper, tail);
        }
    }
#endif//SIMD_AVX2_ENABLE

#if defined(SIMD_AVX512BW_ENABLE) && 0  
    namespace Avx512bw
    {
        template<::SimdConvolutionActivationType type> SIMD_INLINE __m512i Activate(__m512i value, const __m512* params, size_t index);

        template<> SIMD_INLINE __m512i Activate<::SimdConvolutionActivationIdentity>(__m512i value, const __m512* params, size_t index)
        {
            return value;
        }

        template<> SIMD_INLINE __m512i Activate<::SimdConvolutionActivationRelu>(__m512i value, const __m512* params, size_t index)
        {
            return _mm512_max_epi32(_mm512_setzero_si512(), value);
        }

        template<> SIMD_INLINE __m512i Activate<::SimdConvolutionActivationRestrictRange>(__m512i value, const __m512* params, size_t index)
        {
            return _mm512_min_epi32(_mm512_max_epi32(_mm512_castps_si512(params[0]), value), _mm512_castps_si512(params[1]));
        }

        template<> SIMD_INLINE __m512i Activate<::SimdConvolutionActivationPrelu>(__m512i value, const __m512* params, size_t index)
        {
            __m512i positive = _mm512_max_epi32(_mm512_setzero_si512(), value);
            __m512i negative = _mm512_min_epi32(_mm512_setzero_si512(), value);
            return _mm512_or_si512(positive, _mm512_cvtps_epi32(_mm512_mul_ps(params[index], _mm512_cvtepi32_ps(negative))));
        }

        template <Base::SynetConvolution8iNhwcDirect::Term8iType term> struct Term
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1);
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iSingle8u>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                __m512i i32 = Activate<type>(_mm512_add_epi32(_mm512_mullo_epi32(sum, norm), bias[index]), params, index);
                __m512 f32 = Fmadd<nofma>(_mm512_cvtepi32_ps(i32), scale[index], shift[index]);
                __m128i u8 = _mm256_castsi256_si128(Avx2::PackI16ToU8(_mm512_cvtepi32_epi16(_mm512_cvtps_epi32(f32)), Avx2::K_ZERO));
                _mm_mask_storeu_epi8(dst + index * F, tail, _mm_min_epu8(u8, upper));
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iSingle32f>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                __m512i i32 = Activate<type>(_mm512_add_epi32(_mm512_mullo_epi32(sum, norm), bias[index]), params, index);
                _mm512_mask_storeu_ps((float*)dst + index * F, tail, Fmadd<nofma>(_mm512_cvtepi32_ps(i32), scale[index], shift[index]));
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iFirst>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                _mm512_mask_storeu_epi32(buf + index * F, tail, sum);
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iIterim>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                _mm512_mask_storeu_epi32(buf + index * F, tail, _mm512_add_epi32(_mm512_maskz_loadu_epi32(tail, buf + index * F), sum));
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iLast8u>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                sum = _mm512_add_epi32(_mm512_maskz_loadu_epi32(tail, buf + index * F), sum);
                __m512i i32 = Activate<type>(_mm512_add_epi32(_mm512_mullo_epi32(sum, norm), bias[index]), params, index);
                __m512 f32 = Fmadd<nofma>(_mm512_cvtepi32_ps(i32), scale[index], shift[index]);
                __m128i u8 = _mm256_castsi256_si128(Avx2::PackI16ToU8(_mm512_cvtepi32_epi16(_mm512_cvtps_epi32(f32)), Avx2::K_ZERO));
                _mm_mask_storeu_epi8(dst + index * F, tail, _mm_min_epu8(u8, upper));
            }
        };

        template <> struct Term<Base::SynetConvolution8iNhwcDirect::Term8iLast32f>
        {
            template<SimdConvolutionActivationType type, int index, bool nofma> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf,
                __m512i sum, __m512i norm, const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
            {
                sum = _mm512_add_epi32(_mm512_maskz_loadu_epi32(tail, buf + index * F), sum);
                __m512i i32 = Activate<type>(_mm512_add_epi32(_mm512_mullo_epi32(sum, norm), bias[index]), params, index);
                _mm512_mask_storeu_ps((float*)dst + index * F, tail, Fmadd<nofma>(_mm512_cvtepi32_ps(i32), scale[index], shift[index]));
            }
        };

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, __m512i sum, __m512i norm,
            const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum, norm, bias, params, scale, shift, upper, tail);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type, bool nofma>
        SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, __m512i sum0, __m512i sum1, __m512i norm,
            const __m512i* bias, const __m512* params, const __m512* scale, const __m512* shift, __m128i upper, __mmask16 tail = -1)
        {
            Term<term>::template Save<type, 0, nofma>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term<term>::template Save<type, 1, nofma>(dst, buf, sum1, norm, bias, params, scale, shift, upper, tail);
        }
    }
#endif//SIMD_AVX512BW_ENABLE

#if defined(SIMD_NEON_ENABLE)
    namespace Neon
    {
        template <Base::SynetConvolution8iNhwcDirect::Term8iType term> struct Term8i
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm, 
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t * scale, const float32x4_t* shift, uint8x8_t upper);
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail);
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iSingle8u>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                float32x4_t f32 = Activate<type>(vaddq_f32(vmulq_f32(vcvtq_f32_s32(sum), norm[index]), bias[index]), params, index);
                int32x4_t i32 = Round(vaddq_f32(vmulq_f32(f32, scale[index]), shift[index]));
                uint8x8_t u8 = vmin_u8(vqmovun_s16(vcombine_s16(vmovn_s32(i32), vcreate_s16(0))), upper);
                ((int32_t*)dst)[index] = vget_lane_s32(vreinterpret_s32_u8(u8), 0);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                uint8_t tmp[F];
                Term8i::Save<type, index>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iSingle32f>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                float32x4_t f32 = Activate<type>(vaddq_f32(vmulq_f32(vcvtq_f32_s32(sum), norm[index]), bias[index]), params, index);
                Store<false>((float*)dst + index * F, f32);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                uint8_t tmp[A];
                Term8i::Save<type, index>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iFirst>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                Store<false>(buf + index * F, sum);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                int32_t tmp[F];
                Store<false>(tmp, sum);
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iIterim>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                Store<false>(buf + index * F, vaddq_s32(Load<false>(buf + index * F), sum));
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                int32_t tmp[F];
                Store<false>(tmp, vaddq_s32(Load<false>(buf + index * F), sum));
                for (size_t i = 0; i < tail; ++i)
                    buf[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iLast8u>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                sum = vaddq_s32(Load<false>(buf + index * F), sum);
                float32x4_t f32 = Activate<type>(vaddq_f32(vmulq_f32(vcvtq_f32_s32(sum), norm[index]), bias[index]), params, index);
                int32x4_t i32 = Round(vaddq_f32(vmulq_f32(f32, scale[index]), shift[index]));
                uint8x8_t u8 = vmin_u8(vqmovun_s16(vcombine_s16(vmovn_s32(i32), vcreate_s16(0))), upper);
                ((int32_t*)dst)[index] = vget_lane_s32(vreinterpret_s32_u8(u8), 0);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                uint8_t tmp[F];
                Term8i::Save<type, index>(tmp - index * F, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    dst[index * F + i] = tmp[i];
            }
        };

        template <> struct Term8i<Base::SynetConvolution8iNhwcDirect::Term8iLast32f>
        {
            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
            {
                sum = vaddq_s32(Load<false>(buf + index * F), sum);
                float32x4_t f32 = Activate<type>(vaddq_f32(vmulq_f32(vcvtq_f32_s32(sum), norm[index]), bias[index]), params, index);
                Store<false>((float*)dst + index * F, f32);
            }

            template<SimdConvolutionActivationType type, int index> static SIMD_INLINE void Save(uint8_t* dst, int32_t* buf, int32x4_t sum, const float32x4_t* norm,
                const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
            {
                uint8_t tmp[A];
                Term8i::Save<type, index>(tmp - index * A, buf, sum, norm, bias, params, scale, shift, upper);
                for (size_t i = 0; i < tail; ++i)
                    ((float*)dst)[index * F + i] = ((float*)tmp)[i];
            }
        };

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type> SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, int32x4_t sum, 
            const float32x4_t* norm, const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type> SIMD_INLINE void Save1(uint8_t* dst, int32_t* buf, int32x4_t sum, 
            const float32x4_t* norm, const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum, norm, bias, params, scale, shift, upper, tail);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type> SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, int32x4_t sum0, 
            int32x4_t sum1, const float32x4_t* norm, const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term8i<term>::template Save<type, 1>(dst, buf, sum1, norm, bias, params, scale, shift, upper);
        }

        template<Base::SynetConvolution8iNhwcDirect::Term8iType term, SimdConvolutionActivationType type> SIMD_INLINE void Save2(uint8_t* dst, int32_t* buf, int32x4_t sum0,
            int32x4_t sum1, const float32x4_t* norm, const float32x4_t* bias, const float32x4_t* params, const float32x4_t* scale, const float32x4_t* shift, uint8x8_t upper, size_t tail)
        {
            Term8i<term>::template Save<type, 0>(dst, buf, sum0, norm, bias, params, scale, shift, upper);
            Term8i<term>::template Save<type, 1>(dst, buf, sum1, norm, bias, params, scale, shift, upper, tail);
        }
    }
#endif//SIMD_NEON_ENABLE
}
#endif//__SimdSynetConvolution8iCommon_h__
