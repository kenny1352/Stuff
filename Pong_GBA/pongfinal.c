/*
 /$$$$$$$   /$$$$$$  /$$   /$$  /$$$$$$ 
 | $$__  $$ /$$__  $$| $$$ | $$ /$$__  $$
 | $$  \ $$| $$  \ $$| $$$$| $$| $$  \__/
 | $$$$$$$/| $$  | $$| $$ $$ $$| $$ /$$$$
 | $$____/ | $$  | $$| $$  $$$$| $$|_  $$
 | $$      | $$  | $$| $$\  $$$| $$  \ $$
 | $$      |  $$$$$$/| $$ \  $$|  $$$$$$/
 |__/       \______/ |__/  \__/ \______/ 

 * pong.c
 *  This program is a recreation of the classic Pong arcade game built
 *  for the gameboy advance
 * 
 * Author: Kenny Campbell
 * Language: C
 * Class: CPSC 305 Section 2
 */

// Library for rand() and srand()
#include <stdlib.h>

/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
 * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the screen is simply a pointer into memory at a specific address this
 *  * pointer points to 16-bit colors of which there are 240x160 */
volatile unsigned short* screen = (volatile unsigned short*) 0x6000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short* palette = (volatile unsigned short*) 0x5000000;

/* pointers to the front and back buffers - the front buffer is the start
 * of the screen array and the back buffer is a pointer to the second half */
volatile unsigned short* front_buffer = (volatile unsigned short*) 0x6000000;
volatile unsigned short* back_buffer = (volatile unsigned short*)  0x600A000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank( ) {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}


/* keep track of the next palette index */
int next_palette_index = 0;

/*
 * function which adds a color to the palette and returns the
 * index to it
 */
unsigned char add_color(unsigned char r, unsigned char g, unsigned char b) {
    unsigned short color = b << 10;
    color += g << 5;
    color += r;

    /* add the color to the palette */
    palette[next_palette_index] = color;

    /* increment the index */
    next_palette_index++;

    /* return index of color just added */
    return next_palette_index - 1;
}

/* a colored rectangel or square */
struct Shape {
    short x, y, sizel, sizew;
    unsigned char color;
    int dx, dy;
};

/* put a pixel on the screen in mode 4 */
void put_pixel(volatile unsigned short* buffer, int row, int col, unsigned char color) {
    /* find the offset which is the regular offset divided by two */
    unsigned short offset = (row * WIDTH + col) >> 1;

    /* read the existing pixel which is there */
    unsigned short pixel = buffer[offset];

    /* if it's an odd column */
    if (col & 1) {
        /* put it in the left half of the short */
        buffer[offset] = (color << 8) | (pixel & 0x00ff);
    } else {
        /* it's even, put it in the left half */
        buffer[offset] = (pixel & 0xff00) | color;
    }
}

/* draw a Shape onto the screen */
void draw_shape(volatile unsigned short* buffer, struct Shape* s) {
    unsigned short row, col;
    /* for each row of the square */
    for (row = s->y; row < (s->y + s->sizel); row++) {
        /* loop through each column of the square */
        for (col = s->x; col < (s->x + s->sizew); col++) {
            /* set the screen location to this color */
            put_pixel(buffer, row, col, s->color);
        }
    }
}

// Handles the movement of the ball, checks if someone scored, and collisions
int ball_movement(struct Shape* ball, struct Shape* p1, struct Shape* p2) {
  if (ball->dy == 0) {
    ball->dy += 1; 
  }
  
  /* if statement to see if someone scored*/
  if ((ball->x == 0) || ((ball->x + ball->sizew) == 239)) {
    return 1;
  }

  // if statement for border collisions
  if ((ball->y == 0) || ((ball->y + ball->sizew) == 159)) {
    ball->dy *= -1;
  }

  // if statement for paddle collisions
  if ((ball->y >= p1->y) && (ball->y <= (p1->y + p2->sizel)) && ((ball->x - 1) == p1->x)) {
    ball->dx *= -1;
  }
  else if ((ball->y >= p2->y) && (ball->y <= (p2->y + p2->sizel)) && ((ball->x + ball->sizew + 1) == p2->x)) {
    ball->dx *= -1;
  }

  // Move ball
  ball->x += ball->dx;
  ball->y += ball->dy;

  return 0;
}

