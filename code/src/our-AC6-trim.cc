/**
 * the par-4 version has some errors 
*/

#include "gm.h"
//#include <scc.h>
//#include "my_work_queue.h"
#include <sys/resource.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <our-trim.h>

using namespace std;
 
#if 0
#define ASSERT(a) assert(a)
#else 
#define ASSERT(a)
#endif 

int* color; // used by all 
Count g_cnt;

static node_t* support_begin; // delete from begin
static node_t* support_end; // add to the end
static edge_t* support_idx; //CSR format support edges
node_t* next_begin; // the un-traversed edge begin, also used by ac3 trim
static stack_array *stack; //propogation queue

static omp_lock_t writelock;
static int *lock; // each node has a cas lock

static int global_dead_cnt = 0;
static int global_edge_cnt = 0;

void initialize_ac6_trim(gm_graph &G)
{

    color = new int[G_num_nodes];
    support_idx = new edge_t[G.num_edges()];
    support_begin = new node_t[G_num_nodes+1];    //support begin position 
    support_end = new node_t[G_num_nodes+1];      //support end position or insert position. 
    next_begin = new node_t[G_num_nodes+1];
    stack = new stack_array(G_num_nodes);
    lock = new int[G_num_nodes]; // cas lock for each node


    #pragma omp parallel for
    for (node_t i = 0; i < G_num_nodes; i++) {
        color[i] = LIVE;
        support_begin[i] = G.r_begin[i]; //support -> reverse edges.
        support_end[i] = G.r_begin[i]; // initially empty.
        next_begin[i] = G.begin[i];
        lock[i] = UNLOCK;
        
    }

    #pragma omp paralle for 
    for(int i = 0; i < G.num_edges(); i++){
        support_idx[i] = EMPTY;
    }

    omp_init_lock(&writelock);

}

void finalize_ac6_trim()
{
    delete[] support_idx;
    delete[] support_begin;
    delete[] support_end;
    delete[] next_begin;
    delete stack;

    omp_destroy_lock(&writelock);
}


/*v->w do v's post*/
static inline bool do_post_seq(gm_graph &G, node_t v, stack_array *q) {
    edge_t idx_w;
    ASSERT(next_begin[v] >= G.begin[v]);
    for (idx_w = next_begin[v]; idx_w < G.begin[v+1]; idx_w++) 
    {
        g_cnt.edge++;
        node_t w = G.node_idx[idx_w]; if (EMPTY == w) break;

        //find a LIVE one in post
        if(LIVE == color[w]) { //can't ignore the self-loop
            node_t &s = support_idx[support_end[w]]; // s is the position of w.S
            s = v; // add v to w.S
            support_end[w]++;
            next_begin[v] = idx_w + 1; // move next_begin
            return true;
        }
    }
    
    g_cnt.edge++; // can't find a edge

    color[v] = DEAD;   
    q->push(v);
    next_begin[v] = idx_w; // post is set to empty
    g_cnt.dead++;
    
    return false;
}


void do_seq_ac6_trim(gm_graph &G) {
    stack_array *q =  new stack_array(G_num_nodes);
   
    for(node_t v = 0; v < G_num_nodes; v++)
    {   
        if(DEAD == color[v]) continue;
        
        do_post_seq(G, v, q);
            
        while(!q->empty())
        {
            node_t w = q->top(); q->pop(); 
            for (edge_t idx_v2 = support_begin[w]; idx_v2 < support_end[w]; idx_v2++) {
                node_t v2 = support_idx[idx_v2];
                do_post_seq(G, v2, q);              
            }
            
        }

    }
}



/*our parallel ac-6 trim with lock unlock ************************************/
static inline bool do_post_par_lock(gm_graph &G, node_t v, stack_array *q,
    int &dead_cnt, int &edge_cnt) {

    edge_t idx_w;
    ASSERT(next_begin[v] >= G.begin[v]);
    //omp_set_lock(&writelock);
    for (idx_w = next_begin[v]; idx_w < G.begin[v+1]; idx_w++) 
    {
        edge_cnt++;
        node_t w = G.node_idx[idx_w]; if (EMPTY == w) break;

        //find a LIVE one in post
        omp_set_lock(&writelock);
        if(LIVE == atomic_read(&color[w])) { //can't ignore the self-loop
            node_t &s = support_idx[support_end[w]]; // s is the position of w.S
            s = v; // add v to w.S
            support_end[w]++;
            next_begin[v] = idx_w + 1; // move next_begin
            omp_unset_lock(&writelock);
            return true;
        }
        omp_unset_lock(&writelock);

    }
    
    edge_cnt++; // not find a edge.

    atomic_write(&color[v], DEAD);  
    dead_cnt++;
    //if(cas(&color[v], LIVE, DEAD)) 
    {
        q->push(v);
        next_begin[v] = idx_w; // post is set to empty
    }
    return false;
}



