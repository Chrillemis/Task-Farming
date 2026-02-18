/*
  Assignment: Make an MPI task farm. A "task" is a randomly generated integer.
  To "execute" a task, the worker sleeps for the given number of milliseconds.
  The result of a task should be send back from the worker to the master. It
  contains the rank of the worker
*/

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <array>
#include <vector>

// To run an MPI program we always need to include the MPI headers
#include <mpi.h>

const int NTASKS=5000;  // number of tasks
const int RANDOM_SEED=1234;

const int TAG_STOP = 0;
const int TAG_WORK = 1;
const int TAG_RESULT = 2;



void master (int nworker) {
    std::array<int, NTASKS> task, result;

    // set up a random number generator
    std::random_device rd;
    //std::default_random_engine engine(rd());
    std::default_random_engine engine;
    engine.seed(RANDOM_SEED);
    // make a distribution of random integers in the interval [0:30]
    std::uniform_int_distribution<int> distribution(0, 30);

    for (int& t : task) {
        t = distribution(engine);   // set up some "tasks"
    }

    /*
    IMPLEMENT HERE THE CODE FOR THE MASTER
    ARRAY task contains tasks to be done. Send one element at a time to workers
    ARRAY result should at completion contain the ranks of the workers that did
    the corresponding tasks
    */

    
//  MPI_Send(&buffer, count, datatype, destination, tag, communicator);
    MPI_Status status;
    int task_index = 0;  
    int tasks_finished = 0; 
    std::vector<int> active_indices(nworker+1); // Index corresponts to worker rank. Index holds the task id 

    for (int i = 0; i < nworker; i++){ // This loop send a task to each of the workers
        MPI_Send(&task[task_index], 1, MPI_INT, i+1, 1, MPI_COMM_WORLD); 

        active_indices[i+1] = task_index;
        task_index++;
    }

    while(tasks_finished < NTASKS){
        int received_rank; 
        
        MPI_Recv(&received_rank, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int worker_rank = received_rank; // Who finished?

        int task_index_finished = active_indices[received_rank];

        result[task_index_finished] = worker_rank;

        tasks_finished++;

        if (task_index < NTASKS){
            MPI_Send(&task[task_index], 1, MPI_INT, worker_rank, TAG_WORK, MPI_COMM_WORLD); 
            active_indices[worker_rank] = task_index;
            task_index++;
        } else {
            // No more tasks. Tell this worker to go home.
            // Send TAG_STOP (0)
            int dummy = 0;
            MPI_Send(&dummy, 1, MPI_INT, worker_rank, TAG_STOP, MPI_COMM_WORLD);
        }
    }   
 


    // Print out a status on how many tasks were completed by each worker
    for (int worker=1; worker<=nworker; worker++) {
        int tasksdone = 0; double workdone = 0;
        for (int itask=0; itask<NTASKS; itask++)
        if (result[itask]==worker) {
            tasksdone++;
            workdone += task[itask];
        }
        std::cout << "Master: Worker " << worker <<  " solved " << tasksdone << " tasks, Work done = " << workdone / 1000<< " Seconds\n";    
    }

    double waiting_time = 0;
    for (int i = 0; i < NTASKS;i++){
        waiting_time += task[i];
    }
    std::cout <<"Total waiting time in tasks = "<< waiting_time/1000 << "seconds "<< "\n";
}

// call this function to complete the task. It sleeps for task milliseconds
void task_function(int task) {
    std::this_thread::sleep_for(std::chrono::milliseconds(task));
}


void worker (int rank) {
    int buffer_recv; // This is where we store the received message
    MPI_Status status; 
    while(true){

        MPI_Recv(&buffer_recv, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // Receiving message from master

        if (status.MPI_TAG == TAG_STOP) { // Work is done
            break; 
        }

        task_function(buffer_recv); // Doing the task
        
        MPI_Send(&rank, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD); // Sending back rank of woker 
    }
}  

int main(int argc, char *argv[]) {
    int nrank, rank; // Number of ranks and specific rank

    MPI_Init(&argc, &argv);                // set up MPI
    MPI_Comm_size(MPI_COMM_WORLD, &nrank); // get the total number of ranks
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // get the rank of this process

    if (rank == 0)       // rank 0 is the master
        master(nrank-1); // there is nrank-1 worker processes
    else                 // ranks in [1:nrank] are workers
        worker(rank);

    MPI_Finalize();      // shutdown MPI
}
