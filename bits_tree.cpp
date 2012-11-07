#include "bits_tree.h"
#include <iostream>

#if defined(_LOG_USE_STDOUT)
//#define BITS_DEBUG(a) std::cout << "[" << __FILE__ << ":" << __LINE__ << "]" << a << std::endl
#define BITS_DEBUG(a)
#else
#define BITS_DEBUG(a)
#endif

static void  bits_node_dellocate(void* ptr, size_t size)
{
        free(ptr);
}


BitsBranchNode::allocate_func   BitsBranchNode::_allocate = malloc;
BitsBranchNode::dellocate_func  BitsBranchNode::_dellocate = bits_node_dellocate;


#define BIT_ALIGN 2
#define bits_align(d, a)     (((d) + ((a) - 1)) & ~((a) - 1))

#define bits_alloc(num, each_size) ({ \
        char* addr_ret = (char*)_allocate(bits_align(num, BIT_ALIGN) * each_size); \
        BITS_DEBUG("allocate num=" << num << ", size=" << (int)each_size << ", addr=" << (void*)addr_ret); \
        addr_ret;})
        
#define bits_delloc(addr, num, each_size) do { \
        BITS_DEBUG("dellocate num=" << num << ", size=" << (int)each_size << ", addr=" << (void*)addr); \
        _dellocate(addr, bits_align(num, BIT_ALIGN) * each_size); \
        } while (0)

#define max_bits(depth) (1<<(6*depth))

static uint32_t  bits_multi[] = {0, 6, 12, 18, 24, 30};

void  BitsBranchNode::init()
{
        bzero(this, sizeof(*this));
}

void  BitsBranchNode::uinit(uint8_t depth_index)
{
        if (branch_childs == NULL)
                return;

        assert(bits.bit_64);
        int num = yf_bitset_num(&bits);
        
        if (depth_index != 1)
        {
                for (int i = 0; i < num; ++i)
                {
                        ((BitsBranchNode*)branch_childs)[i].uinit(depth_index - 1);
                }
                bits_delloc(branch_childs, num, sizeof(BitsBranchNode));
        }
        else
                bits_delloc(branch_childs, num, sizeof(BitsLeafNode));

        bits.bit_64 = 0;
        branch_childs = NULL;
}


void  BitsBranchNode::add(uint32_t val, uint8_t depth_index)
{
        uint8_t pre_bits = (val >> bits_multi[depth_index]);
        uint8_t bit_offset = 0;
        uint8_t child_size = depth_index > 1 ? sizeof(BitsBranchNode) : sizeof(BitsLeafNode);
        char* insert_addr = NULL;
        
        if (!yf_test_bit(bits.bit_64, pre_bits))
        {
                yf_set_bit(bits.bit_64, pre_bits);

                yf_def_bitindexs(bits, bitindex);
                yf_def_offset_starts(bitindex, offset_starts);
                
                bit_offset = yf_bitset_offset(bitindex, offset_starts, pre_bits);
                insert_addr = (char*)branch_childs + bit_offset * child_size;
                
                int  now_num = yf_bitset_num(&bits);
                if (now_num & 1)
                {
                        char* new_addr = bits_alloc(now_num, child_size);

                        if (branch_childs)
                        {
                                memcpy(new_addr, branch_childs, bit_offset * child_size);
                                memcpy(new_addr + (bit_offset + 1) * child_size, insert_addr, 
                                                (now_num-1 - bit_offset) * child_size);
                                
                                bits_delloc(branch_childs, now_num-1, child_size);
                        }

                        insert_addr = new_addr + bit_offset * child_size;
                        branch_childs = new_addr;
                }
                else {
                        memmove(insert_addr + child_size, insert_addr, 
                                        (now_num-1 - bit_offset) * child_size);

#if defined(_LOG_USE_STDOUT)
                        if (now_num-1 > bit_offset && depth_index == 1)
                        {
                                BITS_DEBUG("memmove, target=" << (void*)(insert_addr + child_size)
                                        << ", size=" << (now_num-1 - bit_offset) * child_size
                                        << ", org val=" << *(uint64_t*)(insert_addr + child_size));
                        }
#endif
                }

                //dangeous...
                bzero(insert_addr, child_size);
        }
        else {
                yf_def_bitindexs(bits, bitindex);
                yf_def_offset_starts(bitindex, offset_starts);
                
                bit_offset = yf_bitset_offset(bitindex, offset_starts, pre_bits);
                insert_addr = (char*)branch_childs + bit_offset * child_size;
        }

        val &= ((1<<bits_multi[depth_index])-1);

        BITS_DEBUG("depth=" << (int)depth_index << ", add bits=" << (int)pre_bits << ", offset=" << (int)bit_offset 
                        << ", child rest val=" << val << ", addr=" << (void*)insert_addr);

        if (depth_index == 1)
        {
                yf_set_bit(((yf_bit_set_t*)insert_addr)->bit_64, val);
        }
        else
                ((BitsBranchNode*)insert_addr)->add(val, depth_index - 1);
}


