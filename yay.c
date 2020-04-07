/*

  yay - fast and simple yuv viewer

  (c) 2005-2010 by Matthias Wientapper
  (m.wientapper@gmx.de)

  Support of multiple formats added by Cuero Bugot.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "slre.h"

#include "SDL.h"
//#include <SDL/SDL.h>

SDL_Window *window;
SDL_Surface *screen;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect video_rect;
SDL_Texture *my_texture;
int num_displays = 0;
SDL_DisplayMode dm;

Uint32 window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
Uint32 width = 0;
Uint32 height = 0;
char *vfilename;
FILE *fpointer;
Uint8 *y_data, *cr_data, *cb_data;
Uint16 zoom = 1;
Uint16 min_zoom = 1;
Uint16 frame = 0;
Uint16 quit = 0;
Uint8 grid = 0;
Uint8 bpp = 0;
int cfidc = 1;
int isY4M = 0;

static const Uint8 SubWidthC[4] = { 0, 2, 2, 1 };
static const Uint8 SubHeightC[4] = { 0, 2, 1, 1 };
static const Uint8 SubSizeC[4] = { 0, 4, 2, 1 };
static const Uint8 MbWidthC[4] = { 0, 8, 8, 16 };
static const Uint8 MbHeightC[4] = { 0, 8, 16, 16 };
static const Uint8 FrameSize2C[4] = { 2, 3, 4, 6 };

int load_frame()
{
  Uint32 cnt;

  /* Fill in video data */
  if (isY4M) {
    fseek(fpointer, strlen("FRAME "), SEEK_CUR);
  }
  cnt = fread(y_data, 1, width * height, fpointer);

  //fprintf(stderr,"read %d y bytes\n", cnt);
  if (cnt < width * height) {
    return 0;
  }
  else if (cfidc > 0) {
    cnt = fread(cb_data, 1, height * width / SubSizeC[cfidc], fpointer);
    // fprintf(stderr,"read %d cb bytes\n", cnt);
    if (cnt < width * height / 4) {
      return 0;
    } else {
      cnt = fread(cr_data, 1, height * width / SubSizeC[cfidc], fpointer);
      // fprintf(stderr,"read %d cr bytes\n", cnt);
      if(cnt < width * height / 4){
        return 0;
      }
    }
  }

  return 1;
}

void convert_chroma_to_420()
{
  Uint32 i, j;
  Uint8 *cb, *cr;

  //printf("%dx%d\n",width, height);
  switch (cfidc) {
  case 0:
    memset(&y_data[width * height], 0x80, width * height >> 1);
    break;
  case 1:
    memcpy(&y_data[width * height], cb_data, width * height >> 2);
    memcpy(&y_data[width * height * 5 >> 2], cr_data, width * height >> 2);
    break;
  case 2:
    cb = &y_data[width * height];
    cr = &cb[width * height >> 2];
    for (j = 0; j < height / 2; ++j) {
      for (i = 0; i < width / 2; ++i) {
        cb[i] = cb_data[(j * 2) * width / 2 + i] +
                cb_data[(j * 2 + 1) * width / 2 + i] + 1 >> 1;
        cr[i] = cr_data[(j * 2) * width / 2 + i] +
                cr_data[(j * 2 + 1) * width / 2 + i] + 1 >> 1;
      }
      cb += width >> 1;
      cr += width >> 1;
    }
    break;
  case 3:
    cb = &y_data[width * height];
    cr = &cb[width * height >> 2];
    for (j = 0; j < height / 2; ++j) {
      for (i = 0; i < width / 2; ++i) {
        cb[i] = cb_data[(j * 2) * width + i * 2] +
                cb_data[(j * 2) * width + i * 2 + 1] +
                cb_data[(j * 2 + 1) * width + i * 2] +
                cb_data[(j * 2 + 1) * width + i * 2 + 1] + 2 >> 2;
        cr[i] = cr_data[(j * 2) * width + i * 2] +
                cr_data[(j * 2) * width + i * 2 + 1] +
                cr_data[(j * 2 + 1) * width + i * 2] +
                cr_data[(j * 2 + 1) * width + i * 2 + 1] + 2 >> 2;
      }
      cb += width >> 1;
      cr += width >> 1;
    }
    break;
  default:
    break;
  }
}

