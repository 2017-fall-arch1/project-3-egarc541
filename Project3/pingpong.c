#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "pingpong.h"

#define GREEN_LED BIT6

int play, score1, score2;
char game[4] = "0123";

AbRect paddleRec = {abRectGetBounds, abRectCheck, {2,15}};

AbRectOutline fieldOutline = {    /* Playing field*/
  abRectOutlineGetBounds, abRectOutlineCheck,
  {screenWidth/2-12, screenHeight/2-12}
};

Layer rightPaddle= {   /** Right Paddle */
  (AbShape *) &paddleRec,
  {screenWidth/2+48, screenHeight/2+48},
  {0,0},{0,0},
  COLOR_RED,
  0
};

Layer fieldLayer ={
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},
  {0,0},{0,0},
  COLOR_BLUE,
  &rightPaddle,
};


  
Layer leftPaddle = {
  (AbShape *) &paddleRec,
  {screenWidth/2 -48, screenHeight/2-48},
  {0,0}, {0,0},
  COLOR_BLUE,
  &fieldLayer,
};


Layer ball = {
  (AbShape *)&circle8,
  {(screenWidth/2)+5, (screenHeight/2)+5},
  {0,0}, {0,0},
  COLOR_GREEN,
  &leftPaddle,
};

MovLayer ml0 = { &rightPaddle, {0,0}, 0};
MovLayer ml1 ={ &leftPaddle, {0,0}, &ml0};
MovLayer ml2 = {&ball, {2,2}, &ml1};

void movLayerDraw( MovLayer *movLayers, Layer *layers){
  int row,col;
  MovLayer *movLayer;

  and_sr(~8);
  for( movLayer = movLayers; movLayer; movLayer = movLayer -> next){
    Layer *l = movLayer -> layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);

  for(movLayer = movLayers; movLayer; movLayer = movLayer-> next)
    {
      Region bounds;
      layerGetBounds(movLayer-> layer, &bounds);
      lcd_setArea(bounds.topLeft.axes[0],bounds.topLeft.axes[1],
		  bounds.botRight.axes[0],bounds.botRight.axes[1]);
      for(row=bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1];row++){
	for(col= bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++){
	  Vec2 pixelPos= {col, row};
	  u_int color = bgColor;
	  Layer *probeLayer;
	  for( probeLayer = layers; probeLayer; probeLayer = probeLayer = probeLayer ->next){
	    if(abShapeCheck(probeLayer -> abShape, &probeLayer -> pos, &pixelPos)){
	      color = probeLayer -> color;
	      break;
	    }
	  }
	  lcd_writeColor(color);
	} // for col
      } // for row
    } // for moving layer
}

int collision_detector (Vec2 *newPos, u_int axis){
  int velocity2=0;
  if(abShapeCheck(ml0.layer->abShape, &ml0.layer->posNext, newPos) ||
     (abShapeCheck(ml1.layer->abShape, &ml1.layer->posNext, newPos))){
    velocity2 = ml2.velocity.axes[axis] = - ml2.velocity.axes[axis];
    return velocity2;
  }
}
  

void mlAdvance(MovLayer *ml, Region *fence){

  Vec2 newPos;
  u_char axis;
  u_char counter, counter2;
  Region shapeBoundary;

  for(; ml; ml= ml-> next){
    vec2Add(&newPos, &ml ->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml ->layer->abShape, &newPos,
		     &shapeBoundary);

    counter =0;
    counter2 =0;
    
    for(axis = 0; axis < 2; axis ++){
      if(shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      
	if(ml->layer->abShape == ml2.layer->abShape & (counter<1)){
	score1+=1;
	counter += 1;
	
	}
      }
      else if( shapeBoundary.botRight.axes[axis] >  fence->botRight.axes[axis]){
	int velocity= ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      
	if(ml->layer->abShape == ml2.layer->abShape & (counter2<1)){
	  score2 +=1;
	  counter2 +=1;
	}
      }
	
      if(ml->layer->abShape == ml2.layer->abShape){
	int velocity = collision_detector(&newPos,axis);
	newPos.axes[axis] += (2*velocity);
    }
    ml->layer->posNext = newPos;
   }
  }
}

u_int bgColor =COLOR_BLACK;
int redrawScreen = 1;

Region fieldFence;

void main(){

  play=1;
 
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&ball);
  layerDraw(&ball);

  drawString5x7(screenWidth/2-25,150,"PING PONG", COLOR_GREEN,COLOR_BLACK);

  drawString5x7(screenWidth/2-50,2, "SCORE:", COLOR_GREEN, COLOR_BLACK);

  drawString5x7(screenWidth/2+30,2,"EG", COLOR_GREEN, COLOR_BLACK);
  layerGetBounds(&fieldLayer, &fieldFence);
  
  
  enableWDTInterrupts();
  or_sr(0x8);


  for(;;){
    while(!redrawScreen){
      or_sr(0x10);
    }
    redrawScreen = 0;
    movLayerDraw(&ml2,&ball);

    drawChar5x7(screenWidth/2+10,2,game[score1], COLOR_RED,COLOR_BLACK);
    drawString5x7(screenWidth/2,2,"-", COLOR_GREEN, COLOR_BLACK);
    drawChar5x7(screenWidth/2-10,2,game[score2], COLOR_BLUE, COLOR_BLACK);

  }
}

void wdt_c_handler(){

  static short count =0;
  P1OUT |= GREEN_LED;
  count++;
  if(count == 15) {
      mlAdvance(&ml2, &fieldFence);

      u_int switches = ~p2sw_read();

      if(S1 & switches){
      ml1.velocity.axes[1] =3;
    }
      else if(S2 & switches){
     ml1.velocity.axes[1]= -3;
    }

    else if(S3 & switches){
     ml0.velocity.axes[1] = 3;
    }
    else if(S4 & switches){
      ml0.velocity.axes[1] = -3;
    }
    else{
     ml1.velocity.axes[1] = 0;
      ml0.velocity.axes[1] = 0;
    }
    redrawScreen = 1;
    count = 0;
  }
}
      
	
