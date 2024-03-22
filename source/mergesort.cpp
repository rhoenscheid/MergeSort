#include <mpi.h>
#include <random> 
#include <algorithm> 

using namespace std;

// merge two lists which are stored next to one another in a buffer
void merge(double *buffer, int start1, int size1, int size2)
{
    double *working_buffer = new double[size1+size2];
    int count1 = 0; int count2 = 0;
    int start2 = start1 + size1;
    while((count1 < size1) & (count2 < size2))
    {
        double x1 = buffer[start1 + count1];
        double x2 = buffer[start2 + count2];
        if(x1 < x2)
        {
            working_buffer[count1 + count2] = x1;
            count1++;
        }
        else
        {
            working_buffer[count1 + count2] = x2;
            count2++;
        }
    }

    //Fill buffer with whichever values remain
    for(int i = count1; i < size1; i++)
    {
        working_buffer[i + count2] = buffer[start1 + i];
    }
    for(int i = count2; i < size2; i++)
    {
        working_buffer[count1 + i] = buffer[start2 + i];
    }

    for(int i = 0; i < (size1+size2); i++)
    {
        buffer[i] = working_buffer[i];
    }

    delete[] working_buffer;
}

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    int process_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

    int num_proc;
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

    int listSize = 256/num_proc; // I am cheating here because I don't want to send another message communicating the size in this simple example. 
    double sub_list[listSize]; // Only needs to be big enough to hold our sub list 

    if (process_id == 0)
    {
        const int N = 256;
        std::mt19937_64 rng;
        std::uniform_real_distribution<double> dist(0, 1);
        double master_list[N];
        for(int i = 0; i < N; i++)
        {
            master_list[i] = dist(rng);
        }

        int listSize = N / num_proc; // We are using a multiple of four to avoid dealing with remainders!

        // Send the list data in messages
        for(int i = 1; i < num_proc; i++)
        {
            double * buffer_start = master_list + listSize*i;
            MPI_Send(buffer_start,
                     listSize,
                     MPI_DOUBLE,
                     i,
                     0,
                     MPI_COMM_WORLD);
        }
        
        std::sort(master_list, master_list + listSize);

        for(int i = 1; i < num_proc; i++)
        {
            double * buffer_start = master_list + listSize*i; // copy received buffers back into master list
            // we just need the sorted sublists, but we don't care which process is sending them so we'll take 
            // them in whichever order they come using MPI_ANY_SOURCE. The loop makes sure that receive enough
            // messages. 
            MPI_Recv(buffer_start,
                     listSize,
                     MPI_DOUBLE,
                     MPI_ANY_SOURCE,
                     1,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }

        // Merge all the lists
        // Again we'll cheat this loop a bit by assuming that num_proc and N are both powers of two
        // In real code we would have to deal with things like remainders properly but this example is already quite big!
        for(int i = num_proc; i > 1; i /= 2)
        {
            int subListSize = N / i; 
            for(int j = 0; j < i; j+=2)
            {
                merge(master_list, j*subListSize, subListSize, subListSize);
            }
        }

        printf("Sorted List: ");
        for(int i = 0; i < N; i++)
        {
            printf("%f ", master_list[i]);
        }
        printf("\n");
    }
    else
    {
        MPI_Recv(sub_list,
                 listSize,
                 MPI_DOUBLE,
                 0,
                 0,
                 MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        printf("Process %d received a list starting with %f\n", process_id, sub_list[0]);
        
        std::sort(sub_list, sub_list+listSize);
    }

    for(int i = 0; i < static_cast<int>(std::sqrt(num_proc)); i++){

        if(static_cast<int>((process_id / pow(2, i))) % 2 == 0){
            MPI_Recv(sub_list,
                 listSize,
                 MPI_DOUBLE,
                 0,
                 0,
                 MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
            printf("Process %d received a list starting with %f\n", process_id, sub_list[0]);
            std::sort(sub_list, sub_list+listSize);
            merge()
        }
        if(static_cast<int>(((process_id + 1) / pow(2, i))) % 2 == 0){
            
            int receiver = process_id - pow(2, i);

            MPI_Send(sub_list,
                 listSize, 
                 MPI_DOUBLE,
                 receiver,
                 1,
                 MPI_COMM_WORLD);
        }

    }

   

    MPI_Finalize();
}
