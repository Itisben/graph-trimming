#include "gm.h"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <omp.h>
#include <assert.h>
#include <sys/time.h>

//#include "scc.h"
#include "common_main.h"
//#include "my_work_queue.h"
#include <iostream>
#include "our-trim.h"
//extern void work_q_print_max_depth();

size_t PARALLEL_STEP = 1024*4; //default value
size_t SAMPLE_EDGE = 100;
size_t SAMPLE_VER = 100;
extern int* color;

extern void check_WCC();

node_t* G_SCC;
int32_t G_num_nodes;

#define USE_TRIM2   1

class my_main : public main_t
{
    private:
        int method;
        int detail_time;
        int analyze;
        int print;

    public:
        void  sample_edges(int percent_edge) {
            int vnum = 0, edgenum = 0, r_edgenum = 0;
            srand(time(NULL));
            for(node_t i = 0; i < G.num_edges(); i++){
                if (rand() % 100 < (100 - percent_edge)) {
                    G.node_idx[i] = EMPTY;
                } else {edgenum++;}
            }
            
            //remove the dead edges
            #pragma omp parallel for
            for (node_t i = 0; i < G_num_nodes; i++) {
                node_t newk_idx = G.begin[i];
                for (edge_t k_idx = G.begin[i]; k_idx < G.begin[i+1]; k_idx++){
                    node_t k = G.node_idx[k_idx];
                    if (EMPTY != k) {
                        G.node_idx[newk_idx++] = k;
                    }
                }

                for (edge_t k_idx = newk_idx; k_idx < G.begin[i+1]; k_idx++){
                    G.node_idx[k_idx] = EMPTY;
                }
            }

            //generate the r_graph
            int *r_end = new int[G_num_nodes];
            #pragma omp parallel for
            for (node_t i = 0; i < G_num_nodes; i++) {
                r_end[i] = G.r_begin[i];
            }

            
            for (node_t i = 0; i < G_num_nodes; i++) {
                for (edge_t k_idx = G.begin[i]; k_idx < G.begin[i+1]; k_idx++){
                    node_t k = G.node_idx[k_idx]; if (EMPTY == k) { break;}
                    G.r_node_idx[r_end[k]] = i;
                    r_end[k]++;
                    r_edgenum++;
                }
                
            }

            //reset rgraph the empty
            #pragma omp parallel for
            for (node_t i = 0; i < G_num_nodes; i++) {
                for (edge_t k_idx = r_end[i]; k_idx < G.r_begin[i+1]; k_idx++){
                    G.r_node_idx[k_idx] = EMPTY;
                }
            }
            delete[] r_end;

            //printf("sample edge %d%%\n", percent_edge);
            printf("***SampleN = %d, SampleM = %d\n", G.num_nodes(), edgenum);
            printf("***r_SampleM = %d\n", r_edgenum);
        }
        void sample_vertices(int percent_ver) {
            int vnum = 0, edgenum = 0, r_edgenum = 0;
            srand(time(NULL));
            //sample percent_ver to live or 100 - percent to dead.
            for(node_t i = 0; i < G_num_nodes; i++){
                if (rand() % 100 < (100 - percent_ver)) {
                    color[i] = DEAD;
                } else {vnum++;}
            }

            //remove the DEAD edge
            #pragma omp parallel for
            for (node_t i = 0; i < G_num_nodes; i++) {
                node_t newk_idx = G.begin[i];
                for (edge_t k_idx = G.begin[i]; k_idx < G.begin[i+1]; k_idx++){
                    node_t k = G.node_idx[k_idx];
                    if (DEAD != color[k]) {
                        G.node_idx[newk_idx++] = k;
                        #pragma omp atomic
                        edgenum++;
                    }
                }

                for (edge_t k_idx = newk_idx; k_idx < G.begin[i+1]; k_idx++){
                    G.node_idx[k_idx] = EMPTY;
                }
            }

            //remove the DEAD edge in r_graph
            #pragma omp parallel for
            for (node_t i = 0; i < G_num_nodes; i++) {
                node_t newk_idx = G.r_begin[i];
                for (edge_t k_idx = G.r_begin[i]; k_idx < G.r_begin[i+1]; k_idx++){
                    node_t k = G.r_node_idx[k_idx];
                    if (DEAD != color[k]) {
                        G.r_node_idx[newk_idx++] = k;
                        #pragma omp atomic
                        r_edgenum++;
                    }
                }

                for (edge_t k_idx = newk_idx; k_idx < G.r_begin[i+1]; k_idx++){
                    G.r_node_idx[k_idx] = EMPTY;
                }
            }

            //printf("sample vertex %d%%\n", percent_ver);
            printf("***SampleN = %d, SampleM = %d\n", vnum, edgenum);
            printf("***r_SampleM = %d\n", r_edgenum);
        }
        virtual bool check_args(int argc, char** argv)
        {
            method = 0;
            detail_time = 0;
            analyze = 0;
            print = 0;

            if (argc < 1) return false;
            method = atoi(argv[0]);
            //if ((method < 0) || (method > 10 ) || () return false;

            if (argc >= 2) {
                if (strncmp(argv[1],"-d",2)==0) 
                    detail_time = 1;
                else if (strncmp(argv[1],"-a",2)==0) 
                    analyze = 1;
                else if (strncmp(argv[1],"-p",2)==0)
                    print = 1;
                else if (strncmp(argv[1],"-s",2)== 0) {
                    PARALLEL_STEP = atoi(argv[2]);
                }else if (strncmp(argv[1],"-es",3)== 0) {
                    SAMPLE_EDGE = atoi(argv[2]);
                }else if (strncmp(argv[1],"-vs",3)== 0) {
                    SAMPLE_VER = atoi(argv[2]);
                }
                else {printf("ignoring %s\n", argv[1]);}

            }

            printf("my PARALLEL_STEP: %ld\n", PARALLEL_STEP);
            printf("***my SAMPLE_EDGE: %ld\n", SAMPLE_EDGE);
            printf("***my SAMPLE_VER: %ld\n", SAMPLE_VER);

            return true;
        }

