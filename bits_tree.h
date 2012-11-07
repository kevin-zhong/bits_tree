#ifndef _BITS_TREE_H_20121031_H
#define _BITS_TREE_H_20121031_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20121031-20:47:38
*/

#include "bits.h"

typedef yf_bit_set_t BitsLeafNode;

struct  iter_data
{
        void* data;
        uint32_t  pre_bits;
};

typedef void (*bits_iter_func)(uint8_t bit_index, iter_data* data);

struct  BitsBranchNode
{
        yf_bit_set_t  bits;
        void* branch_childs;

        typedef  void* (*allocate_func)(size_t size);
        typedef  void  (*dellocate_func)(void* ptr, size_t size);

        static allocate_func   _allocate;
        static dellocate_func  _dellocate;

        void  init();
        void  uinit(uint8_t depth_index);
        
        void  add(uint32_t val, uint8_t depth_index);
        void  del(uint32_t val, uint8_t depth_index);

        void  iter(bits_iter_func func, void* data, uint8_t depth_index);
        
        void  copy_from(BitsBranchNode* bb, uint8_t depth_index);
        bool  intersect(BitsBranchNode* bb, uint8_t depth_index);

private:
        void _iter(bits_iter_func func, iter_data* data, uint8_t depth_index);
};


struct  BitsTree
{
        BitsBranchNode  root;
        uint32_t  leaf_num;
        uint8_t    depth;
        uint8_t    padding8;
        uint16_t  padding16;

        void  init(uint32_t max_leaf_num);
        int  add(uint32_t val);
        int  del(uint32_t val);

        void  copy_from(BitsTree* bt);
};

#endif
