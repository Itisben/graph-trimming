
//#include "gm.h"
//#include "scc.h"
//#include "my_work_queue.h"
#include <sys/resource.h>
#include <assert.h>
#include <vector>
#include "our-trim.h"
//#define ASSERT(a) assert(a)
#define ASSERT(A) 

using namespace std;

extern int* color;
extern node_t *next_begin;

static int global_dead_cnt = 0;
static long long global_edge_cnt = 0;

void static inline do_trim_step(gm_graph &G, int &step_dead_cnt, int &step_edge_cnt){
    #pragma omp parallel
    {
        int edge_cnt = 0;
        int dead_cnt = 0;
        #pragma omp for schedule (dynamic, PARALLEL_STEP) 
        for(node_t i = 0; i < G_num_nodes; i++){
            if (LIVE == (color[i])) {
                int degree = 0; edge_t k_idx;
                for (k_idx = atomic_read(&next_begin[i]); k_idx < G.begin[i+1] ; k_idx++) 
                {
                    node_t k = G.node_idx[k_idx]; if (EMPTY == k) break;
                    edge_cnt++; // traverse an existing edge
                    /* for simplisity, we can't igore the self-loop. 
                    //if (k==i) continue; //jump over self loop
                    */
                    if (LIVE == atomic_read(&color[k])){
                        degree = 1; 
                        
                        break;
                    }
                }

                next_begin[i] = k_idx; // skip edges next time.

                if (0 == degree) { // not find live.
                    atomic_write(&color[i], DEAD);   
                    dead_cnt++;
                }

                
            } 
        }

        #pragma omp critical
        {
            step_dead_cnt += dead_cnt;
            if (step_edge_cnt < edge_cnt) step_edge_cnt = edge_cnt; //max edge count for each worker
        }
    }
   
}

void do_ac3_repeat_trim_graph_all(gm_graph &G){
    int step_dead_cnt = 0;
    int step_edge_cnt = 0;
    int step = 0;
    do {
        step_dead_cnt = 0; step_edge_cnt = 0;
        do_trim_step(G, step_dead_cnt, step_edge_cnt);
        global_dead_cnt += step_dead_cnt;
        global_edge_cnt += step_edge_cnt;
        step++;
        //printf("ac-3 step dead cnt: %d, in step %d\n", step_dead_cnt, step);
    } while (step_dead_cnt > 0); 
    printf("trim-repeat-time: %d\n", step);
    printf("Total Delete Vertex #: %d\n", global_dead_cnt);
    printf("total-traversed-e #: %lld\n", global_edge_cnt); // maximum number of traversed edges.
}