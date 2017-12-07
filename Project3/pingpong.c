#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "pingpong.h"

int play, score1, score2;
char game[10] = "0123456789";

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

MovLayer rightP= { &rightPaddle, {0,0}, 0};
MovLayer leftP ={ &leftPaddle, {0,0}, 0};
MovLayer mball = {&ball, {2,2}, &leftP};

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
	      
void mlAdvance(MovLayer *ml, MovLayer *ml1, MovLayer *ml2, Region *fence){

  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for(; ml; ml= ml-> next){
    vec2Add(&newPos, &ml ->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml ->layer->abShape, &newPos,
		     &shapeBoundary);
    for(axis = 0; axis < 2; axis ++){
      if((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	 (shapeBoundary.botRight.axes[axis]> fence->botRight.axes[axis])) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      if(ml->layer->abShape == mball.layer->abShape){
	if(abShapeCheck(ml1->layer->abShape, &ml1->layer->posNext,&newPos)){
	  int velocity = mball.velocity.axes[axis] = - mball.velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
	}
      }
      
      if(abShapeCheck(ml2->layer->abShape, &ml2->layer->posNext, &newPos)){
	int velocity = mball.velocity.axes[axis] = -mball.velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      if((shapeBoundary.topLeft.axes[axis] < fence-> topLeft.axes[axis]) &&
	 !(abShapeCheck(ml2->layer->abShape, &ml2->layer->posNext,
			&newPos))){
	score1++;
      }
      else if((shapeBoundary.botRight.axes[axis] < fence-> botRight.axes[axis]) &&
	      !(abShapeCheck(ml1->layer->abShape, &ml1->layer->posNext,
			     &newPos))){
	score2++;
      }
    }
    ml->layer->posNext = newPos;
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
    movLayerDraw(&mball,&ball);

    drawChar5x7(screenWidth/2+10,2,game[score1], COLOR_RED,COLOR_BLACK);
    drawString5x7(screenWidth/2,2,"-", COLOR_GREEN, COLOR_BLACK);
    drawChar5x7(screenWidth/2-10,2,game[score2], COLOR_BLUE, COLOR_BLACK);

  }
}

void wdt_c_handler(){

  static short count =0;
  count++;
  if(count == 15) {
    if((S1 & p2sw_read()) && (S3 & p2sw_read())){
      play=0;
    }

    if(!play){
      redrawScreen = 1;
      mlAdvance(&mball, &leftP, &rightP, &fieldFence);

    }

    if((S1 & p2sw_read())==0){
      leftP.velocity.axes[1] =4;
    }
    else if((S2 & p2sw_read()) ==0){
      leftP.velocity.axes[1]= -4;
    }
    else{
      leftP.velocity.axes[1]=0;
    }

    if((S3 & p2sw_read()) == 0){
      rightP.velocity.axes[1] = 4;
    }
    else if((S4 & p2sw_read()) == 0){
      rightP.velocity.axes[1] = -4;
    }
    else{
      rightP.velocity.axes[1] =0;
    }
    count = 0;
  }
}
      
	
