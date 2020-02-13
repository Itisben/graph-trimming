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
    // int p = gm_rt_get_num_threads();
    // for(int i = 0; i < p; i++){
    //      Q[i] = new queue_array(G_num_nodes);
    // }
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
    // int p = gm_rt_get_num_threads();
    // for(int i = 0; i < p; i++){
    //     delete Q[i];
    // }
}


inline bool is_live(node_t v){
    return (LIVE  == atomic_read(&color[v]) );
}

/*for edge v->w, add a support of w, idx is the edge index of v->w.
this is a concurrent support append.*/
inline static void pra_support_append(node_t w, node_t v, int edge_idx_v_w, queue_array *q){

#if 0
    int *end = &support_end[w];
    support_t *s = &support_idx[*end];
    /*this while loop most only run once since cas and atomic_add 
    are very close, and in the worst case, it travel all of the post. */
    while(true){
        if(cas(&s->v, -1, v)){
            /*here we can avoid to use atomic_add!*/
            atomic_add(end, 1);
            s->edge_idx = edge_idx_v_w;
            //assert(*end <= support_begin[w+1]);
            break;
        }
        s++;
    }

    int *end = &support_end[w];
    support_t *s = &support_idx[*end];
    while(true){
        if(cas(&s->v, -1, v)){
            
            atomic_add(end, 1);
            if(LIVE != atomic_read(&color[w])){
                atomic_write(&color[w], DEAD);
                q->push(w);
                #if 1
                    if(support_end[w] - support_begin[w] < 1){
                        printf(">>> push again %d\n", w);
                    }
                #endif
            }
            break;
        }

        s++;
    }

#else

    support_t *s = &support_idx[support_begin[w]];
    while(true){
        if(cas(&s->v, -1, v)){
            atomic_write(&s->edge_idx, edge_idx_v_w);
            if(COMPLETE == atomic_read(&color[w])){
                atomic_add(&support_end[w], 1);
                atomic_write(&color[w], DEAD);
                q->push(w);
               
            }
            break;
        }

        s++;
    }

#endif

    #if 0

    if (w == 2){
        printf(">>>support_append %d: %d, beginidx %d, endidx %d, nextidx %d, thread %d \n",
            w, v, support_begin[w], support_end[w], support_begin[w+1], gm_rt_thread_id());
    }

    #endif

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
    //assert(edge_idx_v_w >= G.begin[v] && edge_idx_v_w <= G.begin[v+1]);
    if (edge_idx_v_w < G.begin[v] || edge_idx_v_w >= G.begin[v+1]) return false;
    while(edge_idx_v_w < G.begin[v+1]){
        w = G.node_idx[edge_idx_v_w];
        if(LIVE ==  atomic_read(&color[w]) && v != w) {
            #if 0
            if (2 == w) {
                printf(">>>>>%d has next %d\n", v, w);
            }
            #endif
            return true;
        }
        edge_idx_v_w++;
    }
    return false;
}

#if 0
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
#endif 

#if 1 //debug 
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
                }
            }
        }
    }
}

void show_invalid_check(gm_graph &G){
    #if 0
    #pragma omp parallel for
    for (node_t w = 0; w < G_num_nodes; w++){
     

        if(support_end[w] > support_begin[w+1])
        {
            printf("v %d, color %d, begin %d, end %d, next %d, idx %d\n", 
            w, color[w], color[w], support_begin[w], support_end[w], support_begin[w+1]);
        }
        
        
    }

    #pragma omp parallel for
    for (node_t w = 0; w < G_num_nodes; w++){
     

        for(int k_idx =G.begin[w]; k_idx < G.begin[w+1]; k_idx++)
        {
           if(2 == G.node_idx[k_idx]){
               printf(">>>node %d -> 2\n",w);
           }
        }
        
        
    }
    #endif 
    #pragma omp parallel for
    for (node_t w = 0; w < G_num_nodes; w++){
        if(atomic_read(&color[w]) == LIVE){
            // if(support_begin[w])
            //     printf(">>>live node %d, support begin %d, end %d, next %d\n", 
            //     w,  support_begin[w], support_end[w], support_begin[w+1]);
            
          
            for(int k_idx = G.begin[w]; k_idx < G.begin[w+1]; k_idx++)
            {
                printf(">>>live node %d, post idx %d, post %d, color %d \n",w, k_idx, G.node_idx[k_idx], color[G.node_idx[k_idx]]);
            }
            
           
        }
    }
    

}
#endif 