        virtual void print_arg_info() 
        {
            printf("<method> {-s 4096|-es 100|-vs 100}\n");
            // printf("   method  0: Trim1 + FW-BW (Baseline) \n");
            // printf("   method  1: Trim1 + Global FW-BW + Trim1 + FW-BW (Method 1)\n");
            // printf("   method  2: Trim1 + Global FW-BW + Trim1/2 + WCC + FW-BW (Method 2)\n");
            // printf("   method  3: Tarjan \n");
            // printf("   method  *4: Trim1 + Tarjan \n");
            // printf("   method  5: UFSCC \n");
            // printf("   method  51: UFSCC with Trim\n");
            // printf("   method  6: Our Sequntial Fast Trim1\n");
            // printf("   method  7: Our Sequntial together Fast Trim1\n");
            // printf("   method  8: Our Concurrent Fast Trim1\n");
            printf("   method  *9: Our Concurrent AC-6 Trim\n");
            printf("   method  *91: Our Sequential AC-6 Trim\n");
            printf("   method *10: our Concurrent AC-4 Trim\n");
            printf("   method *11: our Concurrent AC-3 Trim\n");
            // printf("   -d: Print detailed execution time breakdown\n");
            // printf("   -a: Print SCC size histogram\n");
            // printf("   -p: Output SCC list to file\n");
            printf("   -s 4096: dynamic parallel step, defalut 4096\n");
            printf("   -se 100: sample the edges by percent, default 100%%\n");
            printf("   -sv 100: sample the vertices by percent, default 100%%\n");
        }

        virtual bool prepare() 
        {
            gm_rt_initialize();
            G_num_nodes = G.num_nodes();

            G.make_reverse_edges();
            G_SCC= new node_t[G.num_nodes()];

            #pragma omp parallel for
            for(int i =0;i <G_num_nodes; i++)
                G_SCC[i] = gm_graph::NIL_NODE;

            //work_q_init(gm_rt_get_num_threads());  // called at the beginning
            //initialize_color();
            //initialize_trim1();
            //initialize_trim2();
            //initialize_tarjan();

#if 0  /*we don't test other methods right now.*/
            initialize_ufscc();
            initialize_analyze();
            initialize_global_fb();
            initialize_WCC();
#endif
            /*our*/
            initialize_ac4_trim(G);
            initialize_ac6_trim(G);
            //initialize_dcscc_trim(G); // AC4-trim

            //sampel vertices by setting color
            if (SAMPLE_VER < 100) sample_vertices(SAMPLE_VER);

            /*sample the edges by shrank the node_idx*/ 
            if (SAMPLE_EDGE < 100) sample_edges(SAMPLE_EDGE);

            return true;
        }

