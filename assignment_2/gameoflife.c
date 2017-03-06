#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "mpi.h"

#define DEFAULT_ITERATIONS 1
#define GRID_WIDTH  256
#define DIM  16     // assume a square grid

int main ( int argc, char** argv ) {

  int global_grid[ 256 ] = 
   {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  

  // MPI Standard variable
  int num_procs;
  int id;
  int iters = 0;
  int num_iterations;

  // Setup number of iterations
  if (argc == 1) {
    num_iterations = DEFAULT_ITERATIONS;
  }
  else if (argc == 2) {
    num_iterations = atoi(argv[1]);
  }
  else {
    printf("Usage: ./gameoflife <num_iterations>\n");
    exit(1);
  }

  // Messaging variables
  MPI_Status stat;
  // TODO add other variables as necessary

  // MPI Setup
  if ( MPI_Init( &argc, &argv ) != MPI_SUCCESS )
  {
    printf ( "MPI_Init error\n" );
  }

  MPI_Comm_size ( MPI_COMM_WORLD, &num_procs ); // Set the num_procs
  MPI_Comm_rank ( MPI_COMM_WORLD, &id );

  assert ( DIM % num_procs == 0 );
  
  int next_id = (id + 1) % num_procs;
  int prev_id = (id - 1) < 0? num_procs - 1: (id - 1);
  
  // dimension processed by a single process
  int block_row_size = DIM / num_procs;
  
  // We need a temporary array to save
  // the results of the game for the current
  // and then put the results back to full matrix
  int *temporary_array = (int *) malloc(sizeof(int) * block_row_size * DIM );
  
  int *final_array = NULL;
  
  // We need this array only for the 0th process
  // when all the results are gathered.
  if(id == 0)
  {
    final_array = (int*) malloc(sizeof(int) * GRID_WIDTH);
  }
  
  int final_array_count = block_row_size * DIM;
   
  // offset that respresents row index from which
  // the block of current process starts from
  int row_offset = block_row_size * id;
  
  int index, i, j;
  int current_relative_index;
  int alive_neighbour_count;
  int is_current_cell_alive;
  int current_row, current_col, current_index;
  int current_cell_index, current_neighbour_index;
  int temporary_array_cell_index;
  int current_neighbour_row, current_neighbour_col;
  int row_shift, col_shift;
  int iteri;
  
  // Parts to be received and sent to
  // the process responsible for the upper block
  int row_previous_to_receive;
  int row_first_to_send;
  int* row_previous_to_receive_pointer;
  int* row_first_to_send_pointer;
  
  // Parts to be received and sent to
  // the process responsible for the bottom block
  int row_next_to_receive;
  int row_last_to_send;
  int* row_next_to_receive_pointer;
  int* row_last_to_send_pointer;
  
  for(iteri = 0; iteri < num_iterations; iteri++)
  {
    for(current_row = row_offset; current_row < row_offset + block_row_size; current_row++)
    {
      for(current_col = 0; current_col < DIM; current_col++)
      {
        
        alive_neighbour_count = 0;
        
        current_cell_index = current_row*DIM + current_col;
        temporary_array_cell_index = (current_row - row_offset)*DIM + current_col;
        is_current_cell_alive = global_grid[current_cell_index];
        
        // Perform correlation-like operation
        for(row_shift = -1; row_shift < 2; row_shift++)
        {
          
          current_neighbour_row = current_row + row_shift;
          
          // Wrap if necessary
          current_neighbour_row = current_neighbour_row >= DIM? 0: current_neighbour_row;
          current_neighbour_row = current_neighbour_row < 0? DIM-1: current_neighbour_row;
          
          for(col_shift = -1; col_shift < 2; col_shift++)
          {
            
            current_neighbour_col = current_col + col_shift;
            
            // Wrap if necessary
            current_neighbour_col = current_neighbour_col >= DIM? 0: current_neighbour_col;
            current_neighbour_col = current_neighbour_col < 0? DIM-1: current_neighbour_col;
            
            current_neighbour_index = current_neighbour_row*DIM + current_neighbour_col;
            
            // We only need neighbours and not our central element
            if(row_shift == 0 && col_shift == 0)
            {
              
              continue;
            }
            
            // Count the number of alive elements
            if(global_grid[current_neighbour_index])
            {
              alive_neighbour_count++;
            }
            
          }
        }
        
        // Rules of the game
        if(is_current_cell_alive)
        {
          if(alive_neighbour_count != 2 && alive_neighbour_count != 3)
          {
            temporary_array[temporary_array_cell_index] = 0;
          }
          else
          {
            temporary_array[temporary_array_cell_index] = 1;
          }
        }
        else
        {
          if(alive_neighbour_count == 3)
          {
            temporary_array[temporary_array_cell_index] = 1;
          }
          else
          {
            temporary_array[temporary_array_cell_index] = 0;
          }
        }
      }
    }
      
    // Copy matrix back
    int b;
    
    for(b=0; b < DIM*block_row_size; b++)
    {
      global_grid[row_offset*DIM + b] = temporary_array[b];
    }
    
    // Make sure that all of the processes are done
    // with their work.
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Parts to be received and sent to
    // the process responsible for the upper block
    row_previous_to_receive = id == 0? DIM - 1: row_offset - 1;
    row_first_to_send = row_offset;
    row_previous_to_receive_pointer = &global_grid[row_previous_to_receive * DIM];
    row_first_to_send_pointer = &global_grid[row_first_to_send * DIM];
    
    // Parts to be received and sent to
    // the process responsible for the bottom block
    row_next_to_receive = id == (num_procs - 1)? 0: row_offset + block_row_size;
    row_last_to_send = row_offset + block_row_size - 1;
    row_next_to_receive_pointer = &global_grid[row_next_to_receive * DIM];
    row_last_to_send_pointer = &global_grid[row_last_to_send * DIM];
    
    // Don't send anything if we only run on one process
    // Was tested with MPI_Ssend -- works.
    // We avoid the deadlock here by pairing up operations.
    // For this approach to work the number of nodes should be
    // even.
    if(num_procs != 1)
    {
      if ( id % 2 == 0)
      {
        MPI_Send(row_first_to_send_pointer, DIM, MPI_INT, prev_id, 0, MPI_COMM_WORLD);
        MPI_Recv(row_next_to_receive_pointer, DIM, MPI_INT, next_id, 0, MPI_COMM_WORLD, &stat);
        MPI_Send(row_last_to_send_pointer, DIM, MPI_INT, next_id, 0, MPI_COMM_WORLD);
        MPI_Recv(row_previous_to_receive_pointer, DIM, MPI_INT, prev_id, 0, MPI_COMM_WORLD, &stat);
      }
      else 
      { 
        MPI_Recv(row_next_to_receive_pointer, DIM, MPI_INT, next_id, 0, MPI_COMM_WORLD, &stat);
        MPI_Send(row_first_to_send_pointer, DIM, MPI_INT, prev_id, 0, MPI_COMM_WORLD);
        MPI_Recv(row_previous_to_receive_pointer, DIM, MPI_INT, prev_id, 0, MPI_COMM_WORLD, &stat);
        MPI_Send(row_last_to_send_pointer, DIM, MPI_INT, next_id, 0, MPI_COMM_WORLD);
      }
    }
    
    // Gather all the data to the 0th process
    MPI_Gather(temporary_array,
               DIM*block_row_size,
               MPI_INT,
               final_array,
               final_array_count,
               MPI_INT,
               0,
               MPI_COMM_WORLD);
    
    if ( id == 0 ) {
      printf ( "\nIteration %d: final grid:\n", iteri );
      for ( j = 0; j < GRID_WIDTH; j++ ) {
        if ( j % DIM == 0 ) {
          printf( "\n" );
        }
        printf ( "%d  ", final_array[j] );
      }
      printf( "\n" );
    }
    
  }
  
  // Clean up memory
  free(temporary_array);
  free(final_array);
  
  // Finalize so I can exit
  MPI_Finalize();
}