
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
      /*prallel*/
    counter = new int[G_num_nodes];

    #pragma omp parallel for
    for(int i = 0; i < G_num_nodes; i++){
        counter[i] = 0;
    }

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
    delete[] counter;

    int p = gm_rt_get_num_threads();
    for(int i = 0; i < p; i++){
        delete Q[i];
    }
}

/*for edge v->w, add a support of w, idx is the edge index of v->w.
this is a concurrent support append.*/
inline static void pra_support_append(node_t w, node_t v, int edge_idx_v_w){
    support_t *s;
    int *end;
    //note_t *v;
    //int
    while(true){
        end = &support_end[w];
        s = &support_idx[*end];
        //if(s->v == -1) 
        {
            if(true == cas(&s->v, -1, v))
            {
                // #pragma omp atomic
                support_end[w]++;

                atomic_add(end, 1);
                break;
            }
        }
    }

    s->edge_idx = edge_idx_v_w;

    //#pragma omp atomic
    //counter[w]--;

    atomic_add(&counter[w], -1);
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
        //printf("travel: %d->%d\n", v, w);
        //cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v != w) {
            
            // #pragma omp atomic
            // counter[w]++;

            atomic_add(&counter[w], 1);
            if (LIVE == color[w]) {
                return true;
            }else{
                // #pragma omp atomic
                // counter[w]--;
                atomic_add(&counter[w], -1);
            }
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
        for(int i = 0; i < G_num_nodes; i++){
            
            //if(DEAD == color[v]) continue; // this is not necessary.            
#ifdef DEBUG_RAND_VERTEX
            v = shuffle_vertex[i];
#else 
            v = i;
#endif 

#if 0
            bool r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
#else
            bool r = false;
            for (edge_idx_v_w = G.begin[v]; edge_idx_v_w < G.begin[v+1]; edge_idx_v_w++) 
            {
                w = G.node_idx[edge_idx_v_w];
                if(LIVE == color[w] && v!=w) { //conditional and to avoid the self-loop
                    r = true;
                    break; 
                }   
            }
#endif
            if (false == r){
                cnt->delete_v++;
                color[v] = DEAD;
                q->push(v);
            }
            else{
#if 0
            pra_support_append(w, v, edge_idx_v_w);
#else      
            support_t *s;
            while(true){
                int &end = support_end[w];
                s = &support_idx[end];
                if(true == cas(&s->v, -1, v))
                {
                    #pragma omp atomic
                    end++;
                    break;
                }
                
            }

            // support_t &s = support_idx[support_end[w]++];
            // s.v = v;
            // s.edge_idx = edge_idx_v_w;

            // s->edge_idx = edge_idx_v_w;
            // #pragma omp atomic
            // counter[w]--;
#endif
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
        bool r;
        Count *cnt = new Count();

        //propogation
        while(!q->empty()){
            w = q->top();
            q->pop();

            if (counter[w] > 0) {
                q->push2(w);
                continue;
            }
            
            //for each (v, edge_idx_v_w) as support of w.
            for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                support_t &s = support_idx[k_idx];
                if(DEAD == color[s.v]) continue;

                r = pra_get_next_post(G, s.v, w2, s.edge_idx);
                if(false == r){
                    color[s.v] = DEAD;
                    cnt->delete_v++;
                    q->push(s.v);
                    //printf("dead: %d\n", s.v);
                }else{
                    //printf("support add: %d -> %d\n", w2, s.v);
                    pra_support_append(w2, s.v, s.edge_idx);
                }   
            }
        }

        #pragma critical
        {
            printf("q.push: %d\n", q->push_num());
        }

        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;
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
        node_t v, w, w2;
        int edge_idx_v_w;
        bool r;
        /*dynamic to keep the work balance for each queue.*/
        #pragma omp for nowait schedule(dynamic,128)
        for(int i = 0; i < G_num_nodes; i++){ 
            //if(DEAD == color[v]) continue; 

#ifdef DEBUG_RAND_VERTEX
            v = shuffle_vertex[i];
#else 
            v = i;
#endif 

            r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
            if (false == r){
                cnt->delete_v++;
                color[v] = DEAD;
                q->push(v);
            }
            else{
                pra_support_append(w, v, edge_idx_v_w);
            }  
#if 0
            //propogation
            while(!q->empty()){
                node_t w = q->top();
                q->pop();

                if (counter[w] > 0) {
                    q->push2(w);
                    //printf("delay %d\n", w);
                    continue;
                }
                
                //for each (v, edge_idx_v_w) as support of w.
                for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                    support_t &s = support_idx[k_idx];
                    if(DEAD == color[s.v]) continue;

                    r = pra_get_next_post(G, s.v, w2, s.edge_idx);
                    if(false == r){
                        color[s.v] = DEAD;
                        cnt->delete_v++;
                        q->push(s.v);
                        //printf("dead: %d\n", s.v);
                    }else{
                        //printf("support add: %d -> %d\n", w2, s.v);
                        pra_support_append(G, w2, s.v, s.edge_idx);
                    }   
                }
            }
#endif
        }


        #pragma critical
        {
            printf("q.push: %d\n", q->push_num());
        }

        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        delete q;
        delete cnt;

    
    }

}
