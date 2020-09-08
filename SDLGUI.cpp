

#include "SDLGUI.h"
#include <functional>
#include <chrono>

bool equal(volatile unsigned char* volatile x, volatile unsigned char* volatile y)
{
  if(*x == *y) return true;
  else return false;
}


SDLGUI::SDLGUI(const std::string windowtitle)
{
  if(initialize()){
    this->title = windowtitle;
  }
}


SDLGUI::~SDLGUI()
{
  this->hide();

  {
    const std::lock_guard<std::mutex> lock(sdl_mutex);

    if(renderer){
      SDL_DestroyRenderer(renderer);
      renderer = NULL;
    }
    
    if(window){
      SDL_DestroyWindow(window);
      window = NULL;
    }
  }

  {
    thread_mutex.lock();
    
    running = false;
    
    if(gui_thread){
      gui_thread->join();
      delete gui_thread;
      gui_thread = nullptr;
    }

    thread_mutex.unlock();
  }

  {
    const std::lock_guard<std::mutex> lock(sdl_mutex);
    SDL_Quit();
  }
}

bool SDLGUI::initialize()
{
  if(initialized == true) return true;
  
  int rv = 0;
  {
    const std::lock_guard<std::mutex> lock(sdl_mutex);
    rv = SDL_Init(SDL_INIT_VIDEO);
  }
  
  if(rv != 0){
    initialized = false;
    return false;
  }
  else{

    {
      thread_mutex.lock();

      if(gui_thread == nullptr){
	running = true;

	try{
	  gui_thread = new thread(std::bind(&SDLGUI::loop, this));
	}
	catch(std::exception& e){
	  running = false;
	  gui_thread = nullptr;
	}
      }
      
      thread_mutex.unlock();
    }

    if(running){
      initialized = true;
      return true;
    }
    else{
      initialized = false;
      return false;
    }
  }
}

// shows/opens graphics window
bool SDLGUI::show()
{
  if(!initialized){
    if(initialize() == false){
      printf("SDLGUI::show(): SDL INITIALIZE FAILED\n");
      return false;
    }
  }

  if(window == NULL){

    unsigned int XSIZE = this->getScreenX();
    unsigned int YSIZE = this->getScreenY();

    if(XSIZE == 0 || YSIZE == 0){
      printf("SDLGUI::show(): SDL SCREEN SIZE FAILED.\n");
      return false;
    }

    const std::lock_guard<std::mutex> lock(sdl_mutex);
    
    window = SDL_CreateWindow(title.c_str(),
			      SDL_WINDOWPOS_UNDEFINED,
			      SDL_WINDOWPOS_UNDEFINED,
			      XSIZE,
			      YSIZE,
			      SDL_WINDOW_INPUT_FOCUS | 
			      SDL_WINDOW_SHOWN);

    if(window == NULL){
      printf("SDLGUI::show(): SDL CREATE WINDOW FAILED\n");
      return false;
    }

    if(renderer){
      SDL_DestroyRenderer(renderer);
      renderer = NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    if(renderer == NULL){
      printf("SDLGUI::show(): SDL CREATE RENDERER FAILED\n");
      
      SDL_DestroyWindow(window);
      window = NULL;

      return false;
    }

    keypress_happened = false;

    // fills area with (black) red
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    SDL_RenderPresent(renderer);

    SDL_RaiseWindow(window);

    return true;
  }
  else{
    const std::lock_guard<std::mutex> lock(sdl_mutex);
    
    // just show existing window
    SDL_ShowWindow(window);

    SDL_RaiseWindow(window);

    return true;
  }
			      
}

// hides/closes graphics window
bool SDLGUI::hide()
{
  if(!initialized) if(initialize() == false) return false;

  if(window == NULL) return false;

  {
    const std::lock_guard<std::mutex> lock(sdl_mutex);
    SDL_HideWindow(window);
  }

  return true;
}

// gets maximum X+1 coordinate
unsigned int SDLGUI::getScreenX()
{
  if(!initialized) if(initialize() == false) return 0;

  const std::lock_guard<std::mutex> lock(sdl_mutex);

  return 1024; // hack
  
#if 0

  SDL_DisplayMode DM;
  DM.w = 0;
  DM.h = 0;
  SDL_GetCurrentDisplayMode(0, &DM);

  unsigned int x = (unsigned int)DM.w;
  unsigned int y = (unsigned int)DM.h;

  if(x <= 0 || y <= 0) return false;

  if(x > y) x = y;
  if(y > x) y = x;

  // half the size of desktop
  x /= 2;
  y /= 2;

  if(x <= 0) x = 1;
  if(y <= 0) y = 1;

  return x;
#endif
}

// gets maximum Y+1 coordinate
unsigned int SDLGUI::getScreenY()
{
  if(!initialized) if(initialize() == false) return 0;

  const std::lock_guard<std::mutex> lock(sdl_mutex);

  return 1024;
  
#if 0
  SDL_DisplayMode DM;
  DM.w = 0;
  DM.h = 0;
  SDL_GetCurrentDisplayMode(0, &DM);

  unsigned int x = (unsigned int)DM.w;
  unsigned int y = (unsigned int)DM.h;

  if(x <= 0 || y <= 0) return false;

  if(x > y) x = y;
  if(y > x) y = x;

  // half the size of desktop
  x /= 2;
  y /= 2;

  if(x <= 0) x = 1;
  if(y <= 0) y = 1;

  return y;
#endif
}

// plots pixel to screen (white as the default)
bool SDLGUI::plot(unsigned int x, unsigned int y,
		  unsigned int r, unsigned int g, unsigned int b)
{
  if(!initialized) if(initialize() == false) return false;

  if(window == NULL || renderer == NULL) return false;

  const std::lock_guard<std::mutex> lock(sdl_mutex); // this is slow [don't lock SDL mutex]

  if(SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF) != 0)
    return false;  

  if(SDL_RenderDrawPoint(renderer, x, y) != 0)
    return false;


#if 0
  SDL_Rect rect;
  rect.x = ((int)x)-1;
  rect.y = ((int)y)-1;
  rect.w = 3;
  rect.h = 3;
  
  if(SDL_RenderFillRect(renderer, &rect) != 0)
    return false;
#endif

  return true;
}

