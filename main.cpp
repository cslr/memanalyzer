/*
 * Memory visualizer
 * 
 * Visualizes first 1 MB of physical memory which contains many
 * kernel data structures. This is done by reading every 100ms
 * first 1 MB of "/dev/mem" in Linux.
 *
 * Device file is mapped to memory for easier access.
 */

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

#include <dinrhiw/dinrhiw.h>

#include "SDLGUI.h"

#include <sys/uio.h>


const unsigned long MEMSIZE = 32*1024*1024; // 4 MB




/*

ssize_t process_vm_readv(pid_t pid,
			 const struct iovec *local_iov,
			 unsigned long liovcnt,
			 const struct iovec *remote_iov,
			 unsigned long riovcnt,
			 unsigned long flags);

ssize_t process_vm_writev(pid_t pid,
			  const struct iovec *local_iov,
			  unsigned long liovcnt,
			  const struct iovec *remote_iov,
			  unsigned long riovcnt,
			  unsigned long flags);

*/

struct memory_area
{
  unsigned long start;
  unsigned long end;
  unsigned long length;
};

// gets virtual address mappings [heap] and [stack] of the target process
bool getProcessMemory(const pid_t pid,
		      std::vector<memory_area>& areas)
{
  if(pid < 0) return false;

  areas.clear();
  
  char buffer[256];
  snprintf(buffer, 256, "/proc/%d/maps", (unsigned int)pid);
  
  FILE* fp = fopen(buffer, "r");
  if(fp == NULL) return false;

  std::vector<std::string> lines;

  while(!feof(fp)){
    if(buffer == fgets(buffer,256,fp)){
      std::string str = buffer;

      if(str.find("[heap]") != std::string::npos ||
	 str.find("[stack]") != std::string::npos ||
	 str.find("[ anon ]") != std::string::npos){

	// NOTE this is buggy should really check more precisely if it is rw flag or filename has rw
	if(str.find("rw") != std::string::npos){
	  lines.push_back(str);
	}
      }
    }
    else break;
  }

  fclose(fp);

  
  if(lines.size() <= 0) // could not find any interesting memory mappings
    return false;

  // parses interesting lines
  for(auto line : lines){
    auto pos = line.find(" ");

    if(pos == std::string::npos) continue;

    line = line.substr(0, pos);

    pos = line.find("-");

    if(pos == std::string::npos) continue;

    std::string part1 = line.substr(0, pos);
    std::string part2 = line.substr(pos+1);

    struct memory_area area;

    errno = 0;
    area.start = strtoul(part1.c_str(), NULL, 16);
    if(errno == EINVAL || errno == ERANGE) // conversion error
      continue;

    errno = 0;
    area.end = strtoul(part2.c_str(), NULL, 16);
    if(errno == EINVAL || errno == ERANGE) // conversion error
      continue;

    area.length = area.end - area.start;

    areas.push_back(area);
  }

  return (areas.size() > 0);
}


bool readProcessMemory(const pid_t pid,
		       const std::vector<memory_area>& areas,
		       whiteice::math::vertex<>& memory,
		       unsigned char* buffer) // buffer must be memory.length()*sizeof(char)
{
  if(pid < 0) return false;
  if(areas.size() < 0) return false;
  if(buffer == NULL) return false;
  
  unsigned long area = 0;
  unsigned long total_memory = 0;

  while(total_memory < memory.size() && area < areas.size()){
    // get area and copy it to vertex

    struct iovec local_iov[1];
    struct iovec remote_iov[1];

    unsigned long bytes = 0;

    if(total_memory + areas[area].length > memory.size()){
      if(total_memory < memory.size())
	bytes = memory.size() - total_memory;
      else
	bytes = 0;
    }
    else{
      bytes = areas[area].length;
    }

    local_iov[0].iov_base = (void*)buffer;
    local_iov[0].iov_len = bytes;

    remote_iov[0].iov_base = (void*)areas[area].start;
    remote_iov[0].iov_len = bytes;

    if(process_vm_readv(pid, local_iov, 1, remote_iov, 1, 0) != bytes){
      printf("Reading pid process memory failed (area: %d/%d length: %d).\n",
	     area, areas.size(), bytes);
      return false;
    }

#pragma omp parallel for schedule(auto)
    for(unsigned long i=0;i<bytes;i++){
      memory[total_memory+i] = (float)buffer[i];
    }

    total_memory += bytes;
    area++;
  }

  if(total_memory < memory.size()){
#pragma omp parallel for schedule(auto)    
    for(unsigned int i=total_memory;i<memory.size();i++)
      memory[i] = 0.0f;
  }

  
  if(total_memory > 0 && total_memory <= memory.size())
    return true;
  else
    return false;
}


