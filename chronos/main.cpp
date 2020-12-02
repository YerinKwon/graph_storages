#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>

#include "csrgraph.h"
#include "timer.h"

using namespace std;

#define NUM_SN 6

typedef float datatype;

struct edge{
    int target;
    int32_t bitmap;
    edge(int t):target(t),bitmap(0){}
    edge(int t, int num):edge(t){
        bit_on(num);
    }
    void bit_on(int num){
        // bitmap |= (1<<(31-num)); //논문표현
        bitmap |= 1<<num; //프린트 편의상
    }

    bool is_set(int num){
        return (bitmap & (1<<num)) != 0;
    }

    void print_edge(int num_snapshot = 32){
        printf("target: %d\tbitmap: ", target);
        for(int i=0;i<num_snapshot;++i) printf("%d",(bitmap>>i)&1);
        printf("\n");
    }

    bool operator==(const edge &other) const{ return target == other.target;}
    bool operator!=(const edge &other) const{ return !(*this == other);}
};

int main(int argc, char** argv){
    // command line parsing
    string filename = argv[1];
    ifstream in(filename);
    if(!in.is_open()) {
        cout<<"No such input file"<<endl;
        return 0;
    }

    // on-disk design : csrgraph로 대체
    int u,v;
    vector<pair<int,int>> e,el;
    while(in>>u>>v) e.push_back({u,v});
    int block_size = (e.size()+9)/10, max_size = e.size();
 
    int cur_idx = 0, max_v = 0;
    CSRGraph<int> g[NUM_SN];
    for(int i=0;i<NUM_SN;++i){
        int cur_size = min(max_size, block_size*(NUM_SN-1+i));
        while(cur_idx<cur_size) el.push_back(e[cur_idx++]);
        g[i] = CSRGraph<int>(el);
        if(max_v < g[i].num_nodes()) max_v = g[i].num_nodes();
    }

    // in memory design
    vector<vector<datatype>> vertex_array_cur(max_v),vertex_array_update(max_v);
    vector<vector<edge>> edge_array_in(max_v),edge_array_out(max_v);

    cout<<"make vertex array\n";
    #pragma omp parallel for
    for(int i=0;i<max_v;++i) {
        vertex_array_cur[i].resize(NUM_SN);
        vertex_array_update[i].resize(NUM_SN);
    }

    cout<<"make edge array\n"; 
    for(int i=0;i<NUM_SN;++i){
        #pragma omp parallel for
        for(int j=0;j<g[i].num_nodes();++j){
            for(const auto &u: g[i].in_neigh(j)){
                auto it = find(edge_array_in[j].begin(), edge_array_in[j].end(), u);
                if(it == edge_array_in[j].end()) edge_array_in[j].push_back(edge(u,i));
                else it->bit_on(i);
            }
            for(const auto &v: g[i].out_neigh(j)){
                auto it = find(edge_array_out[j].begin(), edge_array_out[j].end(),v);
                if(it == edge_array_out[j].end()) edge_array_out[j].push_back(edge(v,i));
                else it->bit_on(i);
            }
        }
    }

    // pagerank example
    cout<<"start pagerank"<<endl;
    const float df = 0.85;
    const double epsilon = 0.0001;
    const int max_iter = 20;

    Timer t;
    t.Start();

    datatype init_score[NUM_SN];
    datatype base_score[NUM_SN];
    for(int i=0;i<NUM_SN;++i) 
        init_score[i] = 1.0f / g[i].num_nodes(), base_score[i] = (1.0f - df) / g[i].num_nodes();

    vector<vector<int>> out_degree(max_v);
    vector<vector<datatype>> out_contrib(max_v);
    #pragma omp parallel for
    for(int i=0;i<max_v;++i) out_degree[i].resize(NUM_SN,0), out_contrib[i].resize(NUM_SN);

    #pragma omp parallel for
    for(int n=0;n<max_v;++n){
        for(auto edge: edge_array_out[n]){
            for(int s=0;s<NUM_SN;++s){
                if(edge.is_set(s)) ++out_degree[n][s];
            }
        }
    }

    #pragma omp parallel for
    for(int n=0;n<max_v;++n){
        for(int s=0;s<NUM_SN;++s){
            vertex_array_cur[n][s] = init_score[s];
        }
    }

    for(int iter = 0;iter<max_iter;++iter){
        vector<double> errors(NUM_SN,0);
        #pragma omp parallel for
        for(int v=0;v<max_v;++v)
            for(int s=0;s<NUM_SN;++s)
                out_contrib[v][s] = vertex_array_cur[v][s]/out_degree[v][s];

        #pragma omp parallel for schedule(dynamic, 64)
        for(int v=0;v<max_v;++v){
            vector<double> local_errors(NUM_SN,0);
            vector<datatype> incoming_totals(NUM_SN,0);

            for(auto edge: edge_array_in[v]){
                int u = edge.target;
                for(int s = 0;s<NUM_SN;++s){
                    if(edge.is_set(s))
                        incoming_totals[s] += out_contrib[u][s];
                }
            }

            for(int s=0;s<NUM_SN;++s){
                if(v < g[s].num_nodes()){
                    vertex_array_update[v][s] = base_score[s] + df*incoming_totals[s];
                    local_errors[s] += fabs(vertex_array_update[v][s] - vertex_array_cur[v][s]);
                }
            }

            #pragma omp critical
            {
                for(int s=0;s<NUM_SN;++s) errors[s] += local_errors[s];
            }
        }
        cout<<iter<<endl;
        bool terminate = true;
        for(int s=0;s<NUM_SN;++s) {
            printf("snapshot %d: %lf\n",s,errors[s]);
            // if(errors[s] >= epsilon) terminate = false;
        }
        // if(terminate) break;
        if(errors[NUM_SN-1] < epsilon) break;

        vertex_array_cur.swap(vertex_array_update);
    }
    t.Stop();
    printf("Time: \t %lf\n",t.Seconds());
}