        virtual bool run() 
        {   
            printf("Running Method %d : ", method);
            // if (method == 0) {
            //     printf("Running Baseline: Trim1 + FW-BW\n");
            //     do_baseline();
            //     return true;
            // } else if (method == 1) {
            //     printf("Trim1 + Global FW-BW + Trim1 + FW-BW\n");
            //     do_baseline_global_fb();
            //     return true;
            // } else if (method == 2) {
            //     printf("Trim1 + Global FW-BW + Trim1/2 + WCC + FW-BW\n");
            //     do_baseline_global_wcc_fb();
            //     return true;
            // } else if (method == 3) {
            //     printf("Tarjan\n");
            //     do_tarjan_all(G);
            //     return true;
            // } else if (method == 4) {
            //     //printf("Trim1 + Tarjan\n");
            //     printf("Traditional Trim1\n");
            //     do_baseline_global_trim_tarjan();
            //     return true;
            // } else if (method == 5) {
            //     printf("Running UFSCC\n");
            //     do_baseline_global_ufscc();
            //     return true;
            // } else if (method == 51) {
            //     printf("Running UFSCC with Trim\n");
            //     //do_baseline_global_ufscc_trim();
            //     return true;
            // } else if (method == 6){
            //     printf("Our Sequential Fast Trim \n");
            //     //do_our_trim();
            //     return true;
            // } else if (method == 7){
            //     printf("Our Sequential Together Fast Trim \n");
            //     //do_our_together_trim();
            //     return true;
            // } else if (method == 8){
            //     printf("Our Concurrent Fast Trim\n");
            //     //do_our_concurrent_trim();  
            //     return true;
            // } else 
            if (method == 9){ 
                printf("*Our Concurrent AC6 Trim\n");
                do_parallel_ac6_trim(G);
                return true;
            } else if (method == 91){
                printf("*Our Sequential AC6 Trim\n");
                do_seq_ac6_trim(G); 
                return true;
            }else if (method == 92){
                printf("Our Concurrent AC6 by lock Trim\n");
                do_parallel_lock_ac6_trim(G);
                return true;
            }else if (method == 93){
                printf("Our imporoved Concurrent AC6 Trim\n");
                do_parallel_ac6_trim2(G);
    
            }else if (method == 10) {
                printf("*Our Concurrent AC-4 Trim\n");
                do_trim_graph_all(G);
                return true;
            } else if (method == 11) {
                printf("*Our Concurrent AC-3 Trim\n");
                do_ac3_repeat_trim_graph_all(G);
                return true;
            }
            // else if (method == 109){
            //     printf("Test AC-6 Trim correctness\n");
            //     extern int* color;
            //     do_trim_graph_all(G); //ac4 trim
            //     vector<int> ac4(color, &color[G_num_nodes-1]);

            //     for(int i = 0; i < G_num_nodes; i++) { color[i] = LIVE;} //reset
            //     do_parallel_ac6_trim(G);
            //     vector<int> ac6(color, &color[G_num_nodes-1]);
            //     if (ac4 != ac6) {
            //         for(int i = 0; i < G_num_nodes; i++) {
            //             if (ac4[i] != ac6[i]) 
            //             printf("#### node %d, ac4 %d, ac6 %d\n", i, ac4[i], ac6[i]);
            //         }
            //     }
            //     return true;
            // }
            else{
                printf("Please check the method\n");
            }

            return false;
        }

        virtual bool post_process() {         

            /*ours*/  
            finalize_ac6_trim();
            finalize_ac4_trim();

            unsigned int max_degout = 0, max_degin = 0;
            double  degout = 0, degin = 0;
            for(int i=0;i<G.num_nodes(); i++) {
                unsigned int m = G.begin[i+1] - G.begin[i];
                degout += m;
                if (m > max_degout) max_degout = m;
                
                m = G.r_begin[i+1] - G.r_begin[i];
                degin += m;
                if (m > max_degin) max_degin = m;
                degin += m;
            }
            
            printf("max degout = %u\n", max_degout);
            printf("max degin = %u\n", max_degin);
            
            printf("total degout = %f, average = %f\n", degout, double(degout/G.num_nodes()));
            printf("total degin = %f, average = %f\n", degin, double(degin/G.num_nodes()));
            printf("average degree = %f\n", (degout+degin)/G.num_nodes());

            //test the correctness of triming vertices. 
            /*I don't know why here has double free errors.*/
            //if (scc != NULL) delete[] scc;
            return true;
        }

#define PHASE_BEGIN(X) {if (detail_time) {gettimeofday(&V1, NULL);}}
#define PHASE_END(X)   {if (detail_time) {gettimeofday(&V2, NULL); \
    printf("\t[%s phase: %f ms]\n", X,  \
            (V2.tv_sec - V1.tv_sec)*1000.0 + (V2.tv_usec - V1.tv_usec)*0.001);}}
#define EMPTY_PHASE(X) {if (detail_time) printf("\t[%s phase: %f ms]\n", X, 0.0);}
   
};

int main(int argc, char** argv)
{
    my_main T;
    T.main(argc, argv);
}
