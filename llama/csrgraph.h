#ifndef CSRGRAPH_H_
#define CSRGRAPH_H_

#include <vector>
#include <algorithm>
#include <utility>
#include <cstdio>

#include "snapshot.h"

typedef int64_t Offset;

template <typename Node>
class csrgraph{
    typedef std::pair<Node,Node> Edge;

    size_t num_node_;
    size_t nodelist_size_;
    Node** out_idx_;
    Node* out_nodelist_;
    Node** in_idx_;
    Node* in_nodelist_;

    Node find_max_node(std::vector<Edge> &el){
        Node max_node = 0;
        #pragma omp parallel for reduction(max: max_node)
        for(auto iter = el.begin(); iter<el.end(); ++iter){
            Node mx = std::max(iter->first, iter->second);
            max_node = std::max(max_node, mx);
        }
        return max_node;
    }

    size_t find_current_num_node(std::vector<Edge> &el){
        size_t cnt = 0;
        bool *exist = new bool[num_node_];
        std::fill(exist, exist+num_node_,false);
        #pragma omp parallel for reduction(+:cnt)
        for(auto iter = el.begin(); iter<el.end(); ++iter){
            if(!exist[iter->first]){
                cnt++;
                exist[iter->first] = true;
            }
        }
        return cnt;
    }

    Node* count_degree(std::vector<Edge> &el, bool inv, bool deltamap){
        Node* degree = new Node[num_node_];
        #pragma omp parallel for
        for(int i=0;i<num_node_;++i) degree[i] = 0;

        #pragma omp parallel for
        for(auto iter = el.begin();iter<el.end();++iter){
            if(inv){
                if(!deltamap || !__sync_bool_compare_and_swap(&degree[iter->second], 0, 3))
                    __sync_fetch_and_add(&degree[iter->second],1);
            }else {
                if(!deltamap || !__sync_bool_compare_and_swap(&degree[iter->first], 0, 3))
                    __sync_fetch_and_add(&degree[iter->first],1);
            }
        }
        return degree;
    }

    Offset* count_offset(Node* degree){
        size_t block_size = 1<<20;
        size_t num_block = (num_node_ + (block_size-1))/block_size;

        Offset* offset = new Offset[num_node_+1];
        #pragma omp parallel for
        for(int i=0;i<num_node_+1;++i) offset[i] = 0;

        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_node_);
            #pragma omp parallel for reduction(+:offset[end])
            for(int j=block_size*i; j<end;++j) offset[end] += degree[j];
        }

        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_node_);
            offset[end] += offset[block_size*i];
        }

        #pragma omp parallel for
        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_node_);
            Offset cur = 0;
            for(int j=block_size*i; j<end;++j){
                offset[j] = cur;
                cur += degree[j];
            }
        }

        return offset;
    }

    void set_index(Offset* offset, Node** idx, Node* nodelist){
        #pragma omp parallel for
        for(int i=0;i<num_node_+1;++i)
            idx[i] = nodelist + offset[i];
    }

    void makeCSR(std::vector<Edge> &el, Node** idx, Node* nodelist, bool inv){
        Node *degree = count_degree(el, inv, false);
        Offset *offset = count_offset(degree);
        
        set_index(offset, idx, nodelist);

        #pragma omp parallel for
        for(auto iter = el.begin(); iter<el.end();++iter){
            if(inv) nodelist[__sync_fetch_and_add(&offset[iter->second],1)] = iter->first;
            else nodelist[__sync_fetch_and_add(&offset[iter->first],1)] = iter->second;
        }

        delete[] degree;
        delete[] offset;
    }

    void makeCSR(std::vector<Edge> &el, Node** idx, Node* nodelist, bool inv, 
        std::vector<snapshot<Node>*> &snapshots){
        Node *degree = count_degree(el, inv, true);
        Offset *offset = count_offset(degree);
        
        set_index(offset, idx, nodelist);

        #pragma omp parallel for
        for(auto iter = el.begin(); iter<el.end();++iter){
            if(inv) nodelist[__sync_fetch_and_add(&offset[iter->second],1)] = iter->first;
            else nodelist[__sync_fetch_and_add(&offset[iter->first],1)] = iter->second;
        }

        delete[] degree;
        delete[] offset;
    }

public:
    csrgraph(std::vector<Edge> &el){
        nodelist_size_ = el.size();
        num_node_ =  find_max_node(el)+1;

        out_idx_ =  new Node*[num_node_+1];
        out_nodelist_ =  new Node[nodelist_size_];
        in_idx_ =  new Node*[num_node_+1];
        in_nodelist_ =  new Node[nodelist_size_];

        makeCSR(el, out_idx_, out_nodelist_, false);
        makeCSR(el, in_idx_, in_nodelist_, true);
    }

    csrgraph(std::vector<Edge> &el, std::vector<snapshot<Node>*> &snapshots){
        num_node_ =  find_max_node(el)+1;
        size_t cur_num_node = find_current_num_node(el);
        nodelist_size_ = el.size() + 2*cur_num_node;

        out_idx_ =  new Node*[num_node_+1];
        out_nodelist_ =  new Node[nodelist_size_];
        in_idx_ =  new Node*[num_node_+1];
        in_nodelist_ =  new Node[nodelist_size_];

        makeCSR(el, out_idx_, out_nodelist_, false, snapshots);
        makeCSR(el, in_idx_, in_nodelist_, true, snapshots);
    }


    ~csrgraph(){
        if(out_idx_ != nullptr) delete[] out_idx_;
        if(out_nodelist_ != nullptr) delete[] out_nodelist_;
        if(in_idx_ != nullptr) delete[] in_idx_;
        if(in_nodelist_ != nullptr) delete[] in_nodelist_;
    };

    int64_t num_nodes() const{ return num_node_;}
    int64_t nodelist_size(){ return nodelist_size_;}
    size_t out_degree(Node n) const { return out_idx_[n+1]-out_idx_[n];}
    size_t in_degree(Node n) const { return in_idx_[n+1]-in_idx_[n];}
    size_t out_offset(Node n) const { return out_idx_[n] - out_idx_[0];}
    size_t in_offset(Node n) const { return in_idx_[n] - in_idx_[0];}
    Node* out_idx(Node n){ return out_idx_[n];}
    Node* in_idx(Node n){ return in_idx_[n];}

    Node* edge_table(){
        Node* rt = out_nodelist_;
        out_nodelist_ = nullptr;
        return rt;
    }

    void set_out_record(size_t i, Node snap_id, Node offset){
        out_nodelist_[i] = snap_id;
        out_nodelist_[i+1] = offset;
    }
};

#endif