// clears screen to wanted color (black as the default)
bool SDLGUI::clear(unsigned int r, unsigned int g, unsigned int b)
{
  if(!initialized) if(initialize() == false) return false;
  
  const std::lock_guard<std::mutex> lock(sdl_mutex);
  
  if(window == NULL || renderer == NULL) return false;

  if(r >= 0xFF) r = 0xFF;
  if(g >= 0xFF) g = 0xFF;
  if(b >= 0xFF) b = 0xFF;

  // fills area with black
  if(SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF) != 0)
    return false;
  
  if(SDL_RenderClear(renderer) != 0)
    return false;

  return true;
}

// updates plotted pixels to screen
bool SDLGUI::updateScreen()
{
  if(!initialized) if(initialize() == false) return false;

  const std::lock_guard<std::mutex> lock(sdl_mutex);

  if(window && renderer){
    SDL_RenderPresent(renderer);
  }
  else return false;

  return true;
}


bool SDLGUI::waitKeypress()
{
  if(!initialized) if(initialize() == false) return false;

  const unsigned int keypresses_start = this->sdl_keypresses;

  // wait for keypress
  while(keypresses_start == this->sdl_keypresses){
    // sleep for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return true;
}


bool SDLGUI::keypress()
{
  if(!initialized) if(initialize() == false) return false;

  if(keypress_happened){
    keypress_happened = false;
    return true;
  }
  else return false;
}


// internal thread to update GUI
void SDLGUI::loop()
{
  unsigned int counter = 0;
  
  while(running){
    // sleep for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // skip update if window isn't open
    if(initialized == false) continue;
    
    const std::lock_guard<std::mutex> lock(sdl_mutex);

    SDL_Event event;
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_QUIT){
	sdl_keypresses++;
	keypress_happened = true;
      }
      
    } // don't handle events

#if 0
    if(window != NULL && renderer != NULL)
      if(counter == 0) SDL_RenderPresent(renderer); // update screen only every 1 second
#endif

    counter++;
    if(counter >= 10) counter = 0;
  }
}
