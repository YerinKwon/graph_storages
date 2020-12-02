#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

#include <vector>
#include "page.h"

template <typename Node>
struct snapshot{
    std::vector<page*> indirection_table;
    Node* edge_table;
    snapshot(std::vector<page*> &i_t, Node* e_t) : edge_table(e_t){
        int num_pages = i_t.size();
        indirection_table.reserve(num_pages);
        for(int i=0;i<num_pages;++i)
            indirection_table[i] = i_t[i];
    }
}; 

#endif