#include <stdio.h>

#include "fenster.h"
#include "fenster_audio.h"

#define POCKETMOD_IMPLEMENTATION
#include "pocketmod.h"

#define W 320
#define H 240

char *readfile(const char *fname, long *lenp) {
    FILE *f;
    long r;
    char *str;

    if(!(f = fopen(fname, "rb")))
        return NULL;

    fseek(f, 0, SEEK_END);
    *lenp = (long)ftell(f);
    rewind(f);

    if(!(str = malloc(*lenp+2)))
        return NULL;
    r = fread(str, 1, *lenp, f);

    if(r != *lenp) {
        free(str);
        return NULL;
    }

    fclose(f);
    str[*lenp] = '\0';
    return str;
}
 
#define MIN(a,b) (((a) < (b))?(a):(b))
 
static int run() {
	
  uint32_t buf[W * H];
  
  struct fenster f = {
      .title = "hello",
      .width = W,
      .height = H,
      .buf = buf,
  };
  
  struct fenster_audio fa = {0};
  
  pocketmod_context mod = {0};
  
  //char *modfile = "songs/elysium.mod";
  //char *modfile = "songs/nemesis.mod";
  char *modfile = "songs/king.mod";
  
  long mod_size;
  char *mod_data = readfile(modfile, &mod_size);
  if(!mod_data) {
	  fprintf(stderr, "error: Couldn't read %s\n", modfile);
	  return 1;
  }
  
  fenster_open(&f);
  fenster_audio_open(&fa);
  
  pocketmod_init(&mod, mod_data, mod_size, FENSTER_SAMPLE_RATE);
  
  uint32_t t = 0, u = 0;
  float audio[FENSTER_AUDIO_BUFSZ * 2];
  float audio_out[FENSTER_AUDIO_BUFSZ];
  int64_t now = fenster_time();
  while (fenster_loop(&f) == 0) {
    t++;
    
	int n = fenster_audio_available(&fa);
    if (n > 0) {	 
	  int samples_to_render = 2 * MIN(n, FENSTER_AUDIO_BUFSZ);
	  int bytes_rendered = pocketmod_render(&mod, audio, samples_to_render * sizeof audio[0]);
	  int samples_rendered = bytes_rendered / sizeof audio[0];
	  /* pocketmod renders the samples to stereo, but fenster_audio is mono, so
	  I just mix the two channels together */
	  for(int j = 0; j < samples_rendered / 2; j++) {
		  audio_out[j] = (audio[j*2] + audio[j*2+1]) / 2; 
	  }
	  fenster_audio_write(&fa, audio_out, samples_rendered / 2);
    }
	
    if (f.keys[27])
      break;

    for (int i = 0; i < 320; i++) {
      for (int j = 0; j < 240; j++) {
        /* White noise: */
        /* fenster_pixel(&f, i, j) = (rand() << 16) ^ (rand() << 8) ^ rand(); */

        /* Colourful and moving: */
        /* fenster_pixel(&f, i, j) = i * j * t; */

        /* Munching squares: */
        fenster_pixel(&f, i, j) = i ^ j ^ t;
      }
    }
    int64_t time = fenster_time();
    if (time - now < 1000 / 60) {
      fenster_sleep(time - now);
    }
    now = time;
  }
  fenster_audio_close(&fa);
  fenster_close(&f);
  return 0;
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine,
                   int nCmdShow) {
  (void)hInstance, (void)hPrevInstance, (void)pCmdLine, (void)nCmdShow;
  return run();
}
#else
int main() { return run(); }
#endif
