/*
 * game.c
 * program which demonstrates my understanding of gba programming
 */

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// NEED TO UPDATE
/* include the background image we are using */
#include "background.h"

// NEED TO UPDATE
/* include the sprite image we are using */
#include "droid2.h"
#include "sprites.h"

// NEED TO UPDATE
/* include the tile map we are using */
#include "map.h"
#include "map2.h"


/* the tile mode flags needed for display control register */
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02

#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;

/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

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


// Assembly Code
int bulletfired(int r0, int r1);
int brain1(int r0, int r1, int r2);


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

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    /* set all control the bits in this register */
    *bg0_control = 1 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (16 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    *bg1_control = 0 |
        (0 << 2)  |
        (0 << 6)  |
        (1 << 7)  |
        (24 << 8) |
        (1 << 13) |
        (0 << 14);


    /* load the tile data into screen block 16 */
    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map, map_width * map_height);
    memcpy16_dma((unsigned short*) screen_block(24), (unsigned short*) map2, map2_width * map2_height);
}

/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /* rendering mode */
                            (0 << 10) |         /* gfx mode */
                            (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode, 0:16, 1:256 */
                            (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the sprites on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) sprites_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) sprites_data, (sprites_width * sprites_height) / 2);
}

/* a struct for the koopa's logic and behavior */
struct Koopa {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion, in 1/256 pixels */
    int x, y;

    /* the koopa's y velocity in 1/256 pixels/second */
    int yvel;

    /* the koopa's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the koopa is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the koopa stays */
    int border;

    /* if the koopa is currently falling */
    int falling;

    /* which direction is the sprite facing */
    int direction;

    /* distance traveled by sprite */
    int distance;

    /* timer for droid fire */
    int timer;

	/* Is this sprite dead */
	int dead;
};

/* initialize the droid/player */
void droid_init(struct Koopa* droid) {
    droid->x = 100 << 8;
    droid->y = 75 << 8;
    droid->yvel = 0;
    droid->gravity = 50;
    droid->border = 40;
    droid->frame = 0;
    droid->move = 0;
    droid->counter = 0;
    droid->falling = 0;
    droid->animation_delay = 8;
    droid->direction = 1;
    droid->timer = 0;
	droid->dead = 0;
    droid->sprite = sprite_init(droid->x >> 8, droid->y >> 8, SIZE_32_64, 1, 0, droid->frame, 0);
}

// initialize the koopa/enemies
void koopa_init(struct Koopa* koopa, int random) {
    koopa->x = random << 8;
    koopa->y = 200 << 8;
    koopa->yvel = 0;
    koopa->gravity = 50;
    koopa->border = 40;
    koopa->frame = 0;
    koopa->move = 1;
    koopa->counter = 0;
    koopa->falling = 0;
    koopa->animation_delay = 8;
    koopa->direction = 1;
	koopa->dead = 0;
    koopa->sprite = sprite_init(koopa->x >> 8, koopa->y >> 8, SIZE_32_64, 1, 0, koopa->frame, 0);
}

// initialize the bullets from the droid rifle
void bullet_init(struct Koopa* bullet) {
    bullet->x = SCREEN_WIDTH;
    bullet->y = SCREEN_HEIGHT;
    bullet->yvel = 0;
    bullet->gravity = 0;
    bullet->border = 40;
    bullet->frame = 250;
    bullet->move = 0;
    bullet->counter = 0;
    bullet->falling = 0;
    bullet->animation_delay = 8;
    bullet->direction = -1;
    bullet->distance = 0;
    bullet->sprite = sprite_init(bullet->x, bullet->y, SIZE_32_8, 0, 0, bullet->frame, 0);
}

// Fires the bullets from the droid
int droid_fire(struct Koopa* droid, struct Koopa* bullet) {
  if (droid->timer == 0) {  
	int pass = bulletfired(droid->direction, bullet->move);
    if (/*droid->direction == 1 && bullet->move != 1*/pass) {
          bullet->x = (droid->x >> 8) + 18;
          bullet->y = (droid->y >> 8) + 22;
          bullet->move = 1;
          bullet->direction = 1;
          sprite_set_horizontal_flip(bullet->sprite, 1);
          sprite_position(bullet->sprite, bullet->x, bullet->y);        
      }
      else if (bullet->move != 1) {
          bullet->x = (droid->x >> 8) - 16;
          bullet->y = (droid->y >> 8) + 22;
          bullet->move = 1;
          bullet->direction = 0;
          sprite_set_horizontal_flip(bullet->sprite, 0);
          sprite_position(bullet->sprite, bullet->x, bullet->y);
      }
      droid->timer = 5;
  }
  else {
    droid->timer -= 1;
  }
}

