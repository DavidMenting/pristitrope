/*
 fps van de kernel driver omlaag gezet naar 50fps 
 frame_timeout = 11ms
 frame_time = 25ms

 hiermee halen we 20fps
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "jsmn.c"

#define NUM_TOKENS 128

const char *KEYS[] = { "up_frame_start", "up_frame_end", "down_frame_start", "down_frame_end", "idle_frame_start", "idle_timeout", "video_fps" };
char * js = 0; // Will hold the contents of the JSON file
long length;
int key_index;
int j, state;
const static unsigned int encode_up = 2, encode_down = 3, tft_all = 18, tft_next = 27, tft_updown = 22, fb_cs = 23;
const static unsigned int frame_timeout = 10, frame_time = 25; 
unsigned int cs_off_time = 0;
unsigned int next_trigger;
unsigned long previousFrameTime = 0, currentFrameTime, time_elapsed;
const static int STATE_IDLE = 0, STATE_UP = 1, STATE_DOWN = 2, STATE_TILT = 3;
//const static int OVERSPEED = 3; // How many interrupts to miss in order to go into tilt mode

const static unsigned int NUMBER_OF_DISPLAYS = 16;

unsigned int current_frame = 0,
  idle_frame_start=306,
  up_frame_start=0,
  up_frame_end=107,
  down_frame_start=180,
  down_frame_end=286,
  idle_timeout=15000,
  video_fps=16;
//*/
float video_spf; // Seconds Per Frame = 1/fps. 0.05 means 20fps


volatile int upFlag = 0, downFlag = 0;
int direction, previous_direction;

void osd_print(char * msg) {
  puts("osd_show_text ");
  puts(msg);
  puts("\n");
  fflush(stdout);
}

void log_die(char * msg, ...) {
  osd_print(msg);
  abort();
}

char * json_token_tostr(char *js, jsmntok_t *t) {
    js[t->end] = '\0';
    return js + t->start;
}

void config(void){
  FILE * f = fopen ("/media/usb/animation.json", "rb");

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    js = malloc (length);
    if (js)
    {
      fread (js, 1, length, f);
    }
    fclose (f);
  } else {
    osd_print("Unable to open animation.json. Is the file actually there?\n");
  }
  if (js) {
    /* start the json parser */
    jsmn_parser p;
    jsmntok_t tokens[NUM_TOKENS];
    jsmn_init(&p);

    typedef enum { START, KEY, VALUE, SKIP, STOP } parse_state;
    parse_state state = START;

    size_t object_tokens = 0;
    int r = jsmn_parse(&p, js, strlen(js), tokens, NUM_TOKENS);

    for (size_t i = 0, j = 1; j > 0; i++, j--) {
        jsmntok_t *t = &tokens[i];

        // Should never reach uninitialized tokens
        if(!(t->start != -1 && t->end != -1)) log_die("probleem");

        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
            j += t->size;

        switch (state)
        {
            case START:
                if (t->type != JSMN_OBJECT)
                    log_die("Invalid response: root element must be an object.");

                state = KEY;
                object_tokens = t->size;

                if (object_tokens == 0)
                    state = STOP;

                if (object_tokens % 2 != 0)
                    log_die("Invalid response: object must have even number of children.");

                break;

            case KEY:
                object_tokens--;

                if (t->type != JSMN_STRING)
                    log_die("Invalid response: object keys must be strings.");

                state = SKIP;

                for (size_t i = 0; i < sizeof(KEYS)/sizeof(char *); i++)
                {
                    if (strncmp(js + t->start, KEYS[i], t->end - t->start) == 0 && strlen(KEYS[i]) == (size_t) (t->end - t->start))
                    {
                        key_index = i;
                        state = VALUE;
                        break;
                    }
                }

                break;

            case SKIP:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                object_tokens--;
                state = KEY;

                if (object_tokens == 0)
                    state = STOP;

                break;

            case VALUE:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                char *str = json_token_tostr(js, t);

                switch(key_index) {
                  case 0:
                    up_frame_start = atoi(str);
                    break;
                  case 1:
                    up_frame_end = atoi(str);
                    break;
                  case 2:
                    down_frame_start = atoi(str);
                    break;
                  case 3:
                    down_frame_end = atoi(str);
                    break;
                  case 4:
                    idle_frame_start = atoi(str);
                    break;
                  case 5:
                    idle_timeout = atoi(str)*1000;
                    break;
                  case 6:
                    video_fps = atoi(str);
                    break;
                }

                object_tokens--;
                state = KEY;

                if (object_tokens == 0)
                    state = STOP;

                break;

            case STOP:
                // Just consume the tokens
                break;

            default:
                log_die("Invalid state %u", state);
        }
    }
    video_spf = 1.0/video_fps;
  }
}

