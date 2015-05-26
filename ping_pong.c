#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

/* Error Check */
#define CHKERRQ(n) if(n != MPI_SUCCESS) printf("Error!! Check line number : %d\n",__LINE__)


int main(int argc, char **argv)
{

    const int PING_PONG_LIMIT = 30;
    int ierr, rank, size, i, source, dest,n;
    double starttime, endtime,elapsedtime,latency,bandwidth;
    double *val; char a = 'a';

    ierr = MPI_Init(&argc,&argv); CHKERRQ(ierr);

    ierr = MPI_Comm_size(MPI_COMM_WORLD,&size); CHKERRQ(ierr);
    ierr = MPI_Comm_rank(MPI_COMM_WORLD,&rank); CHKERRQ(ierr);

    if(size != 2) {
        fprintf(stderr, "World size must be two!!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if(rank == 0) printf("    Bytes \t     Time \t    Latency \t    Bandwidth\n");

    // n is used to allocate different size of data to send/recv
    for(n = 1; n <= 1000000; n*=10){

        val = (double*)malloc(n*sizeof(double));
        for(i = 0; i < n; i++){
            val[i] = i+1;
        }

        /* latency check, sends 1 byte of data */
        latency = 0;
        for(i = 0; i < 50; i++){
            if(rank == 0){
                ierr = MPI_Barrier(MPI_COMM_WORLD); CHKERRQ(ierr);
                starttime = MPI_Wtime();
                ierr = MPI_Send(&a,1,MPI_BYTE,1,0,MPI_COMM_WORLD); CHKERRQ(ierr);
                ierr = MPI_Recv(&a,1,MPI_BYTE,1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE); CHKERRQ(ierr);
                endtime = MPI_Wtime();
                latency += (endtime-starttime)/2;
            }else{
                ierr = MPI_Barrier(MPI_COMM_WORLD); CHKERRQ(ierr);
                ierr = MPI_Recv(&a,1,MPI_BYTE,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE); CHKERRQ(ierr);
                ierr = MPI_Send(&a,1,MPI_BYTE,0,0,MPI_COMM_WORLD); CHKERRQ(ierr);
            }
        }
        latency /= 50;

        /* bandwidth check */
        int PING_PONG_COUNT = 0, tag = 0;
        elapsedtime = 0;
        while(PING_PONG_COUNT < PING_PONG_LIMIT){

            source = PING_PONG_COUNT%2, dest = (PING_PONG_COUNT+1)%2;

            if(rank == source){
                ierr = MPI_Barrier(MPI_COMM_WORLD); CHKERRQ(ierr);
                starttime = MPI_Wtime();
                ierr = MPI_Send(val,n,MPI_DOUBLE,dest,tag,MPI_COMM_WORLD); CHKERRQ(ierr);
                ierr = MPI_Recv(val,n,MPI_DOUBLE,dest,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE); CHKERRQ(ierr);
                endtime = MPI_Wtime();
                elapsedtime += endtime-starttime;

            }else if (rank == dest){
                ierr = MPI_Barrier(MPI_COMM_WORLD); CHKERRQ(ierr);
                ierr = MPI_Recv(val,n,MPI_DOUBLE,source,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE); CHKERRQ(ierr);
                ierr = MPI_Send(val,n,MPI_DOUBLE,source,tag,MPI_COMM_WORLD); CHKERRQ(ierr);
            }
            PING_PONG_COUNT++; tag++;
        }

        if(rank == 0){
            elapsedtime /= PING_PONG_LIMIT;
            bandwidth = (n*sizeof(double)*1.0e-6)/(elapsedtime/2);
            printf("%8lu \t %f \t %f \t %8g\n",n*sizeof(double),elapsedtime,latency,bandwidth);
        }
        free(val);
    }

    ierr = MPI_Barrier(MPI_COMM_WORLD); CHKERRQ(ierr);
    ierr = MPI_Finalize(); CHKERRQ(ierr);

    return 0;
}