/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_concurrent_fast_trim1_together(gm_graph &G){
    //initialization
    #pragma omp parallel
    {
        int thread_id = gm_rt_thread_id();
        queue_array *q = new queue_array(G_num_nodes); //Q[thread_id];
        Count *cnt = new Count();
        node_t v, w, w2, edge_idx_v_w;

        /*dynamic to keep the work balance for each queue.*/
        //#pragma omp for nowait schedule( static, 1 )
        #pragma omp for nowait schedule( dynamic, 1024 )
        for(v = 0; v < G_num_nodes; v++)
        {    
            //#pragma omp critical 
            { 
            if(LIVE == atomic_read(&color[v])) {        
                bool r = pra_get_fisrt_post(G, v, w, edge_idx_v_w);
                if (false == r){
                    
                    atomic_write(&color[v], DEAD);

                    #if 1
                    if(31 == v) {
                        printf(">>>set %d to DEAD\n", v);
                        for(int k_idx = support_begin[v]; k_idx < support_end[v]; k_idx++){
                            support_t &s = support_idx[k_idx];
                            if(s.v >= 0) {
                                printf("%d: %d\n", v, s.v);
                            }
                        }
                        printf("<<<set %d to DEAD end \n", v);
                    }
                    #endif 

                    q->push(v);
                    cnt->delete_v++;
                }
                else{
            
                    pra_support_append(w, v, edge_idx_v_w, q);
                }

            }

            }//omp critical 
 #if 1  //here has bugs to make the segment falt.         
            //propogationdfasdf
            while(!q->empty()){
                w = q->top();
                q->pop();
                /*Do we need this ? not sure.*/
                

                /*for each (v, edge_idx_v_w) as support of w.
                * here the support_end need sychronization since it may not be the real end.*/
                //#pragma omp critical 
                {
                

                for(int k_idx = support_begin[w]; k_idx < support_begin[w+1]/*support_end[w]*/;
                    k_idx++){
                    
                    BEGIN:
                    support_t *s = &support_idx[k_idx];

                    node_t v = atomic_read(&s->v); 
                   



                    //printf(">>>propogat %d:%d\n", w, v);
                    if(-1 == v ) continue;
                    if(-2 == v) continue;


                    if (false == cas(&s->v, v, -1)) {
                        goto BEGIN;
                    }
                    
                    AGAIN:
                    int edge_idx = atomic_read(&s->edge_idx);
                    if(edge_idx < 0) goto AGAIN;
                    assert(edge_idx >= 0);

                    // if(LIVE != color[v]) {
                    //     /*Here is Ok.*/
                    //     //printf(">>>propogate not live v:%d\n", v);
                    //     continue;
                    // }
                    #pragma omp critical 
                    {
                    bool r = pra_get_next_post(G, v, w2, edge_idx);
                    if(false == r){
                        atomic_write(&color[v], DEAD);

                        #if 0
                         if(31 == v) {
                            printf(">>>set %d to DEAD thread %d -- \n", v, gm_rt_thread_id());
                            printf("begin %d, end %d\n", support_begin[v], support_end[v]);
                            for(int k_idx = support_begin[v]; k_idx < support_end[v]; k_idx++){
                                support_t &s = support_idx[k_idx];
                                if(s.v >= 0) {
                                    printf("%d: %d, idx %d\n", v, s.v, k_idx);
                                }
                            }
                            printf("<<<set %d to DEAD end -- \n", v);
                        }
                        #endif 

                        q->push(v);
                        cnt->delete_v++;
                    }else{
                        pra_support_append(w2, v, edge_idx, q);
                    }

                    
                    }
                }
                cas(&color[w], DEAD, COMPLETE);
            }
            }//omp critical
#endif
        }

#if 1   
        #pragma critical
        {
            printf("q.push: %d, size: %d\n", q->push_num(), q->size());
            g_cnt.push += q->push_num();
        }
#endif 
        #pragma omp atomic
        g_cnt.delete_v += cnt->delete_v;

        delete q;
        delete cnt;
    }

    concurrent_fast_trim1_check();
    //show_invalid_check(G);
}

