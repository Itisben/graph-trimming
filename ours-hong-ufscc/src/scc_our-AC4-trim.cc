
#include "gm.h"
#include "scc.h"
#include "my_work_queue.h"
#include <sys/resource.h>
#include <assert.h>
#include <vector>
#include "scc_our.h"

using namespace std;

extern int8_t *color;

static int *G_in = NULL;
static int *G_out = NULL;
static int global_dead_cnt = 0;
void initialize_dcscc_trim(gm_graph &G){
    G_in = new int[G_num_nodes];
    G_out = new int[G_num_nodes];

    for (node_t i = 0; i < G_num_nodes; i++)
    {
        color[i] = LIVE;
        G_out[i] = G.begin[i+1] - G.begin[i];
        G_in[i] = G.r_begin[i+1] - G.r_begin[i];
    }
} 

void finalize_dcscc_trim(){
    delete [] G_in;
    delete [] G_out;
}

/*take patent graph for example. 
This trim visit the edges and reverse edges together. So that the running time is double compared with sequential tarjan's algorithm.*/
void static inline do_trim_graph(gm_graph &G, stack_array &stack, int &dead_cnt){
    while(!stack.empty()){
        node_t v = stack.top(); stack.pop();
        for(int idx = G.begin[v]; idx < G.begin[v+1]; idx++){
            node_t w = G.node_idx[idx];
            if(DEAD != color[w]){

                #pragma omp atomic
                G_in[w]--;
                
                if(0 == G_in[w]){
                    color[w] = DEAD;
                    dead_cnt++;
                    stack.push(w);
                }
            }
        }

        for(int idx = G.r_begin[v]; idx < G.r_begin[v+1]; idx++){
            node_t w = G.r_node_idx[idx];
            if(DEAD != color[w]){

                #pragma omp atomic
                G_out[w]--;

                if(0 == G_out[w]){
                    color[w] = DEAD;
                    dead_cnt++;
                    stack.push(w);
                }

            }
        }
    }
    
}

void do_trim_graph_all(gm_graph &G){
    #pragma omp parallel
    {
        stack_array *stack = new stack_array(G_num_nodes);
        int dead_cnt = 0;
        #pragma omp for //schedule (dynamic, 128) 
        for(node_t i = 0; i < G_num_nodes; i++){
            if (LIVE == color[i])
            {
                if(0 == G_in[i] || 0 == G_out[i]) {
                    color[i] = DEAD;
                    stack->push(i);
                    
                    dead_cnt++;
                }
            }
        }

        //#pragma omp for schedule (dynamic, 128)

        do_trim_graph(G, *stack, dead_cnt);
                
        #pragma omp critical
        {
            global_dead_cnt += dead_cnt;
        }
        delete stack;
    }

    printf("Our Trim: delete %d\n", global_dead_cnt);
}