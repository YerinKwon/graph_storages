#include "graph_manager.h"

typedef int64_t Node;
typedef pair<Node,Node> Edge;

#define CHUNK_SIZE 1<<10

int main(int argc, char **argv){
    graph_manager<Node> manager;
    freopen(argv[1],"rt",stdin);
    int ret = 0;
    for(int snap_id = 0; ret != -1; ++snap_id){
        //load file
        vector<Edge> el;
        int u,v,cnt = 0;
        while((ret = scanf("%d %d",&u,&v)) !=-1 && cnt++ < CHUNK_SIZE) el.push_back({u,v});
        if(snap_id == 0) manager.init_graph(el);
        else manager.add_snapshot(el);
        manager.print_graph();
    }
}