/* move the koopa left or right returns if it is at edge of the screen */
int koopa_left(struct Koopa* koopa) {
    /* face left */
    // Depending on the type of sprite
    if (koopa->direction != -1) {
      sprite_set_horizontal_flip(koopa->sprite, 0);
      koopa->direction = 0;
    }
    else {
      sprite_set_horizontal_flip(koopa->sprite, 1);
    }
    koopa->move = 1;

    /* if we are at the left end, just scroll the screen */
    if ((koopa->x >> 8) < koopa->border) {
        return 1;
    } else {
        /* else move left */
        koopa->x -= 256;
        return 0;
    }
}
int koopa_right(struct Koopa* koopa) {
    /* face right */
    // Depending on the type of sprite
    if (koopa->direction != -1) {
      sprite_set_horizontal_flip(koopa->sprite, 1);
      koopa->direction = 1;
    }
    else {
      sprite_set_horizontal_flip(koopa->sprite, 0);
    }
    koopa->move = 1;

    /* if we are at the right end, just scroll the screen */
    if ((koopa->x >> 8) > (SCREEN_WIDTH - 16 - koopa->border)) {
        return 1;
    } else {
        /* else move right */
        koopa->x += 256;
        return 0;
    }
}

/* stop the koopa from walking left/right */
void koopa_stop(struct Koopa* koopa) {
    koopa->move = 0;
    koopa->frame = 0;
    koopa->counter = 7;
    sprite_set_offset(koopa->sprite, koopa->frame);
}

/* start the koopa jumping, unless already fgalling */
void koopa_jump(struct Koopa* koopa) {
    if (!koopa->falling) {
        koopa->yvel = -1500;
        koopa->falling = 1;
    }
}

/* finds which tile a screen coordinate maps to, taking scroll into account */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* lookup this tile from the map */
    int index = y * tilemap_w + x;

    /* return the tile */
    return tilemap[index];
}


/* update the koopa */
void koopa_update(struct Koopa* koopa, int xscroll, int random) {
	if (koopa->dead) {
		koopa_init(koopa, random);
		return;
	}    
	/* update y position and speed if falling */
    if (koopa->falling) {
        koopa->y += koopa->yvel;
        koopa->yvel += koopa->gravity;
    }

    /* check which tile the koopa's feet are over */
    unsigned short tile = tile_lookup((koopa->x >> 8) + 8, (koopa->y >> 8) + 56, xscroll,
            0, map2, map2_width, map2_height);

    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the koopa can walk on */
    if ((tile >= 48 && tile <= 52) || 
        (tile >= 65 && tile <= 92)) {
        /* stop the fall! */
        koopa->falling = 0;
        koopa->yvel = 0;

        /* make him line up with the top of a block
         * works by clearing out the lower bits to 0 */
        koopa->y &= ~0x7ff;

        /* move him down one because there is a one pixel gap in the image */
        koopa->y++;

    } else {
        /* he is falling now */
        koopa->falling = 1;
    }


    /* update animation if moving */
    if (koopa->move) {
        koopa->counter++;
        if (koopa->counter >= koopa->animation_delay) {
            koopa->frame = koopa->frame + 64;
            if (koopa->frame > 64) {
                koopa->frame = 0;
            }
            sprite_set_offset(koopa->sprite, koopa->frame);
            koopa->counter = 0;
        }
    }

    /* set on screen position */
    sprite_position(koopa->sprite, koopa->x >> 8, koopa->y >> 8);
}


// Update the bullet location
void bullet_update(struct Koopa* bullet){
    if (bullet->move) {
      if (bullet->direction == 1) {
        sprite_move(bullet->sprite, 8, 0);
        bullet->distance += 8;
      }
      else {
        sprite_move(bullet->sprite, -8, 0);
        bullet->distance += 8;
      }
      if (bullet->distance > 200) {
          bullet->distance = 0;
          bullet->direction = -1;
          bullet->move = 0;
          bullet->x = SCREEN_WIDTH;
          bullet->y = SCREEN_HEIGHT;
          sprite_position(bullet->sprite, SCREEN_WIDTH, SCREEN_HEIGHT);
      }
    }   
}


