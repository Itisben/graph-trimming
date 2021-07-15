#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
/*linear accessing is faster than random accessing around 10 times.
on my local HP with I7 CPU 12G meory. 
guo@HP-BINGUO:/mnt/d/git-code/scc-trim-v3/experiments/our-convert$ ./a.out
linear accessing =65.281000
random accessing =853.827000
*/
int main(){
    const int repeat = 1000;
    std::vector<int> a(1000000, 0);
    
    struct timeval T1, T2;
    gettimeofday(&T1, NULL); 
    for (int j = 0; j < repeat; j++) {
        for(int i = 0; i < a.size(); i++){
            a[i]++;
        }
    }
    gettimeofday(&T2, NULL); 
    printf("linear accessing =%lf\n", 
        (T2.tv_sec - T1.tv_sec)*1000 + 
        (T2.tv_usec - T1.tv_usec)*0.001 
    );

    gettimeofday(&T1, NULL); 
    for (int j = 0; j < repeat; j++) {
        for (int i = 0; i < 1000; i++) {
            for(int k = i; k < a.size(); k+=1000){
                a[k]++;
            }
        }
       
    }
    gettimeofday(&T2, NULL); 
    printf("random accessing =%lf\n", 
        (T2.tv_sec - T1.tv_sec)*1000 + 
        (T2.tv_usec - T1.tv_usec)*0.001 
    );


    return 0;

}