int main(int argc, char** argv)
{
  printf("Memory Visualizer\n");
  printf("(C) Copyright Tomas Ukkonen. 2020.\n");

  pid_t pid = getpid();

  bool visualization = false;

  for(unsigned int i=0;i<argc;i++){
    if(strcmp("--gui", argv[i]) == 0)
      visualization = true;
    else{
      unsigned int p = atoi(argv[i]);
      if(p > 0) pid = p;
    }
  }
  
    
  std::vector<memory_area> memareas;

  if(getProcessMemory(pid, memareas) == false){
    printf("ERROR: Cannot get pid %d memory mappings.\n", pid);
    return -1;
  }

  unsigned long total_length = 0;

  for(const auto& area : memareas){
    std::cout << "Memory Area: " << std::hex << area.start << "-" << area.end
	      << " length: " << std::dec << area.length << std::endl;
    total_length += area.length;
  }

  printf("Sampling first %.2f MB of memory (%.2f%% of total vector length)\n",
	 total_length/((double)1024*1024), 100.0*total_length/((double)MEMSIZE));
  
  whiteice::math::vertex<> memory;
  whiteice::math::vertex<> prev;
  memory.resize(MEMSIZE);
  memory.zero();

  unsigned char* buffer = (unsigned char*)malloc(memory.size());
  if(buffer == NULL){
    printf("ERROR: Memory allocation failed.\n");
    return -1;
  }
  
  if(readProcessMemory(pid, memareas, memory, buffer) == false){
    printf("ERROR: Cannot read process memory.\n");
    free(buffer);
    return -1;
  }

  prev = memory;

  {
    char title[80];
    snprintf(title, 80, "PID %d memory visualization.", (int)pid);
    
    SDLGUI* const gui = visualization ? new SDLGUI(title) : nullptr;

    if(gui) gui->show();
    
    unsigned int deltas = 0;
    
    while(1){
      const auto time_begin = std::chrono::high_resolution_clock::now();
      
      if(gui) if(gui->keypress() == true) break;

      memareas.clear();

      if(getProcessMemory(pid, memareas)){
	if(readProcessMemory(pid, memareas, memory, buffer) == false){
	  printf("readProcessMemory() call faild.\n");
	}
      }

      deltas = 0;

      unsigned int XMAX = 1;
      unsigned int YMAX = 1;

      if(gui){
	XMAX = gui->getScreenX();
	YMAX = gui->getScreenY();
	gui->clear();
      }

      // plots first 64 KB to 2d image
#pragma omp parallel
      {
	unsigned int d = 0;

#pragma omp for schedule(auto) nowait
	for(unsigned int i=0;i<MEMSIZE;i++){
	  // detect changes in values
	  {
	    if(prev[i] != memory[i]){
	      deltas++;
	      prev[i] = memory[i];
	    }
	  }

	  if(gui){
	    const unsigned int x = (i % XMAX);
	    const unsigned int y = (i / XMAX);
	    if(x >= XMAX) continue;
	    if(y >= YMAX) continue;
	    
	    int value = 0;
	    whiteice::math::convert(value, memory[i]);
	    value &= 0xFF; // keep within 0-255 range
	    
	    gui->plot(x, y, value, value, value);
	  }

	}

#pragma omp critical
	{
	  deltas += d;
	}
      }

      if(gui)
	gui->updateScreen();
      
      const auto time_end = std::chrono::high_resolution_clock::now();
      const unsigned long timens = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end-time_begin).count();
      const double update_hz = 1.0/(timens/((double)1.0e9));
      
      unsigned long mapping_total = 0;
      
      for(const auto m : memareas)
	mapping_total += m.length;
      
      
      if(deltas > 0){
	printf("%d/%d changes (%.2f%%). [process mem: %.2f MB] [sampling Hz: %.2f]\n",
	       deltas, MEMSIZE,
	       100.0f*((float)deltas)/((float)MEMSIZE),
	       ((double)mapping_total/(1024.0*1024.0)),
	       update_hz);
	fflush(stdout);
      }
      
      
      // sleep 100ms (10 Hz screen update frequency)
      // std::this_thread::sleep_for(100ms);
      
    } // while(gui.keypress())

    if(gui) gui->hide();
    
    delete gui;
    
    
  }

  free(buffer);
  
  return 0;
}