void draw_frame() {
  Uint32 x, y;
  Uint8 *pixels = NULL;
  int pitch = 0;

  /* Fill in pixel data - the pitches array contains the length of a line in each plane*/
  SDL_LockTexture(my_texture, NULL, &pixels, &pitch);

  assert(pitch == (int)width);

  convert_chroma_to_420();
  memcpy(pixels, y_data, width * height * 3 >> 1);

  if (grid) {
    // horizontal grid lines
    for (y = 0; y < height; y += 16) {
      for (x = 0; x < width; x += 8) {
        pixels[y * pitch + x] = 0xF0;
        pixels[y * pitch + x + 4] = 0x20;
      }
    }
    // vertical grid lines
    for (x = 0; x < width; x += 16) {
      for (y = 0; y < height; y += 8) {
        pixels[y * pitch + x] = 0xF0;
        pixels[(y + 4) * pitch + x] = 0x20;
      }
    }
  }

  SDL_UnlockTexture(my_texture);

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, my_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void print_usage()
{
  fprintf(stdout, "Usage: yay [-s <widht>x<heigh>] [-f format] filename.yuv\n"
          "\t format can be: 0-Y only, 1-YUV420, 2-YUV422, 3-YUV444\n");
}

int main(int argc, char *argv[])
{
  int opt;
  char caption[32];
  struct slre_cap caps[2];
  int result;
  int used_s_opt = 0;
  int play_yuv = 0;
  unsigned int start_ticks = 0;
  int depth = 8;

  if (argc == 1) {
    print_usage();
    return 1;
  } else {
    while ((opt = getopt(argc, argv, "f:s:")) != -1) {
      switch (opt) {
      case 's':
        if (sscanf(optarg, "%dx%d", &width, &height) != 2) {
          fprintf(stdout, "No geometry information provided by -s parameter.\n");
          return 1;
        }
        used_s_opt = 1;
        break;
      case 'f':
        if (sscanf(optarg, "%d", &cfidc) != 1 || (cfidc < 0 && cfidc>3)) {
          fprintf(stdout, "Invalid format provided by -f parameter.\n");
          return 1;
        }
        break;
      default:
        print_usage();
        return 1;
        break;
      }
    }
  }
  argv += optind;
  argc -= optind;

  vfilename = argv[0];
  fpointer = fopen(vfilename, "rb");
  if (fpointer == NULL) {
    fprintf(stderr, "Error opening %s\n", vfilename);
    return 1;
  }

  if (!used_s_opt) {
    // try to find picture size from filename or path
    result = slre_match("_(\\d+x\\d+).*", vfilename, strlen(vfilename),
                        caps, 2, SLRE_IGNORE_CASE);

    if (result < 0 || sscanf(caps[0].ptr, "%dx%d", &width, &height) != 2) {
      /* Maybe it's Y4M ? */
      const char * y4m_magic = "YUV4MPEG2";
      char input[9];

      if (fread(input, 1, 9, fpointer) != 9 || memcmp(y4m_magic, input, 9)) {
        fprintf(stdout, "No geometry information found in path/filename.\n"
                "Please use -s <width>x<height> paramter.\n");
        return 1;
      } else {
        /* We got a Y4M ! Hurray ! */
        fseek(fpointer, 0, SEEK_SET);
        while (!feof(fpointer)) {
          // Skip Y4MPEG string
          int c = fgetc(fpointer);
          int d, csp = 0;
          while (!feof(fpointer) && (c != ' ') && (c != '\n')) {
            c = fgetc(fpointer);
          }

          while (c == ' ' && !feof(fpointer)) {
            // read parameter identifier
            switch (fgetc(fpointer)) {
            case 'W':
              width = 0;
              while (!feof(fpointer)) {
                c = fgetc(fpointer);

                if (c == ' ' || c == '\n') {
                  break;
                } else {
                  width = width * 10 + (c - '0');
                }
              }
              break;
            case 'H':
              height = 0;
              while (!feof(fpointer)) {
                c = fgetc(fpointer);
                if (c == ' ' || c == '\n') {
                  break;
                } else {
                  height = height * 10 + (c - '0');
                }
              }
              break;
            case 'F':
              /* rateNum = 0; */
              /* rateDenom = 0; */
              while (!feof(fpointer)) {
                c = fgetc(fpointer);
                if (c == '.') {
                  /* rateDenom = 1; */
                  while (!feof(fpointer)) {
                    c = fgetc(fpointer);
                    if (c == ' ' || c == '\n') {
                      break;
                    } else {
                      /* rateNum = rateNum * 10 + (c - '0'); */
                      /* rateDenom = rateDenom * 10; */
                    }
                  }
                  break;
                } else if (c == ':') {
                  while (!feof(fpointer)) {
                    c = fgetc(fpointer);
                    if (c == ' ' || c == '\n') {
                      break;
                    } else {
                      /* rateDenom = rateDenom * 10 + (c - '0'); */
                    }
                  }
                  break;
                } else {
                  /* rateNum = rateNum * 10 + (c - '0'); */
                }
              }
              break;
            case 'A':
              /* sarWidth = 0; */
              /* sarHeight = 0; */
              while (!feof(fpointer)) {
                c = fgetc(fpointer);
                if (c == ':') {
                  while (!feof(fpointer)) {
                    c = fgetc(fpointer);
                    if (c == ' ' || c == '\n') {
                      break;
                    } else {
                      /* sarHeight = sarHeight * 10 + (c - '0'); */
                    }
                  }
                  break;
                } else {
                  /* sarWidth = sarWidth * 10 + (c - '0'); */
                }
              }
              break;
            case 'C':
              csp = 0;
              d = 0;
              while (!feof(fpointer)) {
                c = fgetc(fpointer);

                if (c <= '9' && c >= '0') {
                  csp = csp * 10 + (c - '0');
                } else if (c == 'p') {
                  // example: C420p16
                  while (!feof(fpointer)) {
                    c = fgetc(fpointer);

                    if (c <= '9' && c >= '0')
                      d = d * 10 + (c - '0');
                    else
                      break;
                  }
                  break;
                } else {
                  break;
                }
              }
              if (d >= 8 && d <= 16) {
                depth = d;
              }
              cfidc = (csp == 444) ? 3 : (csp == 422) ? 2 : 1;
              break;
            default:
              while (!feof(fpointer)) {
                // consume this unsupported configuration word
                c = fgetc(fpointer);
                if (c == ' ' || c == '\n') break;
              }
              break;
            }
          }

          if (c == '\n') {
            break;
          }
        }
        isY4M = ftell(fpointer);
      }
    }
  }

  // some WM can't handle small windows...
  if (width < 100) {
    zoom = 2;
    min_zoom = 2;
  }
  //printf("using x=%d y=%d\n", width, height);

  // SDL init
  if (SDL_Init(SDL_INIT_VIDEO) < 0) { 
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);

  if (SDL_CreateWindowAndRenderer(width * zoom, height * zoom, window_flags,
      &window, &renderer)) {
    fprintf(stderr, "Unable to create window/renderer: %s\n", SDL_GetError());
    exit(1); 
  }

  // DEBUG output
  // printf("SDL Video mode set successfully. \nbbp: %d\nHW: %d\nWM: %d\n", 
  // 	info->vfmt->BitsPerPixel, info->hw_available, info->wm_available);

  //SDL_EnableKeyRepeat(500, 10);

  my_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                 SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!my_texture) {
    fprintf(stderr, "Couldn't create texture\n");
    exit(1);
  }

  /* should allocate memory for y_data, cr_data, cb_data here */
  y_data = malloc(width * height * sizeof(Uint8) * 3 >> 1);
  if (cfidc > 0) {
    cb_data = malloc(width * height * sizeof(Uint8) / SubSizeC[cfidc]);
    cr_data = malloc(width * height * sizeof(Uint8) / SubSizeC[cfidc]);
  }

  // send event to display first frame
  event.type = SDL_KEYDOWN;
  event.key.keysym.sym = SDLK_RIGHT;
  SDL_PushEvent(&event);

  // main loop
  while (!quit) {
    sprintf(caption, "frame %d, zoom=%d", frame, zoom);
    SDL_SetWindowTitle(window, caption);

    // wait for SDL event
    SDL_WaitEvent(&event);

    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_SPACE:
      {
        play_yuv = 1; // play it, sam!
        while (play_yuv) {
          start_ticks = SDL_GetTicks();
          sprintf(caption, "frame %d, zoom=%d", frame, zoom);
          SDL_SetWindowTitle(window, caption);

          // check for next frame existing
          if (load_frame()) {
            draw_frame();
            //insert delay for real time viewing
            if (SDL_GetTicks() - start_ticks < 40) {
              SDL_Delay(40 - (SDL_GetTicks() - start_ticks));
            }
            frame++;
          } else {
            play_yuv = 0;
          }
          // check for any key event
          if (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
              // stop playing
              play_yuv = 0;
            }
          }
        }
        break;
      }
      case SDLK_RIGHT:
      {
        // check for next frame existing
        if (load_frame()) {
          draw_frame();
          frame++;
        }
        break;
      }
      case SDLK_BACKSPACE:
      case SDLK_LEFT:
      {
        if (frame > 1) {
          frame--;
          fseek(fpointer, isY4M + (frame - 1) * (height * width *
                FrameSize2C[cfidc] / 2 + (isY4M ? strlen("FRAME ") : 0)),
                SEEK_SET);
          //if(draw_frame())
          load_frame();
          draw_frame();
        }
        break;
      }
      case SDLK_UP:
      {
        zoom++;
        SDL_DestroyTexture(my_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_CreateWindowAndRenderer(width * zoom, height * zoom, window_flags,
                                    &window, &renderer);
        my_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                       SDL_TEXTUREACCESS_STREAMING, width, height);
        draw_frame();
        break;
      }
      case SDLK_DOWN:
      {
        if(zoom > min_zoom) {
          zoom--;
        }
        SDL_DestroyTexture(my_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_CreateWindowAndRenderer(width * zoom, height * zoom, window_flags,
                                    &window, &renderer);
        my_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                       SDL_TEXTUREACCESS_STREAMING, width, height);
        draw_frame();
        break;
      }
      case SDLK_r:
      {
        if(frame > 1) {
          frame = 1;
          fseek(fpointer, isY4M, SEEK_SET);
          //if(draw_frame())
          load_frame();
          draw_frame();
        }
        break;
      }
      case SDLK_g:
        grid = ~grid;
        draw_frame();
        break;
      case SDLK_q:
        quit = 1;
        break;
      case SDLK_f:
        SDL_DestroyTexture(my_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        window_flags ^= SDL_WINDOW_FULLSCREEN;
        SDL_CreateWindowAndRenderer(width * zoom, height * zoom, window_flags,
                                    &window, &renderer);
        my_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                       SDL_TEXTUREACCESS_STREAMING, width, height);
        draw_frame();
        break;
      default:
        break;
      } // switch key
      break;
    case SDL_QUIT:
      quit = 1;
      break;
    case SDL_WINDOWEVENT:
      switch (event.window.event) {
      case SDL_WINDOWEVENT_EXPOSED:
        draw_frame();
        break;
      default:
        break;
      }
      break;
    default:
      break;
    } // switch event type
  } // while

  // clean up
  SDL_DestroyTexture(my_texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  free(y_data);
  free(cb_data);
  free(cr_data);
  fclose(fpointer);

  return 0;
}

