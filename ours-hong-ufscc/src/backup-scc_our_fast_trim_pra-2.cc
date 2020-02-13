#include "gm.h"
#include "scc.h"
#include "my_work_queue.h"
#include <sys/resource.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <scc_our.h>

using namespace std;

#define atomic_read(v)      (*(volatile typeof(*v) *)(v))
#define atomic_write(v,a)   (*(volatile typeof(*v) *)(v) = (a))
#define fetch_or(a, b)      __sync_fetch_and_or(a,b)
#define cas(a, b, c)        __sync_bool_compare_and_swap(a,b,c)
#define atomic_add(a, b)        __sync_fetch_and_add(a, b);
/*sequential*/
extern edge_t* support_begin;
extern edge_t* support_end;
extern support_t* support_idx; //CSR format support edges
extern int* color;
//stack_array *stack; //propogation queue


/*parallel*/
static int* counter;

Count g_cnt;


/*debug random vertex.*/
//#define DEBUG_RAND_VERTEX
#ifdef DEBUG_RAND_VERTEX
vector<int> shuffle_vertex;
#endif 

static queue_array *Q[32]; 
void initialize_our_pra_trim(gm_graph &G){
    int p = gm_rt_get_num_threads();
    for(int i = 0; i < p; i++){
         Q[i] = new queue_array(G_num_nodes);
    }
#ifdef DEBUG_RAND_VERTEX
    for(int i = 0; i < G_num_nodes; i++){
        shuffle_vertex.push_back(i);
    }
    random_shuffle(shuffle_vertex.begin(), shuffle_vertex.end());
    // for(int i = 0; i < 100; i++){
    //     printf("random: %d\n", shuffle_vertex[i]);
    // }

#endif 
}

void finalize_our_pra_trim(){
    int p = gm_rt_get_num_threads();
    for(int i = 0; i < p; i++){
        delete Q[i];
    }
}

/*for edge v->w, add a support of w, idx is the edge index of v->w.
this is a concurrent support append.*/
inline static void pra_support_append(node_t w, node_t v, int edge_idx_v_w, queue_array *q){
    int *end = &support_end[w];
    support_t *s = &support_idx[*end];
    /*this while loop most only run once since cas and atomic_add 
    are very close, and in the worst case, it travel all of the post. */
    while(true){
        if(cas(&s->v, -1, v)){
            atomic_add(end, 1);
            s->edge_idx = edge_idx_v_w;
            break;
        }
        s++;
    }
    
    if(COMPLETE == color[w]){
        q->push(w);
    }
}

/*input v; output w, idx. concurrent*/
inline static bool pra_get_fisrt_post(gm_graph &G, node_t v, node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w = G.begin[v]; edge_idx_v_w < G.begin[v+1]; edge_idx_v_w++) 
    {
        //cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v!=w) { //conditional and to avoid the self-loop
            return true; 
        }   
    }
    return false;
}

/*input v, idx; output w and renewed idx*/
inline static bool pra_get_next_post(gm_graph &G, node_t v,  node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w++; edge_idx_v_w < G.begin[v+1] ; edge_idx_v_w++) 
    {
        //cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v != w) {
            return true;
        }
    }
    return false;
}

/*our concurrent fast trim. Initialize stage.*/
void do_our_concurrent_fast_trim1_init(gm_graph &G){
    #pragma omp parallel
    {
        int thread_id = gm_rt_thread_id();
        queue_array *q = Q[thread_id];
        Count *cnt = new Count();
        node_t v, w, edge_idx_v_w;

        /*dynamic to keep the work balance for each queue.*/
        #pragma omp for schedule(dynamic, 1024)
        for(v = 0; v < G_num_nodes; v++)
        { 
            if(DEAD == color[v]) continue; // this is not necessary.         
            bool r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
            if (false == r){
                color[v] = DEAD;
                q->push(v);
                cnt->delete_v++;
            }
            else{
                pra_support_append(w, v, edge_idx_v_w, q);
            }
            
        }
        
        #pragma critical
        {
            printf("q.push: %d\n", q->push_num());
        }

        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        delete cnt;
    }
    
        
}

/*our concurrent fast trim. Propagation stage.*/
void do_our_concurrent_fast_trim1_prop(gm_graph &G){
    #pragma omp parallel
    {
        int thread_id = gm_rt_thread_id();
        queue_array *q = Q[thread_id];
        node_t w, w2; 
        Count *cnt = new Count();

        //propogation
        while(!q->empty()){
            w = q->top();
            q->pop();
            
            //for each (v, edge_idx_v_w) as support of w.
            for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                support_t &s = support_idx[k_idx];
                if(-1 == s.v || DEAD == color[s.v]) continue;
                bool r = pra_get_next_post(G, s.v, w2, s.edge_idx);
                if(false == r){
                    color[s.v] = DEAD;
                    q->push(s.v);
                    cnt->delete_v++;
                    //printf("dead: %d\n", s.v);
                }else{
                    //printf("support add: %d -> %d\n", w2, s.v);
                    pra_support_append(w2, s.v, s.edge_idx, q);
                }
                s.v = -1;  //remove
            }
            color[w] = COMPLETE;
        }

        #pragma critical
        {
            printf("q.push: %d\n", q->push_num());
        }

        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        delete cnt;
    }

}

/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_concurrent_fast_trim1_together(gm_graph &G){
    //initialization
    #pragma omp parallel
    {
        int thread_id = gm_rt_thread_id();
        queue_array *q = Q[thread_id];
        Count *cnt = new Count();
        node_t v, w, w2, edge_idx_v_w;

        /*dynamic to keep the work balance for each queue.*/
        #pragma omp for nowait schedule( dynamic, 1024 )
        for(v = 0; v < G_num_nodes; v++)
        {    
            if(LIVE != color[v]) continue; // this is not necessary.         
            bool r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
            if (false == r){
                color[v] = DEAD;
                q->push(v);
                cnt->delete_v++;
            }
            else{
                pra_support_append(w, v, edge_idx_v_w, q);
            }

 #if 1  // here for debug.      
            //propogation
            while(!q->empty()){
                w = q->top();
                
                color[w] = COMPLETE; // ??? is here ok?
                q->pop();
               
                //for each (v, edge_idx_v_w) as support of w.
                for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                    support_t &s = support_idx[k_idx];
                    if(-1 == s.v) continue;
                    if(LIVE != color[s.v]) continue;
                    bool r = pra_get_next_post(G, s.v, w2, s.edge_idx);
                    if(false == r){
                        color[s.v] = DEAD;
                        q->push(s.v);
                        cnt->delete_v++; 
                    }else{
                        pra_support_append(w2, s.v, s.edge_idx, q);
                    }
                    s.v = -1;  //??? remove should be atomic.
                }

            }
#endif
        }

#if 1   
        #pragma critical
        {
            printf("q.push: %d\n", q->push_num());
        }
#endif 
        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        delete cnt;
    }


}