void  BitsBranchNode::del(uint32_t val, uint8_t depth_index)
{
        yf_def_bitindexs(bits, bitindex);
        yf_def_offset_starts(bitindex, offset_starts);
        
        uint8_t pre_bits = (val >> bits_multi[depth_index]);
        uint8_t bit_offset = yf_bitset_offset(bitindex, offset_starts, pre_bits);
        uint8_t child_size = depth_index > 1 ? sizeof(BitsBranchNode) : sizeof(BitsLeafNode);
        char* del_addr = (char*)branch_childs + bit_offset * child_size;
        yf_bit_set_t* child_bitset = NULL;
        
        val &= ((1<<bits_multi[depth_index])-1);

        BITS_DEBUG("depth=" << (int)depth_index << ", delete bits=" << (int)pre_bits << ", offset=" << (int)bit_offset 
                        << ", child rest val=" << val << ", addr=" << (void*)del_addr);

        if (depth_index == 1)
        {
                child_bitset = (yf_bit_set_t*)del_addr;
                yf_reset_bit(child_bitset->bit_64, val);
        }
        else {
                ((BitsBranchNode*)del_addr)->del(val, depth_index - 1);
                child_bitset = &((BitsBranchNode*)del_addr)->bits;
        }

        if (child_bitset->bit_64)
                return;

        yf_reset_bit(bits.bit_64, pre_bits);
        int  now_num = yf_bitset_num(&bits);
        if (now_num & 1)
        {
                memmove(del_addr, del_addr + child_size, 
                                (now_num - bit_offset) * child_size);
        }
        else {
                char* new_addr = NULL;
                if (now_num)
                {
                        new_addr = bits_alloc(now_num, child_size);
                        
                        memcpy(new_addr, branch_childs, bit_offset * child_size);
                        memcpy(new_addr + bit_offset * child_size, del_addr + child_size, 
                                        (now_num - bit_offset) * child_size);
                }
                bits_delloc(branch_childs, now_num+2, child_size);
                branch_childs = new_addr;
        }
}


void  BitsBranchNode::copy_from(BitsBranchNode* bb, uint8_t depth_index)
{
        uinit(depth_index);
        
        uint8_t child_size = depth_index > 1 ? sizeof(BitsBranchNode) : sizeof(BitsLeafNode);
        int src_num = yf_bitset_num(&bb->bits);
        
        bits = bb->bits;
        branch_childs = bits_alloc(src_num, child_size);
        
        if (depth_index == 1)
        {
                memcpy(branch_childs, bb->branch_childs, child_size * src_num);
        }
        else {
                for (int i = 0; i < src_num; ++i)
                {
                        ((BitsBranchNode*)branch_childs)[i].init();
                        ((BitsBranchNode*)branch_childs)[i].copy_from(
                                        (BitsBranchNode*)bb->branch_childs + i, depth_index - 1);
                }
        }
}


void  BitsBranchNode::iter(bits_iter_func func, void* data, uint8_t depth_index)
{
        iter_data  data_tmp = {data, 0};
        _iter(func, &data_tmp, depth_index);
}

