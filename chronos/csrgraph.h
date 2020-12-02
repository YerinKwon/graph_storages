#ifndef CSRGRAPH_H_
#define CSRGRAPH_H_

#include <vector>
#include <algorithm>
#include <utility>
#include <cstdio>
#include <iostream>

typedef int64_t Offset;

template <typename Node>
class CSRGraph{
    typedef std::pair<Node,Node> Edge;
public:
    int64_t num_nodes_;
    int64_t num_edges_;
    Node** out_index_;
    Node* out_neighbors_;
    Node** in_index_;
    Node* in_neighbors_;

    typedef std::make_unsigned<std::ptrdiff_t>::type OffsetT;

    class Neighborhood {
        Node n_;
        Node** g_index_;
        OffsetT start_offset_;
    public:
        Neighborhood(Node n, Node** g_index, OffsetT start_offset) :
            n_(n), g_index_(g_index), start_offset_(0) {
            OffsetT max_offset = end() - begin();
            start_offset_ = std::min(start_offset, max_offset);
        }
        typedef Node* iterator;
        iterator begin() { return g_index_[n_] + start_offset_; }
        iterator end()   { return g_index_[n_+1]; }
    };

    Node find_max_node(std::vector<Edge> &el){
        Node max_node = 0;
        #pragma omp parallel for reduction(max: max_node)
        for(auto iter = el.begin(); iter<el.end(); ++iter){
            Node mx = std::max(iter->first, iter->second);
            max_node = std::max(max_node, mx);
        }
        return max_node;
    }

    Node* count_degree(std::vector<Edge> &el, bool inv){
        Node* degree = new Node[num_nodes_];
        #pragma omp parallel for
        for(int i=0;i<num_nodes_;++i) degree[i] = 0;

        #pragma omp parallel for
        for(auto iter = el.begin();iter<el.end();++iter){
            if(inv) __sync_fetch_and_add(&degree[iter->second],1);
            else __sync_fetch_and_add(&degree[iter->first],1);
        }
        return degree;
    }

    Offset* count_offset(Node* degree){
        int64_t block_size = 1<<20;
        int64_t num_block = (num_nodes_ + (block_size-1))/block_size;

        Offset* offset = new Offset[num_nodes_+1];
        #pragma omp parallel for
        for(int i=0;i<num_nodes_+1;++i) offset[i] = 0;

        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_nodes_);
            #pragma omp parallel for reduction(+:offset[end])
            for(int j=block_size*i; j<end;++j) offset[end] += degree[j];
        }

        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_nodes_);
            offset[end] += offset[block_size*i];
        }

        #pragma omp parallel for
        for(int i=0;i<num_block;++i){
            int end = std::min((i+1)*block_size, num_nodes_);
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
        for(int i=0;i<num_nodes_+1;++i)
            idx[i] = nodelist + offset[i];
    }

    void makeCSR(std::vector<Edge> &el, Node** idx, Node* nodelist, bool inv){
        Node *degree = count_degree(el, inv);
        Offset *offset = count_offset(degree);
        set_index(offset, idx, nodelist);

        for(auto iter = el.begin(); iter<el.end();++iter){
            if(inv) nodelist[__sync_fetch_and_add(&offset[iter->second],1)] = iter->first;
            else nodelist[__sync_fetch_and_add(&offset[iter->first],1)] = iter->second;
        }

        delete[] degree;
        delete[] offset;
    }

    void Release(){
        if(out_index_ != nullptr) delete[] out_index_;
        if(out_neighbors_ != nullptr) delete[] out_neighbors_;
        if(in_index_ != nullptr) delete[] in_index_;
        if(in_neighbors_ != nullptr) delete[] in_neighbors_;
    }

    CSRGraph(){
        num_edges_ = -1;
        num_nodes_ =  -1;
        out_index_ =  nullptr;
        out_neighbors_ =  nullptr;
        in_index_ =  nullptr;
        in_neighbors_ =  nullptr;   
    }

    CSRGraph(CSRGraph &&other) : num_edges_(other.num_edges_), 
    num_nodes_(other.num_nodes_), out_index_(other.out_index_),
    out_neighbors_(other.out_neighbors_), in_index_(other.in_index_),
    in_neighbors_(other.in_neighbors_){
        other.num_edges_ = -1;
        other.num_nodes_ =  -1;
        other.out_index_ =  nullptr;
        other.out_neighbors_ =  nullptr;
        other.in_index_ =  nullptr;
        other.in_neighbors_ =  nullptr;  
    }

    CSRGraph(std::vector<Edge> &el){
        num_edges_ = el.size();
        num_nodes_ = find_max_node(el)+1;

        out_index_ =  new Node*[num_nodes_+1];
        out_neighbors_ =  new Node[num_edges_];
        in_index_ =  new Node*[num_nodes_+1];
        in_neighbors_ =  new Node[num_edges_];

        makeCSR(el, out_index_, out_neighbors_, false);
        makeCSR(el, in_index_, in_neighbors_, true);
    }

    CSRGraph& operator=(CSRGraph&& other) {
        if (this != &other) {
            Release();
            num_edges_ = other.num_edges_;
            num_nodes_ = other.num_nodes_;
            out_index_ = other.out_index_;
            out_neighbors_ = other.out_neighbors_;
            in_index_ = other.in_index_;
            in_neighbors_ = other.in_neighbors_;
            other.num_edges_ = -1;
            other.num_nodes_ = -1;
            other.out_index_ = nullptr;
            other.out_neighbors_ = nullptr;
            other.in_index_ = nullptr;
            other.in_neighbors_ = nullptr;
        }
        return *this;
    }


    ~CSRGraph(){
        Release();
    };

    int64_t num_nodes() const{ return num_nodes_;}
    int64_t num_edges(){ return num_edges_;}
    int64_t out_degree(Node n) const { return out_index_[n+1]-out_index_[n];}
    int64_t in_degree(Node n) const { return in_index_[n+1]-in_index_[n];}
    Node* out_index(Node n){ return out_index_[n];}
    Node* in_index(Node n){ return in_index_[n];}
    Neighborhood out_neigh(Node n, OffsetT start_offset = 0) const {
        return Neighborhood(n, out_index_, start_offset);
    }

    Neighborhood in_neigh(Node n, OffsetT start_offset = 0) const {
        return Neighborhood(n, in_index_, start_offset);
    }
};

#endif