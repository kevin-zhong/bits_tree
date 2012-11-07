#ifndef _BITS_TREE_TESTOR_H_20121105_H
#define _BITS_TREE_TESTOR_H_20121105_H
/*
* author: kevin_zhong, mail:qq2000zhong@gmail.com
* time: 20121105-15:08:30
*/

#include <gtest/gtest.h>
#include <vector>
#include <set>
#include "bits_tree.h"
#include <sys/time.h>

uint64_t  set_intersect_us = 0;
uint64_t  bits_intersect_us = 0;


using namespace testing;

class BitsTreeTestorCase : public testing::Test
{
public:        
        virtual ~BitsTreeTestorCase()
        {
        }
        virtual void SetUp()
        {
                ::srandom(time(NULL));
                yf_init_bit_indexs();
        }
        virtual void TearDown()
        {
                std::cout << "set_intersect_us=" << set_intersect_us 
                                << " vs bits_intersect_us=" << bits_intersect_us << std::endl;
        }
};

std::vector<int> add_vals_v;
int  add_index = 0;

void bits_iter_testor(uint8_t bit_index, iter_data* data)
{
        int  add_val = add_vals_v[add_index];
        //std::cout << "cmp i=" << add_index << ", org=" << add_val 
                        //<< ", target=" << (data->pre_bits|bit_index) << std::endl;
        assert(add_val == (data->pre_bits|bit_index));
        ++add_index;
}

TEST_F(BitsTreeTestorCase, TestBitsNode)
{
        BitsBranchNode  bits_node;
        bits_node.init();

        std::set<int> add_vals;
        int try_cnt = ::random() % 53;
        int add_val = 0;

        for (int i = 0; i < try_cnt; ++i)
        {
                std::cout << "restart nowww" << std::endl;
                
                int add_cnt = ::random() % 20203 + 1;
                //int add_cnt = 2;
                //int vals[] = {67, 18};
                for (int j = 0; j < add_cnt; ++j)
                {
                        add_val = ::random() % 262143;
                        //add_val = vals[j];
                        add_vals.insert(add_val);
                        std::cout << "insert val=" << add_val << std::endl;
                        bits_node.add(add_val, 2);
                }

                add_vals_v.assign(add_vals.begin(), add_vals.end());
                bits_node.iter(bits_iter_testor, NULL, 2);

                add_vals_v.clear();
                add_index = 0;

                int delete_cnt = ::random() % add_vals.size();
                for (int k = 0; k < delete_cnt; ++k)
                {
                        std::set<int>::iterator iter = add_vals.begin();
                        std::advance(iter, ::random() % add_vals.size());
                        std::cout << "delete val=" << *iter << std::endl;

                        bits_node.del(*iter, 2);
                        add_vals.erase(iter);
                }

                add_vals_v.assign(add_vals.begin(), add_vals.end());
                bits_node.iter(bits_iter_testor, NULL, 2);

                add_vals.clear();
                add_vals_v.clear();
                add_index = 0;
                bits_node.uinit(2);                
        }
}

/*
经过比较, bits intersect 算法比普通的常规算法要快 21 倍以上，且规模越大，时间越长，倍数关系越大

set_intersect_us=36948755 vs bits_intersect_us=1704920
set_intersect_us=37100777 vs bits_intersect_us=1711254

set_intersect_us=48917692 vs bits_intersect_us=2219890
set_intersect_us=49026384 vs bits_intersect_us=2224468

set_intersect_us=70750461 vs bits_intersect_us=3156920
set_intersect_us=70783963 vs bits_intersect_us=3158403

set_intersect_us=85429699 vs bits_intersect_us=3790865
set_intersect_us=85564210 vs bits_intersect_us=3796824
*/

TEST_F(BitsTreeTestorCase, TestBitsIntersect)
{
        BitsBranchNode  bits_org[2], bits_intersect;
        bits_org[0].init();
        bits_org[1].init();
        bits_intersect.init();

        std::vector<int> vals_set[2];
        int try_cnt = ::random() % 53;
        int add_val = 0;

        struct timeval tv_begin, tv_end;

        for (int i = 0; i < try_cnt; ++i)
        {
                std::cout << "restart nowww" << std::endl;

                for (int vals_i = 0; vals_i < 2; ++vals_i)
                {
                        int add_cnt = ::random() % 40003 + 1;
                        //int add_cnt = 10;
                        
                        for (int j = 0; j < add_cnt; ++j)
                        {
                                add_val = ::random() % 262143;
                                //std::cout << "org val=" << add_val << std::endl;
                                
                                vals_set[vals_i].push_back(add_val);
                                bits_org[vals_i].add(add_val, 2);
                        }
                        std::sort(vals_set[vals_i].begin(), vals_set[vals_i].end());
                }

                add_vals_v.clear();
                add_index = 0;

                int last_val = -1, first_val, second_val;
                gettimeofday(&tv_begin, NULL);
                for (int first=0, second=0; first < vals_set[0].size() && second < vals_set[1].size(); )
                {
                        first_val = vals_set[0][first];
                        second_val = vals_set[1][second];

                        if (first_val == second_val)
                        {
                                ++first;
                                ++second;
                                
                                if (first_val > last_val)
                                {
                                        add_vals_v.push_back(first_val);
                                        last_val = first_val;
                                }
                        }
                        else if (first_val < second_val)
                                ++first;
                        else
                                ++second;
                }
                gettimeofday(&tv_end, NULL);
                set_intersect_us += 1000000 * (tv_end.tv_sec - tv_begin.tv_sec) + tv_end.tv_usec - tv_begin.tv_usec;

                std::cout << "copy from, then intersect cmp cnt=" << add_vals_v.size() << std::endl;

                gettimeofday(&tv_begin, NULL);
                bits_intersect.copy_from(bits_org, 2);
                bits_intersect.intersect(bits_org+1, 2);
                gettimeofday(&tv_end, NULL);
                bits_intersect_us += 1000000 * (tv_end.tv_sec - tv_begin.tv_sec) + tv_end.tv_usec - tv_begin.tv_usec;

                bits_intersect.iter(bits_iter_testor, NULL, 2);

                bits_org[0].uinit(2);
                bits_org[1].uinit(2);
                bits_intersect.uinit(2);

                vals_set[0].clear();
                vals_set[1].clear();                
        }
}

#endif