inline void BitsBranchNode::_iter(bits_iter_func func, iter_data* data, uint8_t depth_index)
{
        yf_def_bitindexs(bits, bitindex);

        int  bits_num[] = 
                { bitindex[0]->num
                , bitindex[0]->num + bitindex[1]->num
                , bitindex[0]->num + bitindex[1]->num + bitindex[2]->num
                , bitindex[0]->num + bitindex[1]->num + bitindex[2]->num + bitindex[3]->num
                };

        iter_data tmp_data = {data->data, data->pre_bits};
        uint32_t  pre_bits = 0, step = bits_multi[depth_index];
        int i = 0, j = 0;

#define  iter_bits_parts(part) \
                pre_bits = data->pre_bits | ((16*part) << step); \
                for (j = 0; i < bits_num[part]; ++i, ++j) { \
                        tmp_data.pre_bits = (pre_bits | (bitindex[part]->indexs[j] << step)); \
                        BITS_DEBUG("depth=" << (int)depth_index << ", tpre_bits=" << pre_bits << ", part=" << part \
                                << ", tmppre_bits=" << tmp_data.pre_bits << ", part offset=" << j ); \
                        iter_child; \
                }
#define iter_all_parts iter_bits_parts(0);iter_bits_parts(1);iter_bits_parts(2);iter_bits_parts(3);
        
        if (depth_index == 1)
        {
#define iter_child assert(((yf_bit_set_t*)branch_childs + i)->bit_64);yf_iter_bitset((yf_bit_set_t*)branch_childs + i, func, (&tmp_data))
                iter_all_parts;
#undef  iter_child
        }
        else {
#define iter_child ((BitsBranchNode*)branch_childs + i)->_iter(func, (&tmp_data), depth_index-1)
                iter_all_parts;
#undef  iter_child
        }
}


bool  BitsBranchNode::intersect(BitsBranchNode* bb, uint8_t depth_index)
{
        yf_bit_set_t  intersect_bits;
        intersect_bits.bit_64 = bb->bits.bit_64 & bits.bit_64;

        if (intersect_bits.bit_64 == 0)
        {
                uinit(depth_index);
                return false;
        }

        uint8_t child_size = depth_index > 1 ? sizeof(BitsBranchNode) : sizeof(BitsLeafNode);

        yf_def_bitindexs(bb->bits, rh_bitindex);
        yf_def_offset_starts(rh_bitindex, rh_offset_starts);
        
        int  i = 0, res_offset = 0;
        char* child_addr = NULL, *rh_child_addr = NULL;
        bool  child_intersect = false;

#define intersect_child(addr, index) \
                rh_child_addr = (char*)bb->branch_childs + child_size * \
                        yf_bitset_offset(rh_bitindex, rh_offset_starts, index); \
                if (depth_index == 1) \
                        child_intersect = (((yf_bit_set_t*)addr)->bit_64 &= ((yf_bit_set_t*)rh_child_addr)->bit_64); \
                else \
                        child_intersect = ((BitsBranchNode*)addr)->intersect((BitsBranchNode*)rh_child_addr, depth_index - 1);

        char  buf_tmp[64*sizeof(BitsBranchNode)];
        
#define on_each_index(index, data) \
        if (yf_test_bit(intersect_bits.bit_64, index)) { \
                child_addr = (char*)branch_childs + i * child_size; \
                intersect_child(child_addr, index); \
                if (child_intersect) \
                        memcpy(buf_tmp + res_offset++ * child_size, child_addr, child_size); \
                else \
                        yf_reset_bit(intersect_bits.bit_64, index); \
        } \
        else if (depth_index > 1) \
                ((BitsBranchNode*)branch_childs + i)->uinit(depth_index - 1); \
        ++i;
        
        yf_iter_bitset(&bits, on_each_index, NULL);

        if (bits.bit_64 == intersect_bits.bit_64)
                return true;
        
        if (res_offset == 0)
        {
                uinit(depth_index);
                return false;
        }

        char* target = (i - res_offset > 1 || (i & 1)) ? 
                        bits_alloc(res_offset, child_size) : (char*)branch_childs;
        memcpy(target, buf_tmp, res_offset * child_size);

        if (target != branch_childs)
        {
                bits_delloc(branch_childs, i, child_size);
                branch_childs = target;
        }
        bits = intersect_bits;
        return true;
}



void  BitsTree::init(uint32_t max_leaf_num)
{
        depth = 2;
        while (max_bits(depth) <= (int)max_leaf_num)
                ++depth;

        leaf_num = 0;
        bzero(&root, sizeof(root));
}

int  BitsTree::add(uint32_t val)
{
        if (max_bits(depth) <= (int)val)
                return -1;

        uint8_t now_rdepth = 0;
        return  0;
}

int  BitsTree::del(uint32_t val)
{
        if (max_bits(depth) <= val)
                return -1;        
        return  0;
}


void  BitsTree::copy_from(BitsTree* bt)
{
}


