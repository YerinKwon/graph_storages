#ifndef GRAPH_MANAGER_H_
#define GRAPH_MANAGER_H_

#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <iostream>

#include "csrgraph.h"
#include "snapshot.h"
#include "page.h"
#include "vertex_record.h"

using namespace std;

#define PG_IDX(x) ((x)>>M)
#define VT_IDX(x) ((x) & (PAGE_SIZE - 1))

template <typename Node>
class graph_manager{
    typedef pair<Node,Node> Edge;

    vector<snapshot<Node>*> snapshots;
    vector<page*> indir_table;

public:
    graph_manager(){}

    ~graph_manager(){
        for(auto &sn: snapshots)
            delete sn;
        for(auto &pg: indir_table)
            if(pg != nullptr) delete pg;
    }

    void init_graph(vector<Edge> &el){
        cout<<"init\n";
        csrgraph<Node> g(el);
        cout<<"make graph\n";
        int num_pages = (g.num_nodes() + PAGE_SIZE - 1) / PAGE_SIZE;

        indir_table.reserve(num_pages);
        for(int i=0; i<num_pages; ++i)
            indir_table.push_back(new page(snapshots.size()));
        cout<<"make indir table\n";

        for(Node n=0; n<g.num_nodes();++n)
            if(g.out_degree(n) > 0)
                (*indir_table[PG_IDX(n)])[VT_IDX(n)].set_record(snapshots.size(), g.out_offset(n), g.out_degree(n));
        cout<<"indir init\n";

        snapshots.push_back(new snapshot<Node>(indir_table, g.edge_table()));
        cout<<"pushback\n";
    }
    
    void add_snapshot(vector<Edge> &el){
        csrgraph<Node> g(el, snapshots);
        int num_pages = (g.num_nodes() + PAGE_SIZE - 1) / PAGE_SIZE;
        if(num_pages > indir_table.size()) 
            indir_table.resize(num_pages, nullptr);

        for(Node n=0; n<g.num_nodes();++n){
            if(g.out_degree(n) > 0){
                if(indir_table[PG_IDX(n)] == nullptr)
                    indir_table[PG_IDX(n)] = new page(snapshots.size());
                else if((*indir_table[PG_IDX(n)]).id != snapshots.size()) 
                    indir_table[PG_IDX(n)] = new page(*indir_table[PG_IDX(n)],snapshots.size());
                vertex_record &cur_vertex = (*indir_table[PG_IDX(n)])[VT_IDX(n)];
                g.set_out_record(g.out_offset(n)+g.out_degree(n)-2, cur_vertex.snapshot_id(), cur_vertex.offset());
                cur_vertex.set_record(snapshots.size(), g.out_offset(n), g.out_degree(n)-2);
            }
        }

        snapshots.push_back(new snapshot<Node>(indir_table, g.edge_table()));
    }

    void print_graph(){
        printf("==SNAPSHOT %ld==\n", snapshots.size());
        for(int i=0; i < indir_table.size();++i){
            printf("-page %d-\n", i);
            for(int j=0;j<PAGE_SIZE;++j) (*indir_table[i])[j].printrecord();
        }
        printf("\n");
    }

};

#endif