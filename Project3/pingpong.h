#ifndef pingpong_included
#define pingpong_included

#define S1 BIT0
#define S2 BIT1
#define S3 BIT2
#define S4 BIT3

typedef struct MovLayer_s{
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

void movLayerDraw(MovLayer *movLayers, Layer *layers);
void mlAdvance(MovLayer *ml, MovLayer *ml1, MovLayer *ml2, Region *fence);
void main();
void wdt_c_handler();

#endif
	       
