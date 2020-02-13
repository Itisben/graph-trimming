
#include "gm.h"
#include "scc.h"
#include "my_work_queue.h"
#include <sys/resource.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <scc_our.h>
using namespace std;

//#define DEBUG

edge_t* support_begin;
edge_t* support_end;
support_t* support_idx; //CSR format support edges
stack_array *stack; //propogation queue

edge_t* r_support_begin;
edge_t* r_support_end;
support_t* r_support_idx; //CSR format support edges

int* color;
Count cnt;

/*this reverse edge has some bugs*/
//#define REVERSE_DEGE 

void initialize_our_trim(gm_graph &G){


    support_idx = new support_t[G.num_edges()]; 
    support_begin = new edge_t[G_num_nodes+1];    //support begin position 
    support_end = new edge_t[G_num_nodes+1];      //support end position or insert position. 
    stack = new stack_array(G_num_nodes);



#ifdef REVERSE_EDGE
    r_support_idx = new support_t[G.num_edges()]; 
    r_support_begin = new edge_t[G_num_nodes]; 
    r_support_end = new edge_t[G_num_nodes];
#endif

    color = new int[G_num_nodes];
    
    
    #pragma omp parallel
    {
        #pragma omp for
        for(int i = 0; i < G_num_nodes; i++)
        {
            support_begin[i] = G.r_begin[i]; //support -> reverse edges.
            support_end[i] = G.r_begin[i];
#ifdef REVERSE_EDGE
            r_support_begin[i] = G.begin[i];
            r_support_end[i] = G.begin[i];
#endif
            color[i] = LIVE;
        }

        support_begin[G_num_nodes] = G.num_edges();
        support_end[G_num_nodes] = G.num_edges();
        
        #pragma omp for
        for(int i = 0; i < G.num_edges(); i++){
            atomic_write(&support_idx[i].v, -1);
            atomic_write(&support_idx[i].edge_idx, -1);
        }
    }
    
   
} 

void finalize_our_trim(){
    delete[] support_idx;
    delete[] support_begin;
    delete[] support_end;
    delete stack;

#ifdef REVERSE_EDGE
    delete[] r_support_idx;
    delete[] r_support_begin;
    delete[] r_support_end;
#endif 

    delete[] color;
}

/*for edge v->w, add a support of w, idx is the edge index of v->w.*/
inline static void support_append(node_t w, node_t v, int edge_idx_v_w){
    //assert(support_end[w] <= G.r_begin[w+1]);
    support_t &s = support_idx[support_end[w]++];
    s.v = v;
    s.edge_idx = edge_idx_v_w;
}

// #define support_append(G, w, v, edge_idx_v_w) {\
//       support_t &s = support_idx[support_end[w]++];\
//     s.v = v;\
//     s.edge_idx = edge_idx_v_w; \
// }

/*input v; output w, idx.*/
inline static bool get_fisrt_post(gm_graph &G, node_t v, node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w = G.begin[v]; edge_idx_v_w < G.begin[v+1]; edge_idx_v_w ++) 
    {
        cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v != w) { //conditional and to avoid the self-loop
            return true;
        }   
    }
    return false;
}

/*input v; output w, idx.*/
static bool get_r_fisrt_post(gm_graph &G, node_t v, node_t &w, int &edge_idx_v_w){
    for (edge_t edge_idx_v_w = G.r_begin[v]; edge_idx_v_w < G.r_begin[v+1]; edge_idx_v_w ++) 
    {
        cnt.travel_edge++;
        w = G.r_node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v != w) { //conditional and to avoid the self-loop
            return true;
        }
    }
    return false;
}


/*input v, idx; output w and renewed idx*/
inline static bool get_next_post(gm_graph &G, node_t v,  node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w++; edge_idx_v_w < G.begin[v+1] ; edge_idx_v_w ++) 
    {
        cnt.travel_edge++;
        w = G.node_idx[edge_idx_v_w];
        if(LIVE == color[w] 
            && v != w
            ) {
            return true;
        }
    }
    return false;
}

/*input v, idx; output w and renewed idx*/
static  int get_r_next_post(gm_graph &G, node_t v,  node_t &w, int &edge_idx_v_w){
    for (edge_idx_v_w += 1; edge_idx_v_w < G.r_begin[v+1] ; edge_idx_v_w ++) 
    {
        cnt.travel_edge++;
        w = G.r_node_idx[edge_idx_v_w];
        if(LIVE == color[w] && v != w) {
            return true;
        }
    }
    return false;
}

