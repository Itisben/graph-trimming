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

//#define DEBUG
#define ASSERT(a) //assert(a)
#define OUTSIDE_Q
#define PARALLEL_STEP 1024 * 8 /*parallel dynamic size, which is best?*/

/*sequential*/
extern edge_t* support_begin;
extern edge_t* support_end;
extern support_t* support_idx; //CSR format support edges
extern int* color;


inline bool is_trim_dead(node_t v){
    return LIVE != color[v]; 
}

/*parallel*/
static int* counter;

Count g_cnt;


static queue_array *Q[32]; 
void initialize_our_pra_trim(gm_graph &G)
{

#ifdef OUTSIDE_Q
    /*out side q for debug*/
    int p = gm_rt_get_num_threads();
    for(int i = 0; i < p; i++){
         Q[i] = new queue_array(G_num_nodes);
    }
#endif 
}

void finalize_our_pra_trim()
{

#ifdef OUTSIDE_Q
    int p = gm_rt_get_num_threads();
    for(int i = 0; i < p; i++){
        delete Q[i];
    }
#endif

}

/*for edge v->w, add a support of w, idx is the edge index of v->w.
this is a concurrent support append.*/
inline static void pra_support_append(node_t w, node_t v, int edge_idx_v_w, queue_array *q){
    int *end = &support_end[w];
    //support_t *s = &support_idx[support_begin[w]]; /*search from the start*/
    support_t *s = &support_idx[*end];
    while(true){
        if(cas(&s->v, -1, v)){
            /*note that we should keep here to be syc.*/
            atomic_write(&s->edge_idx, edge_idx_v_w);  
            atomic_add(end, 1);
            ASSERT(*end <= support_begin[w+1]);

            /*here is hard to parallel, I need a better solution.*/
            //if(PROPAGATE == atomic_read(&color[w]))
            if (LIVE != atomic_read(&color[w]))
            {
                /*redead: push into Q again.*/
                q->push(w);
            }
            return;
        }
        s++;
    }

}

/*input v; output w, idx. concurrent*/
inline static bool pra_get_fisrt_post(gm_graph &G, node_t v, node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w = G.begin[v]; edge_idx_v_w < G.begin[v+1]; edge_idx_v_w++) 
    {
        //cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == atomic_read(&color[w]) && v!=w) { //conditional and to avoid the self-loop
            return true; 
        }   
    }
    return false;
}

/*input v, idx; output w and renewed idx*/
inline static bool pra_get_next_post(gm_graph &G, node_t v,  node_t &w, int &edge_idx_v_w){
    /*loop condition: v+1 may out of the bound!!!, this is fixed in initialization part*/
    edge_idx_v_w++;

    ASSERT(edge_idx_v_w >= G.begin[v]);
    ASSERT(edge_idx_v_w <= G.begin[v+1] );
    if (edge_idx_v_w < G.begin[v]) return false;
    if (edge_idx_v_w >= G.begin[v+1]) return false;

    while(edge_idx_v_w < G.begin[v+1]){
        w = G.node_idx[edge_idx_v_w];
        if(LIVE ==  atomic_read(&color[w]) && v != w) {
            return true;
        }
        edge_idx_v_w++;
    }
    return false;
}

void concurrent_fast_trim1_check(); //debug.

