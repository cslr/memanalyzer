// compares changes in the first 1 MB block of kernel memory
// requires root privileges to run

#include <stdio.h>

// Linux/POSIX system headers
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

// C++ headers
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>



int main(int argc, char** argv)
{
  const char* device = "/dev/kmem";
  
  const unsigned int MEMSIZE = 1024*1024;
  
  int fd = open(device, O_RDWR | O_SYNC); // O_RDWR for write access (dangerous) [was: OR_RDONLY]

  if(fd < 0){
    printf("ERROR: Cannot access %s device.\n", device);
    return -1;
  }
  
  unsigned char* address =
    (unsigned char*)mmap(NULL, MEMSIZE,
			 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  // volatile pointer to find changes in memory mapped device/memory within program
  volatile unsigned char* addr = address;
    
  if(address == MAP_FAILED || address == NULL){
    printf("ERROR: Memory mapping /dev/mem memory to own memory address space failed.\n");
    close(fd);
    return -1;
  }

  unsigned char* prev = (unsigned char*)malloc(MEMSIZE);
  
  if(prev == NULL){
    printf("ERROR: Memory allocation failed.\n");
    munmap(address, MEMSIZE);
    close(fd);
    return -1;
  }
  
  
  memset(prev, 0, MEMSIZE); // initially prev memory is assumed to be zero

  unsigned int delta = 0;
  
  while(1){ // Only CTRL-C stops this stub program
    delta = 0;

#pragma omp parallel
    {
      unsigned int d = 0;
      
#pragma omp for nowait schedule(auto)
      for(unsigned int i=0;i<MEMSIZE;i++){
	const auto value = addr[i];
	
	if(prev[i] != value){
	  d++;
	  prev[i] = value; // instantly updates prev copy
	}
      }

#pragma omp critical
      {
	delta += d;
      }
    }

    if(delta > 0){
      printf("%d/%d memory locations changed (%.0f%%).\n", delta, MEMSIZE,
	     100.0f*((float)delta)/((float)MEMSIZE));
      fflush(stdout);
    }
    
    // sleep 100ms (10 Hz screen update frequeny)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }


  free(prev);
  munmap(address, MEMSIZE);
  close(fd);

  return 0;
  
}
