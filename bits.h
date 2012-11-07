#ifndef _BITS_H_20121031_H
#define _BITS_H_20121031_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20121031-20:18:54
*/

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>

#define yf_set_bit(val, index) ((val) |= ((uint64_t)1 << (index)))
#define yf_reset_bit(val, index) ((val) &= ~((uint64_t)1 << (index)))
#define yf_revert_bit(val, index) ((val) ^= ((uint64_t)1 << (index)))
#define yf_test_bit(val, index) ((val) & ((uint64_t)1 << (index)))

#define  WORDS_LITTLE_PART  0
#define  YF_IS_LT_PART(p) (p == WORDS_LITTLE_PART)

typedef  struct
{
        int8_t  indexs[17];//last is end tick...
        int8_t  num;
        int8_t  offsets[16];
}
yf_bit_index_t;

#define  YF_END_INDEX  -1
#define  yf_index_end(bit_indexs, i) (bit_indexs[i] == YF_END_INDEX)

extern yf_bit_index_t  yf_bit_indexs[65536];
extern void yf_init_bit_indexs();


typedef  union
{
        uint64_t  bit_64;
        union {
                uint32_t  bit_32;
                uint16_t  bit_16[2];
        } ubits[2];
}
yf_bit_set_t;

#define YF_BITS_CNT 64

#define  _YF_TEST_BIT16(bit_set, func, data, _first_part, _next_part) \
        test_val = (bit_set)->ubits[_first_part].bit_16[_next_part]; \
        if (test_val) { \
                begin_index = (YF_IS_LT_PART(_first_part) ? 0 : 32) \
                        + (YF_IS_LT_PART(_next_part) ? 0 : 16); \
                ; \
                unempty_indexs = yf_bit_indexs[test_val].indexs; \
                for (i2 = 0; !yf_index_end(unempty_indexs, i2); ++i2) { \
                        func(begin_index + unempty_indexs[i2], data); \
                } \
        }

#define  _YF_TEST_BIT32(bit_set, func, data, _first_part) \
        if ((bit_set)->ubits[_first_part].bit_32) \
        { \
                _YF_TEST_BIT16((bit_set), func, data, _first_part, WORDS_LITTLE_PART); \
                _YF_TEST_BIT16((bit_set), func, data, _first_part, 1 - WORDS_LITTLE_PART); \
        }

#define yf_iter_bitset(bit_set, func, data) \
        if ((bit_set)->bit_64) { \
                int  i2, begin_index; \
                uint16_t  test_val; \
                int8_t*  unempty_indexs; \
                _YF_TEST_BIT32((bit_set), func, data, WORDS_LITTLE_PART); \
                _YF_TEST_BIT32((bit_set), func, data, 1 - WORDS_LITTLE_PART); \
        }

#define yf_bitset_num(bit_set) \
        ( yf_bit_indexs[(bit_set)->ubits[0].bit_16[0]].num \
        + yf_bit_indexs[(bit_set)->ubits[0].bit_16[1]].num \
        + yf_bit_indexs[(bit_set)->ubits[1].bit_16[0]].num \
        + yf_bit_indexs[(bit_set)->ubits[1].bit_16[1]].num )

#define yf_def_bitindexs(bits, bitindex) \
        yf_bit_index_t* bitindex[] = \
                { &yf_bit_indexs[bits.ubits[WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART]] \
                , &yf_bit_indexs[bits.ubits[WORDS_LITTLE_PART].bit_16[1-WORDS_LITTLE_PART]] \
                , &yf_bit_indexs[bits.ubits[1-WORDS_LITTLE_PART].bit_16[WORDS_LITTLE_PART]] \
                , &yf_bit_indexs[bits.ubits[1-WORDS_LITTLE_PART].bit_16[1-WORDS_LITTLE_PART]] \
                }

#define yf_def_offset_starts(bitindex, offset_starts) \
        int  offset_starts[] = { 0 \
                , bitindex[0]->num \
                , bitindex[0]->num + bitindex[1]->num \
                , bitindex[0]->num + bitindex[1]->num + bitindex[2]->num \
                }

#define yf_bitset_offset(bitindex, offset_starts, bit_index) ({ \
        bit_index < 16 ? bitindex[0]->offsets[bit_index] : \
        (bit_index < 32 ? offset_starts[1] + bitindex[1]->offsets[bit_index-16] : \
        (bit_index < 48 ? offset_starts[2] + bitindex[2]->offsets[bit_index-32] : \
                                   offset_starts[3] + bitindex[3]->offsets[bit_index-48])); })


#endif
