
#ifndef __SDLGUI_h
#define __SDLGUI_h

#include <thread>
#include <mutex>
#include <string>

#include <dinrhiw/dinrhiw.h>
#include <SDL.h>

bool equal(volatile unsigned char* volatile x, volatile unsigned char* volatile y); 

class SDLGUI : public whiteice::VisualizationInterface
{
 public:

  SDLGUI(const std::string windowtitle);
  ~SDLGUI();

  bool initialize();

  // shows/opens graphics window
  virtual bool show();
  
  // hides/closes graphics window
  virtual bool hide();
  
  // gets maximum X+1 coordinate
  virtual unsigned int getScreenX();
  
  // gets maximum Y+1 coordinate
  virtual unsigned int getScreenY();

  // plots pixel to screen (white as the default)
  virtual bool plot(unsigned int x, unsigned int y,
		    unsigned int r = 255, unsigned int g = 255, unsigned int b = 255);
		    
  
  // clears screen to wanted color (black as the default)
  virtual bool clear(unsigned int r = 0, unsigned int g = 0, unsigned int b = 0);
  
  // updates plotted pixels to screen
  virtual bool updateScreen();

  // blocks until key have been pressed
  virtual bool waitKeypress();

  // returns true if there have been keypress since the last call
  virtual bool keypress();

 private:
  std::string title;

  bool initialized = false;

  // keeps updating screen every one second so GUI is responsive
  void loop();
  
  bool running = false;
  std::mutex thread_mutex;
  std::thread* gui_thread = nullptr;
  
  
  // SDL variables
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;

  std::mutex sdl_mutex;
  volatile unsigned int sdl_keypresses = 0;
  volatile bool keypress_happened = false;
  
};



#endif

