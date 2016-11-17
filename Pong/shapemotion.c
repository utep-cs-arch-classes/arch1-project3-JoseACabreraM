/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
 
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
 
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}}; // 10x10 rectangle 
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30}; // A Right Arrow 
AbRect pongBar = {abRectGetBounds, abRectCheck, {14,3}}; // Bars for Pong

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 3, screenHeight/2 - 1}
};

static int topPongBarXPosition = 0;
static int bottomPongBarXPosition = 0;

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};

Layer topPongBar = {
  (AbShape *)&pongBar,
  {(screenWidth/2), 30},
  {0,0}, {0,0},
  COLOR_BLACK,
  &fieldLayer
};

Layer bottomPongBar = {
(AbShape *)&pongBar,
  {(screenWidth/2), screenHeight-30},
  {0,0}, {0,0},
  COLOR_BLACK,
  &topPongBar
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &bottomPongBar,
};

/* initial value of {0,0} will be overwritten */
//MovLayer mTestLayer = { &testLayer, {4,4}, 0};

MovLayer mtopPongBar = {&topPongBar, {1,1}, 0};
MovLayer mbottomPongBar = {&bottomPongBar, {1,1}, 0};
MovLayer ml0 = {&layer0, {3,2}, 0}; 

movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
 
void mlAdvance(MovLayer *ml, Region *fence, Region *topPongBarFence, Region *bottomPongBarFence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis++) {
      // If the ball hits the fence
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])){
	    int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	    newPos.axes[axis] += (2*velocity);
      }
      // If the ball hits the top pong bar, only checking on the Y-Axis 
      if (detectCollisionTop(&pongBar, &(topPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
      // If the ball hits the bottom pong bar, only checking on the Y-Axis
      if (detectCollisionBottom(&pongBar, &(bottomPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
      }
    } 
    ml->layer->posNext = newPos;
  }
}

int detectCollisionTop(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos;
  for (i = -7; i < 7; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] += 10;
    int collision = abRectCheck(rect, &centerPos1, pixel);
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] -= 10;
  }
  return 0;
}

int detectCollisionBottom(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos;
  for (i = -7; i < 7; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] -= 10;
    int collision = abRectCheck(rect, &centerPos1, pixel);
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] += 10;
  }
  return 0;
}

void movtopPongBar(int switches){
    mtopPongBar.layer->posNext.axes[0] = mtopPongBar.layer->pos.axes[0] + topPongBarXPosition;
    if (!(BIT0&switches)){
      if (mtopPongBar.layer->pos.axes[0] >= 20){
	topPongBarXPosition = -5;
      } else {
	topPongBarXPosition = 0;
      }
    } else if (!(BIT1 & switches)){
      if (mtopPongBar.layer->pos.axes[0] <= screenWidth - 20){
	topPongBarXPosition = 5;
      }  else {
	topPongBarXPosition = 0;
      }
    } else {
	topPongBarXPosition = 0;
    }
    movLayerDraw(&mtopPongBar, &topPongBar);
}

void movbottomPongBar(int switches){
    mbottomPongBar.layer->posNext.axes[0] = mbottomPongBar.layer->pos.axes[0] + bottomPongBarXPosition;
    if (!(BIT2 &switches)){
      if (mbottomPongBar.layer->pos.axes[0] >= 20){
	    bottomPongBarXPosition = -10;
      } else {
	    bottomPongBarXPosition = 0;
      }
    } else if (!(BIT3 & switches)){
      if (mbottomPongBar.layer->pos.axes[0] <= screenWidth - 20){
	    bottomPongBarXPosition = 10;
      }  else {
	    bottomPongBarXPosition = 0;
      }
    } else {
	    bottomPongBarXPosition = 0;
    }
    movLayerDraw(&mbottomPongBar, &bottomPongBar);
}


u_int bgColor = COLOR_BLUE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region topPongBarFence;
Region bottomPongBarFence;

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(BIT0 + BIT1 + BIT2 + BIT3);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence); 
  layerGetBounds(&topPongBar,&topPongBarFence); 
  layerGetBounds(&bottomPongBar,&bottomPongBarFence);
   
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    drawString5x7(7, 5, "Player 1", COLOR_WHITE, COLOR_BLUE);
    drawString5x7(screenWidth/2 + 10, screenHeight-10, "Player 2", COLOR_WHITE, COLOR_BLUE);
    int switches = p2sw_read();
    movtopPongBar(switches);
    movbottomPongBar(switches);
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence, &topPongBarFence, &bottomPongBarFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