void isr_encode_up (void) {
  ++upFlag;
}

void isr_encode_down (void) {
  ++downFlag;
}

void next_frame (void) {
  cs_off_time = 0;

  while(cs_off_time < frame_timeout) {
      delay(1);
      if(!digitalRead(fb_cs)) cs_off_time = 0; // Someone's transmitting
    cs_off_time++;
  }
  digitalWrite(tft_next, 1);
  printf("frame_step\n");
  fflush(stdout);

  next_trigger = millis() + frame_time;
  while(digitalRead(fb_cs)){delay(1);};

  digitalWrite(tft_next, 0);
  current_frame++;
}

void jump_frame(int position) {
  cs_off_time = 0;

  while(cs_off_time < frame_timeout) {
      delay(1);
      if(!digitalRead(fb_cs)) cs_off_time = 0; // Someone's transmitting
    cs_off_time++;
  }
  digitalWrite(tft_next, 1);
  printf("pausing seek %f 2\n", position*video_spf);
  fflush(stdout);

  next_trigger = millis() + frame_time;
  while(digitalRead(fb_cs)){delay(1);}
  digitalWrite(tft_next, 0);
  current_frame = position;
}

void idle_frame (void) {
  jump_frame(idle_frame_start);
  
  for(i = 0; i < NUMBER_OF_DISPLAYS; i++) {
    next_frame();
    delay(100);
  }
}


int main (void) {
  config();

  wiringPiSetupGpio () ;

  pinMode(encode_up, INPUT);
  pinMode(encode_down, INPUT);
  pinMode(fb_cs, INPUT);

  wiringPiISR (encode_up, INT_EDGE_BOTH,  &isr_encode_up) ;
  wiringPiISR (encode_down, INT_EDGE_BOTH, &isr_encode_down) ;

  pinMode(tft_all, OUTPUT);
  pinMode(tft_next, OUTPUT);
  pinMode(tft_updown, OUTPUT);

  digitalWrite(tft_all, 0);
  digitalWrite(tft_next, 0);
  digitalWrite(tft_updown, 1);

  idle_frame(); // start on the idle frame

  for(;;) {
     

   if(millis() > next_trigger) { 


      if(millis() - next_trigger > idle_timeout) {
        idle_frame();
        state = STATE_IDLE;
      } else if(upFlag > 0) {
          digitalWrite(tft_updown, 1);
          if(current_frame >= up_frame_start && current_frame <= up_frame_end) next_frame();
          else if(current_frame == up_frame_end + 1 || state == STATE_IDLE) jump_frame(up_frame_start);
          else jump_frame(down_frame_end - current_frame + up_frame_start);
          state = STATE_UP;
          upFlag = 0;
        } else if(downFlag > 0) {
          digitalWrite(tft_updown, 0);
          if(current_frame >= down_frame_start && current_frame <= down_frame_end) next_frame();
          else if(current_frame == down_frame_end + 1 || state == STATE_IDLE) jump_frame(down_frame_start);
          else jump_frame(up_frame_end - current_frame + down_frame_start);
          state = STATE_DOWN;
          downFlag=0;
        }
    }

   delay(1);
  } 
  return 0;
}