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
extern Count g_cnt;

static int *G_out = NULL;

void initialize_ac4_trim(gm_graph &G){
    G_out = new int[G_num_nodes];
} 

void finalize_ac4_trim(){
    delete [] G_out;
}


void static inline do_trim_graph(gm_graph &G, stack_array &stack, 
    Count *cnt){
        
    while(!stack.empty()){
        node_t v = stack.top(); stack.pop();
        for(int idx = G.r_begin[v]; idx < G.r_begin[v+1]; idx++){
            node_t w = G.r_node_idx[idx]; if (EMPTY == w) break; //don't igore the self-loop
           

            cnt->edge++; //counting all traversed edges.

            atomic_add(&G_out[w], -1);

            ASSERT(atomic_read(&G_out[w]) >=0);
            if(0 == atomic_read(&G_out[w])){
                if(cas(&color[w], LIVE, DEAD) ) {
                    cnt->dead++;
                    stack.push(w);

                }
            }
           
            
        }
    }
    
}

extern size_t SAMPLE_VER;
void do_trim_graph_all(gm_graph &G){
    #pragma omp parallel
    {
        Count *cnt = new Count();
        stack_array *stack = new stack_array(G_num_nodes);

        #pragma omp for schedule (dynamic, PARALLEL_STEP) 
        for (node_t i = 0; i < G_num_nodes; i++)
        {
            /*this way can not remove the self-loop, so that livej and wiki-talk-en may have problems, but that's find. */
            G_out[i] = 0;
            for(int idx = G.begin[i]; idx < G.begin[i+1]; idx++){
                node_t w = G.node_idx[idx]; if (EMPTY == w) break;
                G_out[i]++; 
            }

        }


        #pragma omp for schedule (dynamic, PARALLEL_STEP) 
        for(node_t i = 0; i < G_num_nodes; i++){
            ASSERT(atomic_read(&G_out[i]) >=0);

            if(LIVE == atomic_read(&color[i])) {
                if (0 == atomic_read(&G_out[i]) ) {
                    if(cas(&color[i], LIVE, DEAD) ) {
                        stack->push(i);
                        cnt->dead++;
                    }
                }
            

                do_trim_graph(G, *stack, cnt);
            }
        }

        
                
        #pragma omp critical
        {
            g_cnt.dead += cnt->dead;
            if (g_cnt.edge < cnt->edge) {g_cnt.edge = cnt->edge; }
            if (g_cnt.seq_v < cnt->seq_v) {g_cnt.seq_v = cnt->seq_v;}
            if (g_cnt.push < stack->get_max_stack()) {g_cnt.push = stack->get_max_stack();}
        }
        delete stack;
    }

    printf("Total Delete Vertex #: %d\n", g_cnt.dead);
    printf("total-traversed-e #: %d\n", g_cnt.edge); // maximum number of traversed edges.
    printf("Max stack push #: %d\n", g_cnt.push);
}