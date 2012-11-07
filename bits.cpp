#include "bits.h"

yf_bit_index_t  yf_bit_indexs[65536];

void yf_init_bit_indexs()
{
        static int  inited = 0;
        if (inited)
                return;
        inited = 1;
        uint32_t i, bit_val;

        for (i = 0; i < sizeof(yf_bit_indexs) / sizeof(yf_bit_indexs[0]); ++i)
        {
                int8_t  index = 0, offset = 0;
                int8_t *indexs = yf_bit_indexs[i].indexs, *offsets = yf_bit_indexs[i].offsets;
                
                for (bit_val = i; bit_val; bit_val >>= 1, ++index)
                {
                        offsets[index] = -1;
                        
                        if (!yf_test_bit(bit_val, 0))
                                continue;

                        offsets[index] = offset;
                        indexs[offset++] = index;
                }

                indexs[offset] = YF_END_INDEX;
                
                yf_bit_indexs[i].num = offset;
        }
}



