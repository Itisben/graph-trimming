#pragma once
#include "gm.h"

//our ac6 trim
void do_seq_ac6_trim(gm_graph &G);
void do_parallel_lock_ac6_trim(gm_graph &G);
void do_parallel_ac6_trim(gm_graph &G);
void do_parallel_ac6_trim2(gm_graph &G);
void initialize_ac6_trim(gm_graph &G);
void finalize_ac6_trim();

// our ac4 trim
void initialize_ac4_trim(gm_graph &G);
void finalize_ac4_trim();
void do_trim_graph_all(gm_graph &G);

//our ac3 trim
void do_ac3_repeat_trim_graph_all(gm_graph &G);

extern int32_t  G_num_nodes;
extern size_t PARALLEL_STEP;  

/*here need to sync the read and write*/
#define atomic_read(v)      (*(volatile typeof(*v) *)(v))
#define atomic_write(v,a)   (*(volatile typeof(*v) *)(v) = (a))

#define fetch_or(a, b)      __sync_fetch_and_or(a,b)
#define cas(a, b, c)        __sync_bool_compare_and_swap(a,b,c)
#define atomic_add(a, b)        __sync_fetch_and_add(a, b);
#define atomic_sub(a, b)        __sync_fectch_and_sub(a, b);
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define LIVE        0
#define DEAD        1
#define REMOVE      2  // removed by sample
#define EMPTY       -1
#define LIVE_LOCK   2
#define DEAD_LOCK   3

#define UNLOCK  0
#define LOCK    1

class Count{
    public:
    int edge;//max number of edges for multi-worker.
    int dead; //total dead
    int push; // max push for multi-worker. 
    int seq_v; // seq running v num for multi-worker.
    Count (): edge{0}, dead{0}, push{0}, seq_v {0} { }
};

/*this stack is more efficient than the STL stack */
class stack_array {
public:
    stack_array(int max_sz) {
        stack = new int32_t[max_sz];
        stack_ptr = 0;
        max_stack = 0;
    }
    virtual ~stack_array() { delete [] stack;}


    inline void push(node_t n) {
        stack[stack_ptr] = n;
        stack_ptr ++;
         if (stack_ptr > max_stack) {
             max_stack = stack_ptr;
         }
        //max_stack++;
    }
    inline int size()   {return stack_ptr;}
    inline int32_t top() {return stack[stack_ptr - 1];}
    inline void set_top(int32_t v) {stack[stack_ptr - 1] = v;}
    inline void pop()   {stack_ptr--;}
    inline bool empty() {return (stack_ptr == 0);}
    int32_t* get_ptr()  {return stack;}
    int32_t get_max_stack() {return max_stack;}
private:
    int32_t* stack;
    int32_t stack_ptr;
    int32_t max_stack;
};


/*queue array may be better fit to our algorithm*/
class queue_array {
public:
    queue_array(int s) {
        max_size = s;
        queue = new int32_t[max_size];
        head = max_size - 1;
        tail = max_size - 1;   //current push position.   
        push_size = 0;
        max_queue = 0;
    }
    virtual ~queue_array() { delete [] queue;}


    inline void push(node_t n) {
        queue[tail] = n;
        tail--;
        push_size++;
        if (max_queue < push_size) 
            max_queue = push_size;
    }

    inline void pop(){
        head--;
        push_size--;      
    }
    
    inline int32_t top(){
        return queue[head];
    }

    inline bool empty(){
        return (head == tail);
    }

    inline int size(){
        return head - tail;
    }

    inline int _num(){
        return push_size;
    }

    inline int get_max(){
        return max_queue;
    }

private:
    int32_t* queue;
    int32_t head;
    int32_t tail;
    int32_t max_size;
    int32_t max_queue;
    int push_size;
};