// Koopa enemy movement
void enemy_move(struct Koopa* koopa, int random) {
    int jump = 0;

    if (random % 2) {
      jump = 1;
      koopa_jump(koopa);
    }

    if (koopa->direction == 0) {
      koopa_left(koopa);
    }
    else {
      koopa_right(koopa);
    }
}


// Koopa brain
void koopa_brain(struct Koopa* droid, struct Koopa* koopa, int random) {
    if (random % 2) {
        return;
    }
 	int pass = brain1(koopa->direction, droid->x, koopa->x);
    if (/*koopa->direction == 0 && droid->x < koopa->x*/pass) {
        enemy_move(koopa, random);
    }
    else if (koopa->direction == 0 && droid->x > koopa->x) {
        sprite_set_horizontal_flip(koopa->sprite, 0);
        koopa->direction = 1;
    }
    else if (koopa->direction == 1 && droid->x > koopa->x) {
        enemy_move(koopa, random);
    }
    else if (koopa->direction == 1 && droid->x < koopa->x) {
        sprite_set_horizontal_flip(koopa->sprite, 1);
        koopa->direction = 0;
    }
}

// attack
void death(struct Koopa* koopa, struct Koopa* bullet) {
//	if ( (bullet->x + 32) == (koopa->x) && (bullet->y >= koopa->y && bullet->y <= (koopa->y + 64)) ) {
//		koopa->dead = 1;
//	}
//	if ( (bullet->x == (koopa->x + 32)) && ( bullet->y >= koopa->y && bullet->y <= (koopa->y + 64)) ) {
//		koopa->dead = 1;
//	}
	if ( (bullet->x + 32) == (koopa->x) && (bullet->y >= koopa->y && bullet->y <= (koopa->y + 64)) ) {
		koopa->dead = 1;
	}
	if ( (bullet->x == (koopa->x + 32)) && ( bullet->y <= koopa->y && bullet->y >= (koopa->y + 64)) ) {
		koopa->dead = 1;
	}
}


/* the main function */
int main( ) {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

	// Start random number generator
    srand(100);
    int random = rand() % 2;

    /* create the player */
    struct Koopa droid;
    droid_init(&droid);

    /* create the koopa droids */
    struct Koopa koopas[3];
    int i;
    for (i = 0; i < 3; i++) {
      koopa_init(&koopas[i], rand() % 200);
    }

    struct Koopa bullets[13];
    for (i = 0; i < 13; i++) {
      bullet_init(&bullets[i]);
    }

    /* set initial scroll to 0 */
    int xscroll0 = 0;
    int xscroll1 = 0;
    int koopacounter = 0;
    int bulletcounter = 0;
	int j = 0;

    /* loop forever */
    while (1) {
        
        /* update the koopa */
        koopa_update(&droid, xscroll1, 0);
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 13; j++) {
				death(&koopas[j], &bullets[i]);
			}
		}        

        for (i = 0; i < 13; i++) {
          bullet_update(&bullets[i]);		  
        }
		for (i = 0; i < 3; i++) {
			koopa_update(&koopas[i], xscroll0, rand() % 200);
			koopa_brain(&droid, &koopas[i], rand() % 100);
		}

        /* now the arrow keys move the koopa */
        if (button_pressed(BUTTON_RIGHT)) {
            if (koopa_right(&droid)) {
                xscroll1++;
                xscroll0 += 2;
            }
        } else if (button_pressed(BUTTON_LEFT)) {
            if (koopa_left(&droid)) {
                xscroll1--;
                xscroll0 -= 2;
            }
        } else {
            koopa_stop(&droid);
        }
        //koopa_brain(&droid, &koopas[0], rand() % 100);

        /* check for jumping */
        if (button_pressed(BUTTON_A)) {
            koopa_jump(&droid);
        }

        if (button_pressed(BUTTON_B)) {
          droid_fire(&droid, &bullets[bulletcounter]);
          if (bulletcounter < 12) {
              bulletcounter++;
          }
          else {
              bulletcounter = 0;
          }
        }

        /* wait for vblank before scrolling and moving sprites */
        wait_vblank();
        *bg0_x_scroll = xscroll0;
        *bg1_x_scroll = xscroll1;
        sprite_update_all();

        /* delay some */
        delay(300);
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

