#include "bitrankw32int.h"
#include<time.h>

#define N 3200
#define P 400

int main(){
    uint *bitmap,*bitmap2;
    srand(time(NULL));
    bitmap = (uint *) malloc(sizeof(uint)*(N/32+1));
    bitmap2 = (uint *) malloc(sizeof(uint)*(N/32+1));
    for(int i=0;i<N;i++){
        if(i==P){
            bitset(bitmap,i);
        }
        else {
            if(rand()%2>0){
                bitset(bitmap,i);
                bitset(bitmap2,i);
            }
        }
    }
    bitRankW32Int * br = createBitRankW32Int(bitmap,N,1,20);
    bitRankW32Int * br2 = createBitRankW32Int(bitmap2,N,1,20);
    printf("unos en br %u unos rn br2 %u\n",rank(br,N-1),rank(br2,N-1));
    for(int i=0;i<br->n/br->s+1;i++) printf("Rs[%d]=%u\n",i,br->Rs[i]);
    for(int i=P-2;i<P+3;i++) printf("Rank[%d]=%u\n",i,rank(br,i));
    replaceRank(br, P, 0);
    printf("Ver Delete\n");
    for(int i=0;i<br->n/br->s+1;i++) printf("Rs[%d]=%u\n",i,br->Rs[i]);
    for(int i=P-2;i<P+3;i++) printf("Rank[%d]=%u\n",i,rank(br,i));
    printf("Ver Correcta\n");
    for(int i=0;i<br2->n/br2->s+1;i++) printf("Rs[%d]=%u\n",i,br2->Rs[i]);
    for(int i=P-2;i<P+3;i++) printf("Rank[%d]=%u\n",i,rank(br2,i));
    printf("unos en br %u unos rn br2 %u\n",rank(br,N-1),rank(br2,N-1));
}