void do_parallel_lock_ac6_trim(gm_graph &G){
    #pragma omp parallel
    {
        
        int dead_cnt = 0; int edge_cnt = 0;
        stack_array *q =  new stack_array(G_num_nodes);


        #pragma omp for schedule(dynamic, PARALLEL_STEP) /*no wait can not be here.*/
        for(node_t v = 0; v < G_num_nodes; v++)
        {   
            if(LIVE == atomic_read(&color[v])) {

                
                do_post_par_lock(G, v, q, dead_cnt, edge_cnt);   
                while(!q->empty())
                {
                    node_t w = q->top(); q->pop(); 
                    for (edge_t idx_v2 = support_begin[w]; idx_v2 < support_end[w]; idx_v2++) {
                        node_t v2 = support_idx[idx_v2];
                        do_post_par_lock(G, v2, q, dead_cnt, edge_cnt);
                    }
                }

            }

        }

        #pragma omp critical
        {
            global_dead_cnt += dead_cnt;
            if (global_edge_cnt < edge_cnt) {global_edge_cnt = edge_cnt; };
        }

        delete q;
    }
    printf("Total Delete Vertex #: %d\n", global_dead_cnt);
    printf("total-traversed-e #: %d\n", global_edge_cnt); // maximum number of traversed edges.
}
/***********************************************************/



/*concurrent by cas***************************************************************/
static inline bool do_post_par(gm_graph &G, node_t v, stack_array *q, Count *cnt) {

    edge_t idx_w;
    ASSERT(next_begin[v] >= G.begin[v]);
    
    for (idx_w = atomic_read(&next_begin[v]); idx_w < G.begin[v+1]; idx_w++) 
    {
        
        node_t w = G.node_idx[idx_w]; if (EMPTY == w) break;
        
        cnt->edge++; //counting all traversed edges.


        //find a LIVE one in post
DO:
        if(LIVE == atomic_read(&color[w])) { //can't ignore the self-loop
            if (cas(&lock[w], UNLOCK, LOCK)) {
                if (DEAD == atomic_read(&color[w])) {
                    atomic_write(&lock[w], UNLOCK);
                    continue;
                } // else {w is locked like, this is what we want}
            } else {goto DO; }

            node_t *s = &support_idx[atomic_read(&support_end[w])]; // s is the position of w.S
            ASSERT(*s == EMPTY);
            
            atomic_write(s,  v); // add v to w.S
            atomic_add(&support_end[w], 1);
            atomic_write(&next_begin[v], idx_w + 1); // move next_begin
            
            ASSERT(LOCK == atomic_read(&lock[w]));
            atomic_write(&lock[w], UNLOCK);     
            return true;
        }

    }

 DO2:    
    if (UNLOCK == atomic_read(&lock[v])) {
        if (cas(&lock[v], UNLOCK, LOCK)) {
            atomic_write(&color[v], DEAD); 
            ASSERT(LOCK == atomic_read(&lock[v]));
            atomic_write(&lock[v], UNLOCK);
        } else {goto DO2;}
    } else {
        goto DO2;
    }

    //cnt->edge++; // not find a edge. ???
    cnt->dead++;
    q->push(v);
    atomic_write(&next_begin[v], idx_w); // post is set to empty

    
    return false;
}



/*our concurrent ac6 trim with cas ***************************************/
void do_parallel_ac6_trim(gm_graph &G){
    #pragma omp parallel
    {
        
        Count *cnt = new Count();
        stack_array *q =  new stack_array(G_num_nodes);


        #pragma omp for schedule(dynamic, PARALLEL_STEP) /*no wait can not be here.*/
        for(node_t v = 0; v < G_num_nodes; v++)
        {   
            if(LIVE == atomic_read(&color[v])) {

                do_post_par(G, v, q, cnt);   
               
                while(!q->empty())
                {
                    node_t w = q->top(); q->pop();
                    edge_t idx_end = atomic_read(&support_end[w]);
                    
                    ASSERT(DEAD == atomic_read(&color[w]));
                    ASSERT(idx_end >= atomic_read(&support_begin[w+1]) || EMPTY == atomic_read(&support_idx[idx_end]));

                    for (edge_t idx_v2 = support_begin[w]; idx_v2 < idx_end; idx_v2++) {
                        node_t v2 = atomic_read(&support_idx[idx_v2]);
                        do_post_par(G, v2, q, cnt);
                    }
                }

            }

        }

        #pragma omp critical
        {
            g_cnt.dead += cnt->dead;
            if (g_cnt.edge < cnt->edge) {g_cnt.edge = cnt->edge; }
            if (g_cnt.seq_v < cnt->seq_v) {g_cnt.seq_v = cnt->seq_v;}
            if (g_cnt.push < q->get_max_stack()) {g_cnt.push = q->get_max_stack();}
        }

        delete q;
        delete cnt;
    }
    
    printf("Total Delete Vertex #: %d\n", g_cnt.dead);
    printf("total-traversed-e #: %d\n", g_cnt.edge); // maximum number of traversed edges.
    printf("Max stack push #: %d\n", g_cnt.push);
}




