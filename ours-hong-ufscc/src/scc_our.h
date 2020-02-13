#ifndef SSC_OUR_H
#define SSC_OUR_H

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
#define PROPAGATE 2
#define REDEAD      3
#define COMPLETE    4

typedef struct{
    node_t v;
    int edge_idx;
}support_t;



class Count{
    public:
    int travel_edge;
    int delete_v;
    int push;
    Count(){
        clear();
    }
    void clear(){
        travel_edge = 0;
        delete_v = 0;
        push = 0;
    }
};

/*this stack is more efficient than the STL stack */
class stack_array {
public:
    stack_array(int max_sz) {
        stack = new int32_t[max_sz];
        stack_ptr = 0;
        //max_stack = 0;
    }
    virtual ~stack_array() { delete [] stack;}


    inline void push(node_t n) {
        stack[stack_ptr] = n;
        stack_ptr ++;
        // if (stack_ptr > max_stack) {
        //     max_stack = stack_ptr;
        // }
    }
    inline int size()   {return stack_ptr;}
    inline int32_t top() {return stack[stack_ptr - 1];}
    inline void set_top(int32_t v) {stack[stack_ptr - 1] = v;}
    inline void pop()   {stack_ptr--;}
    inline bool empty() {return (stack_ptr == 0);}
    int32_t* get_ptr()  {return stack;}
    //int32_t get_max_stack() {return max_stack;}
private:
    int32_t* stack;
    int32_t stack_ptr;
    //int32_t max_stack;
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
    }
    virtual ~queue_array() { delete [] queue;}


    inline void push(node_t n) {
        queue[tail] = n;
        tail--;
        if(tail < 0){
            tail = max_size - 1;
        }
        push_size++;
    }

    inline void push2(node_t n){
        queue[tail] = n;
        tail--;
        if(tail < 0){
            tail = max_size - 1;
        }
    }

    inline void pop(){
        head--;
        if(head < 0){
            head = max_size -1;
        }        
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

    inline int push_num(){
        return push_size;
    }

private:
    int32_t* queue;
    int32_t head;
    int32_t tail;
    int32_t max_size;
    int push_size;
};
#endif