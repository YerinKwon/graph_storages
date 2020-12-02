# graph_storages

## LLAMA    
### How to run   
    g++ -fopenmp -std=c++14 llama/main.cpp -o llama.out   
    ./llama.out [input file path]   

### Result(example)   
init   
make graph   
make indir table   
indir init   
pushback   
==SNAPSHOT 1==   
-page 0-   
id: -1 | offset: 0 | fragment length: 0   
id: -1 | offset: 0 | fragment length: 0   
-page 1-   
id: 0 | offset: 0 | fragment length: 1   
id: -1 | offset: 0 | fragment length: 0   


## Chronos    
5 snapshots, each has 60%,70%,80%,90%,100% of original edges   

### How to run
    g++ -fopenmp -std=c++14 chronos/main.cpp -o chronos.out   
    ./chronos.out [input file path]   

### Result(example)   
make vertex array   
make edge array   
start pagerank   
0   
snapshot 0: 0.833356   
snapshot 1: 0.847656   
snapshot 2: 0.881757   
snapshot 3: 0.919904   
snapshot 4: 0.959412   
snapshot 5: 1.019261   
1   
snapshot 0: 0.196420   
snapshot 1: 0.199912   
snapshot 2: 0.214153   
...   
9   
snapshot 0: 0.000211   
snapshot 1: 0.000781   
snapshot 2: 0.002300   
snapshot 3: 0.004626   
snapshot 4: 0.006475   
snapshot 5: 0.000042   
Time:    0.005233   