/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_concurrent_fast_trim1_together(gm_graph &G){
    #pragma omp parallel
    {
        
        #ifdef OUTSIDE_Q
        int thread_id = gm_rt_thread_id();
        queue_array *q = Q[thread_id];
        #else
        queue_array *q =  new queue_array(G_num_nodes); //Q[thread_id];
        #endif
        
        Count *cnt = new Count();
        node_t v, w, w2, edge_idx_v_w;

        /*dynamic to keep the work balance for each queue. how loarge the dynamic area is a problem. 
        nomally is 1024 * 8 */
        #pragma omp for schedule(dynamic, PARALLEL_STEP) /*no wait can not be here.*/
        for(v = 0; v < G_num_nodes; v++)
        {   
            if(LIVE != atomic_read(&color[v]))  continue;

            
            bool r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
            if (likely(r)) 
            {   
                /*w should be LIVE, but it may be dead so that this 
                *support may not be processed. so we need to push into Q again.*/
                pra_support_append(w, v, edge_idx_v_w, q);
            }
            else{
                /*v can only be push into sigle one queue.
                * if v is DEAD, do nothing.*/
                if(cas(&color[v], LIVE, DEAD)) 
                {
                    q->push(v);
                    cnt->delete_v++;
                  
                }
                else{
                    continue;
                }

            }

            
 
 #if 1 /*propogation*/
           
            while(!q->empty())
            {
                w = q->top();
                q->pop();

                //atomic_write(&color[w], PROPAGATE);

                /*for each (v, edge_idx_v_w) as support of w.
                * here the support_end need sychronization since it may not be the real end.*/
                //for(int k_idx = support_begin[w]; k_idx < support_begin[w+1]; k_idx++)
                int end = atomic_read(&support_end[w]);
                for(int k_idx = support_begin[w]; k_idx < end; k_idx++)
                {

                    BEGIN:
                    node_t v = atomic_read(&support_idx[k_idx].v); 
                    if(v < 0) continue;

                    if (false == cas(&support_idx[k_idx].v, v, -2)) {
                        continue;
                    }

                    if(LIVE != atomic_read(&color[v])) continue;
                   

                    /*if edge_idx is not yet update, the next step is wrong.*/
                    AGAIN:
                    int edge_idx = atomic_read(&support_idx[k_idx].edge_idx); //???               
                    if (edge_idx < 0) goto AGAIN;
                    ASSERT(edge_idx >=G.begin[v]);
                    ASSERT(edge_idx < G.begin[v+1]);
                    


                    bool r = pra_get_next_post(G, v, w2, edge_idx);
                    if(likely(r)){
                        pra_support_append(w2, v, edge_idx, q);
                    }else{
                        if(cas(&color[v], LIVE, DEAD)) 
                        {

                            q->push(v);
                            cnt->delete_v++;
                                     
                        }                     

                    }
                }

            }
#endif
        }

        #pragma critical
        {
            #ifdef DEBUG
            printf("q.push: %d, size: %d\n", q->push_num(), q->size());
            #endif
            g_cnt.push += q->push_num();
        }

        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        #ifndef OUTSIDE_Q
        delete q;
        #endif

        delete cnt;
    }
#ifdef DEBUG
    concurrent_fast_trim1_check();
#endif
}

void propagation(gm_graph &G, node_t v, int thread_id){
    queue_array *q = Q[thread_id];
    if(cas(&color[v], LIVE, DEAD))
    {
        q->push(v);
    }
    
    return;

    while(!q->empty())
    {
        node_t w = q->top();
        q->pop();

        //atomic_write(&color[w], PROPAGATE);

        /*for each (v, edge_idx_v_w) as support of w.
        * here the support_end need sychronization since it may not be the real end.*/
        //for(int k_idx = support_begin[w]; k_idx < support_begin[w+1]; k_idx++)
        int end = atomic_read(&support_end[w]);
        for(int k_idx = support_begin[w]; k_idx < end; k_idx++)
        {

            BEGIN:
            node_t v = atomic_read(&support_idx[k_idx].v); 
            if(v < 0) continue;

            if (false == cas(&support_idx[k_idx].v, v, -2)) {
                continue;
            }

            if(LIVE != atomic_read(&color[v])) continue;
            

            /*if edge_idx is not yet update, the next step is wrong.*/
            AGAIN:
            int edge_idx = atomic_read(&support_idx[k_idx].edge_idx); //???               
            if (edge_idx < 0) goto AGAIN;
            ASSERT(edge_idx >=G.begin[v]);
            ASSERT(edge_idx < G.begin[v+1]);
            

            int w2;
            bool r = pra_get_next_post(G, v, w2, edge_idx);
            if(likely(r)){
                pra_support_append(w2, v, edge_idx, q);
            }else{
                if(cas(&color[v], LIVE, DEAD)) 
                {
                    q->push(v);
                    //cnt->delete_v++;
                    #pragma atomic
                    g_cnt.delete_v++;           
                }                     

            }
        }

    }
}


/*for debug*/
void concurrent_fast_trim1_check(){
    for (node_t w = 0; w < G_num_nodes; w++){
        if(0 == atomic_read(&color[w])) continue;

        //node_t w = 31;
        //printf("begin %d, end %d\n", support_begin[w], support_end[w]);
        bool has = false;
        for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
            int v = atomic_read(&support_idx[k_idx].v);
            if(v >=0){
                has = true;
            }
        }
        if (true == has) {

            for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                int v = atomic_read(&support_idx[k_idx].v);
                {
                    printf("todo v %d, color %d, support %d, color %d, begin %d, end %d, next %d, idx %d\n", 
                    w, color[w], v, color[v], support_begin[w], support_end[w], support_begin[w+1], k_idx);
                    ASSERT(support_end[w] <= support_begin[w+1]);
                }
            }
        }
    }
}