/*concurrent by cas improve by less cas lock***************************************************************/
static inline bool do_post_par2(gm_graph &G, node_t v, queue_array *q, Count *cnt) {

    edge_t idx_w;
    ASSERT(next_begin[v] >= G.begin[v]);
    
    for (idx_w = atomic_read(&next_begin[v]); idx_w < G.begin[v+1]; idx_w++) 
    {
        
        node_t w = G.node_idx[idx_w]; if (EMPTY == w) break;
        
        cnt->edge++; //counting all traversed edges.


        //find a LIVE one in post
DO:
        if(LIVE == atomic_read(&color[w])) { //can't ignore the self-loop
            if (cas(&lock[w], UNLOCK, LOCK)) {
                // if (DEAD == atomic_read(&color[w])) {
                //     atomic_write(&lock[w], UNLOCK);
                //     continue;
                // } // else {w is locked like, this is what we want}
            } else {goto DO; }

            node_t *s = &support_idx[atomic_read(&support_end[w])]; // s is the position of w.S
            //ASSERT(*s == EMPTY);
            
            atomic_write(s,  v); // add v to w.S
            atomic_add(&support_end[w], 1);
           
            
            //ASSERT(LOCK == atomic_read(&lock[w]));
            atomic_write(&lock[w], UNLOCK); 
            
            atomic_write(&next_begin[v], idx_w + 1); // move next_begin    
            return true;
        }

    }

 DO2:    
    // if (UNLOCK == atomic_read(&lock[v])) {
    //     if (cas(&lock[v], UNLOCK, LOCK)) {
    //         atomic_write(&color[v], DEAD); 
    //         ASSERT(LOCK == atomic_read(&lock[v]));
    //         atomic_write(&lock[v], UNLOCK);
    //     } else {goto DO2;}
    // } else {
    //     goto DO2;
    // }

    atomic_write(&color[v], DEAD); 

    //cnt->edge++; // not find a edge. ???
    cnt->dead++;
    q->push(v);
    atomic_write(&next_begin[v], idx_w); // post is set to empty

    
    return false;
}



void do_parallel_ac6_trim2(gm_graph &G){
    #pragma omp parallel
    {
        
        Count *cnt = new Count();
        queue_array *q =  new queue_array(G_num_nodes);


        #pragma omp for schedule(dynamic, PARALLEL_STEP) /*no wait can not be here.*/
        for(node_t v = 0; v < G_num_nodes; v++)
        {   
            if(LIVE == atomic_read(&color[v])) {

                do_post_par2(G, v, q, cnt);   
               
                while(!q->empty())
                {
                    node_t w = q->top(); q->pop();
                    
                    // busy wait until w is unlock
                    while(true == atomic_read(&lock[w])){};
                    // if (true == atomic_read(&lock[w])) {
                    //     q->push(w); continue;
                    // }

                    edge_t idx_end = atomic_read(&support_end[w]);
                    
                    ASSERT(DEAD == atomic_read(&color[w]));
                    ASSERT(idx_end >= atomic_read(&support_begin[w+1]) || EMPTY == atomic_read(&support_idx[idx_end]));

                    for (edge_t idx_v2 = support_begin[w]; idx_v2 < idx_end; idx_v2++) {
                        node_t v2 = atomic_read(&support_idx[idx_v2]);
                        do_post_par2(G, v2, q, cnt);
                    }
                }

            }

        }

        #pragma omp critical
        {
            g_cnt.dead += cnt->dead;
            if (g_cnt.edge < cnt->edge) {g_cnt.edge = cnt->edge; }
            if (g_cnt.seq_v < cnt->seq_v) {g_cnt.seq_v = cnt->seq_v;}
            if (g_cnt.push < q->get_max()) {g_cnt.push = q->get_max();}
        }

        delete q;
        delete cnt;
    }
    
    printf("Total Delete Vertex #: %d\n", g_cnt.dead);
    printf("total-traversed-e #: %d\n", g_cnt.edge); // maximum number of traversed edges.
    printf("Max stack push #: %d\n", g_cnt.push);
}