/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_sequential_fast_trim1_init(gm_graph &G){
    //initialization
    node_t w;
    int edge_idx_v_w;
    bool r;
    for(node_t v = 0; v < G_num_nodes; v++){
        //if(DEAD == color[v]) continue;
        r = get_fisrt_post(G, v, w, edge_idx_v_w);
        if(false == r){
            color[v] = DEAD;
            cnt.delete_v++;
            stack->push(v);
        }else{
            support_append(w, v, edge_idx_v_w);
        }

    }
}

void do_our_sequential_fast_trim1_prop(gm_graph &G){
    support_t *s;
    node_t w, w2;
    bool r;
    //propogation
    while(!stack->empty()){
        w = stack->top();
        stack->pop();
        //for each (v, edge_idx_v_w) as support of w.
        for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
            s = &support_idx[k_idx];
            if(DEAD == color[s->v]) continue;

            r = get_next_post(G, s->v, w2, s->edge_idx);
            if(false == r){
                color[s->v] = DEAD;
                cnt.delete_v++;
                stack->push(s->v);
            }else{
                support_append(w2, s->v, s->edge_idx);
            } 
        }
    }
}


/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_sequential_fast_trim1_together(gm_graph &G){
    //initialization
    node_t w;
    int edge_idx_v_w;
    support_t *s;
    node_t w2;

    for(node_t v = 0; v < G_num_nodes; v++){
        if(DEAD == color[v]) continue; /*here color must be int. if it is char, it will be very very slow.*/
#if 1
        bool r = get_fisrt_post(G, v, w, edge_idx_v_w);
#else
        bool r = false;
        for (edge_idx_v_w = G.begin[v]; edge_idx_v_w < G.begin[v+1]; edge_idx_v_w ++) 
        {
            cnt.travel_edge++;
            w = G.node_idx[edge_idx_v_w];
            if(LIVE == color[w] && v != w) { //conditional and to avoid the self-loop
                r = true; 
                break;
            }   
        }
#endif

        if(false == r){
            color[v] = DEAD;
            cnt.delete_v++;
            stack->push(v);
        }else{
#if 1
            support_append(w, v, edge_idx_v_w);
#else
            support_t &s = support_idx[support_end[w]++];
            s.v = v;
            s.edge_idx = edge_idx_v_w;
#endif

        }
#if 1
        //propogation
        while(!stack->empty()){
            w = stack->top();
            stack->pop();
            //for each (v, edge_idx_v_w) as support of w.
            for(int k_idx = support_begin[w]; k_idx < support_end[w]; k_idx++){
                s = &support_idx[k_idx];
                if(DEAD == color[s->v]) continue;
#if 1
                bool r = get_next_post(G, s->v, w2, s->edge_idx);
#else
                bool r = false;
                for (edge_idx_v_w++; edge_idx_v_w < G.begin[v+1] ; edge_idx_v_w ++) 
                {
                    cnt.travel_edge++;
                    w = G.node_idx[edge_idx_v_w];
                    if(LIVE == color[w] && v != w) {
                        r = true;
                        break;
                    }
                }
#endif

                if(false == r){
                    color[s->v] = DEAD;
                    cnt.delete_v++;
                    stack->push(s->v);
                }else{
#if 1
                    support_append(w2, s->v, s->edge_idx); 
#else
                    support_t &s2 = support_idx[support_end[w2]++];
                    s2.v = s->v;
                    s2.edge_idx = s->edge_idx;
#endif


                } 
            }
        }
#endif
    }
}
#ifdef REVERSE_EDGE

/*our sequential fast trim. Maybe we can combine two stage as one*/
void do_our_sequential_fast_r_trim1_init(gm_graph &G){
    //initialization
    node_t w;
    int edge_idx_v_w;
    for(node_t v = 0; v < G_num_nodes; v++){
        if(DEAD == color[v]) continue;
        bool r = get_r_fisrt_post(G, v, w, edge_idx_v_w);
        if(false == r){
            color[v] = DEAD;
            cnt.delete_v++;
            stack->push(v);
        }else{
            support_append(w, v, edge_idx_v_w);
        }
    }
}

#endif