// Tracks the ball and moves AI paddle
void ai_movement(struct Shape* ball, struct Shape* computer, int random) {
  // If statement to make the AI beatable
  if (random % 2 && ball->x < 180) {
    return;
  }

  // If statement that tracks the ball and moves AI paddle
  if (ball->y < computer->y) {
    if (computer->y > 0) {
      computer->y -= 1;
    }
    return;
  }
  else if (ball->y > (computer->y + computer->sizel)) {
    if ((computer->y + computer->sizel) < 159) {
      computer->y += 1;
    }
    return;
  }
}

/* clear the screen to black */
void update_shape(volatile unsigned short* buffer, unsigned short color, struct Shape* s) {
    short row, col;
    /* set each pixel black */
    for (row = s->y - 5; row < (s->y + s->sizel + 5); row++) {
        for (col = s->x - 5; col < (s->x + s->sizew + 5); col++) {
          if (!(row < 0 || row > 159 || col < 0 || col > 239)) { 
            put_pixel(buffer, row, col, color);
          }
        }
    }
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short* flip_buffers(volatile unsigned short* buffer) {
    /* if the back buffer is up, return that */
    if(buffer == front_buffer) {
        /* clear back buffer bit and return back buffer pointer */
        *display_control &= ~SHOW_BACK;
        return back_buffer;
    } else {
        /* set back buffer bit and return front buffer */
        *display_control |= SHOW_BACK;
        return front_buffer;
    }
}

/* handle the buttons which are pressed down */
void handle_buttons(struct Shape* s) {
    /* move the square with the arrow keys */
    if (button_pressed(BUTTON_DOWN)) {
      if ((s->y + s->sizel + 1) > 159) {
        return;
      }
      s->y += 1;
    }
    if (button_pressed(BUTTON_UP)) {
      if (s->y == 0) {
        return;
      }  
      s->y -= 1;
    }
}

/* clear the screen to black */
void clear_screen(volatile unsigned short* buffer, unsigned short color) {
    unsigned short row, col;
    /* set each pixel black */
    for (row = 0; row < HEIGHT; row++) {
        for (col = 0; col < WIDTH; col++) {
            put_pixel(buffer, row, col, color);
        }
    }
}

/* the main function */
int main( ) {
    /* we set the mode to mode 4 with bg2 on */
    *display_control = MODE4 | BG2;

    // Add colors
    unsigned char black = add_color(0, 0, 0);
    unsigned char green = add_color(0, 20, 2);

    // Generate random number for direction of ball
    srand(100);
    int y = rand() % 2;

    /* make a green ball and paddles */
    struct Shape player = {10, 10, 15, 2, green, 0, 0};
    struct Shape computer = {228, 10, 15, 2, green, 0, 0};
    struct Shape ball = {119, 79, 2, 2, green, 1, y};

    // Int to store 1 if there was a score or 0 if there wasn't
    int scored = 0;

    /* the buffer we start with */
    volatile unsigned short* buffer = front_buffer;

    /* clear whole screen first */
    clear_screen(front_buffer, black);
    clear_screen(back_buffer, black);

    /* loop forever */
    while (1) {
        /* clear the screen - only the areas around the paddles and ball! */
        update_shape(buffer, black, &player);
        update_shape(buffer, black, &computer);
        update_shape(buffer, black, &ball);

        /* draw the paddles and the ball */
        draw_shape(buffer, &player);
        draw_shape(buffer, &computer);
        draw_shape(buffer, &ball);

        /* handle button input & ball movement & AI movement */
        handle_buttons(&player);
        scored = ball_movement(&ball, &player, &computer);
        ai_movement(&ball, &computer, rand() % 100);

        /* wait for vblank before switching buffers */
        wait_vblank();

        /* Check if there was a score */
        if (scored == 1) {
          // Clear ball from both buffers
          update_shape(buffer, black, &ball);
          wait_vblank();
          buffer = flip_buffers(buffer);
          update_shape(buffer, black, &ball);

          // Set the ball to start at center of screen
          ball.x = 119;
          ball.y = 79;
          // Set random ball direction
          ball.dy = rand() % 2;
        }
        else {
          // Swap the buffers
          buffer = flip_buffers(buffer);
        }
        
    }
}

/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore( ) {
    /* do nothing */
}

/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)( );
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};

