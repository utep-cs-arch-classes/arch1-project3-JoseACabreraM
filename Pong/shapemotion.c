/** \File shapemotion.c
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
static char playerOneScore = '0';
static char playerTwoScore = '0';
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
  COLOR_WHITE,
  &fieldLayer
};

Layer bottomPongBar = {
(AbShape *)&pongBar,
  {(screenWidth/2), screenHeight-30},
  {0,0}, {0,0},
  COLOR_WHITE,
  &topPongBar
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)-10}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &bottomPongBar,
};

/* initial value of {0,0} will be overwritten */
//MovLayer mTestLayer = { &testLayer, {4,4}, 0};

MovLayer mtopPongBar = {&topPongBar, {1,1}, 0};
MovLayer mbottomPongBar = {&bottomPongBar, {1,1}, 0};
MovLayer ml0 = {&layer0, {0,0}, 0}; 

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

static int velocityLimit = 0;
static int incrementX = 0;
static int collisionBottomOccurred = 0;
static int collisionTopOccured = 0;
static int difficultyMode = 0;

void incrementBallVelocity(MovLayer *pongBall){

  if (velocityLimit < (8 + difficultyMode)){
    if ( incrementX == 2) {
	if (pongBall->velocity.axes[0] < 0){
	  pongBall->velocity.axes[0]--;
	  //pongBall->velocity.axes[0] -= difficultyMode;
	} else {
	  pongBall->velocity.axes[0]++;
	  //pongBall->velocity.axes[0] += difficultyMode;
	}
	incrementX = 0;
    } else {
      if (pongBall->velocity.axes[1] < 0){
	pongBall->velocity.axes[1]--;
	//pongBall->velocity.axes[1] -= difficultyMode;
      } else {
	pongBall->velocity.axes[1]++;
	//pongBall->velocity.axes[1] += difficultyMode;
      }
      incrementX++;
    }
  
  velocityLimit++;
  }
  
}

void resetBallVelocity(MovLayer *pongBall){
  velocityLimit = 0;
  pongBall->velocity.axes[0] = 2;
  pongBall->velocity.axes[1] = 1;
}

void resetPongBars(){
  mtopPongBar.layer->posNext.axes[0] = screenWidth/2;
  mbottomPongBar.layer->posNext.axes[0] = screenWidth/2;
  movLayerDraw(&mtopPongBar, &topPongBar);
  movLayerDraw(&mbottomPongBar, &bottomPongBar);
}

void playCollisionSound(){
  CCR0 = 2000;
  CCR1 = 2000 >> 1;
}

void resetSound(){
  CCR0 = 0;
  CCR1 = 0 >> 1;
}

void handleCollisionOnFence(Vec2* newPos, MovLayer* ml){
  newPos->axes[1] = (screenHeight/2)-10;
  newPos->axes[0] = (screenWidth/2)+10;
  resetBallVelocity(ml);
  resetPongBars();
  collisionBottomOccurred = 0;
  collisionTopOccured = 0;
}

void handleCollisionOnBar(){
  
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

  resetSound();
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    for (axis = 0; axis < 2; axis++) {
      // If the ball hits the fence
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])){
	if(detectCollisionTopFence(&fieldOutline, &(fieldLayer.pos), &(ml->layer->pos)) && axis == 1){
	  handleCollisionOnFence(&newPos, ml);
	  playerTwoScore++;
	  //playerTwoScore %= ':';
	} else if(detectCollisionBottomFence(&fieldOutline, &(fieldLayer.pos), &(ml->layer->pos)) && axis == 1){
	  handleCollisionOnFence(&newPos, ml);
	  playerOneScore++;
	  //playerOneScore %= ':';
	} else { 
	  int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
	}
      }
      // If the ball hits the top pong bar, only checking on the Y-Axis 
      if (detectCollisionTop(&pongBar, &(topPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	incrementBallVelocity(ml);
	collisionBottomOccurred = 0;
	collisionTopOccured = 1;
	playCollisionSound();
      }
      // If the ball hits the bottom pong bar, only checking on the Y-Axis
      if (detectCollisionBottom(&pongBar, &(bottomPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	incrementBallVelocity(ml);
	collisionBottomOccurred = 1;
	collisionTopOccured = 0;
	playCollisionSound();
      }
    }
    
    ml->layer->posNext = newPos;
  }
}



int detectCollisionBottomFence(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos;
  for (i = -100; i < 100; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] += 50;
    int collision = abRectCheck(rect, &centerPos1, pixel);
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] -= 50;
  }
  return 0;
}

int detectCollisionTopFence(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos;
  for (i = -100; i < 100; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] -= 50;
    int collision = abRectCheck(rect, &centerPos1, pixel);
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] += 50;
  }
  return 0;
}

int detectCollisionTop(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i;
  int j;
  Vec2 centerPos1 = *centerPos;
  if (!collisionTopOccured){
    for (j = 8; j < 12; j++){
      for (i = -7; i < 7; i++){
	centerPos1.axes[0] += i;
	centerPos1.axes[1] += j;
	int collision = abRectCheck(rect, &centerPos1, pixel);
	if (collision){
	  return 1;
	}
	centerPos1.axes[0] -= i;
	centerPos1.axes[1] -= j;
      }
    }
  }
  return 0;
}

int detectCollisionBottom(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  int j = 10;
  Vec2 centerPos1 = *centerPos;
  if (collisionBottomOccurred == 0){
    for (j = 8; j < 12;j++){
      for (i = -7; i < 7; i++){
	centerPos1.axes[0] += i;
	centerPos1.axes[1] -= j;
	int collision = abRectCheck(rect, &centerPos1, pixel);
	if (collision){
	  return 1;
	}
	centerPos1.axes[0] -= i;
	centerPos1.axes[1] += j;
      }
    }
  }
  return 0;
}

void movtopPongBar(int switches){
    mtopPongBar.layer->posNext.axes[0] = mtopPongBar.layer->pos.axes[0] + topPongBarXPosition;
    if (!(BIT0&switches)){
      if (mtopPongBar.layer->posNext.axes[0] >= 30){
	topPongBarXPosition = -12;
      } else {
	topPongBarXPosition = 0;
      }
    } else if (!(BIT1 & switches)){
      if (mtopPongBar.layer->posNext.axes[0] <= screenWidth - 30){
	topPongBarXPosition = 12;
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
      if (mbottomPongBar.layer->posNext.axes[0] >= 30){
	    bottomPongBarXPosition = -12;
      } else {
	    bottomPongBarXPosition = 0;
      }
    } else if (!(BIT3 & switches)){
      if (mbottomPongBar.layer->posNext.axes[0] <= screenWidth - 30){
	    bottomPongBarXPosition = 12;
      }  else {
	    bottomPongBarXPosition = 0;
      }
    } else {
	    bottomPongBarXPosition = 0;
    }
    movLayerDraw(&mbottomPongBar, &bottomPongBar);
}



u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

static int gameResetCounter = 0;
static int modeSelector = 0;
static char* winner;
int switches;
 
Region fieldFence;		/**< fence around playing field  */
Region topPongBarFence;
Region bottomPongBarFence;

void selectMode(){
  layerInit(&layer0);
  if (modeSelector == 0){
    drawString5x7(15, 40, "    PONG", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 60, "BTN1 - Start", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 80, "BTN2 - Set Easy", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 100, "BTN3 - Set Medium", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 120, "BTN4 - Set Hard", COLOR_WHITE, COLOR_BLACK);
    if (!(BIT0 & switches)) {
      clearScreen(0);
      _delay(50);
      layerInit(&layer0);
      ml0.velocity.axes[0] = 2;
      ml0.velocity.axes[1] = 1;
      playerTwoScore = '0';
      playerOneScore = '0';
      modeSelector = 1;
    } else if (!(BIT1 & switches)){
      difficultyMode = -4;
    } else if (!(BIT2 & switches)){
      difficultyMode = 0;
    } else if (!(BIT3 & switches)){
      difficultyMode = 4;
    }
  }
  if (modeSelector == 2){
    //drawString5x7(15, 60, "", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(7, 5, "Player 1 - ", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(screenWidth/2 - 2, screenHeight-10, "- Player 2", COLOR_WHITE, COLOR_BLACK);
    drawChar5x7(75, 5, playerOneScore, COLOR_WHITE, COLOR_BLACK);
    drawChar5x7(screenWidth/2 - 15, screenHeight-10, playerTwoScore, COLOR_WHITE, COLOR_BLACK);
    drawString5x7(10, 60, "  Game Over", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 80, winner, COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 100, " BTN1 - Menu", COLOR_WHITE, COLOR_BLACK);
    
    if (!(BIT0 & switches)) {
      clearScreen(0);
      difficultyMode = 0;
      modeSelector = 0;
      _delay(50);
    }
    
  }
  
}

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  //shapeInit();
  p2sw_init(BIT0 + BIT1 + BIT2 + BIT3);

  timerAUpmode();
  P2SEL &= ~(BIT6 | BIT7);
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;

  clearScreen(0);
  shapeInit();
  layerInit(&layer0);
  //layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence); 
  layerGetBounds(&topPongBar,&topPongBarFence); 
  layerGetBounds(&bottomPongBar,&bottomPongBarFence);
   
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) {
    switches = p2sw_read();
    if (modeSelector != 1){
      selectMode();
    } else {
      while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
	P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
	or_sr(0x10);	      /**< CPU OFF */
      }

      P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
      redrawScreen = 0;
    
      drawString5x7(7, 5, "Player 1 - ", COLOR_WHITE, COLOR_BLACK);
      drawString5x7(screenWidth/2 -2, screenHeight-10, "- Player 2", COLOR_WHITE, COLOR_BLACK);
      
      movtopPongBar(switches);
      movbottomPongBar(switches);
      movLayerDraw(&ml0, &layer0);

      if (playerOneScore == ':' || playerTwoScore == ':'){
	//drawString5x7(7, 5, "Player 1 - ", COLOR_WHITE, COLOR_BLACK);
	//drawString5x7(screenWidth/2 -2, screenHeight-10, "- Player 2", COLOR_WHITE, COLOR_BLACK);
	//drawChar5x7(75, 5, playerOneScore, COLOR_WHITE, COLOR_BLACK);
	//drawChar5x7(screenWidth/2 - 10, screenHeight-10, playerTwoScore, COLOR_WHITE, COLOR_BLACK);
	//drawString5x7(60, 60, "Game Over", COLOR_WHITE, COLOR_BLACK);
	if (playerOneScore == ':'){
	  winner = "Player 1 Won!";
	  playerOneScore = 'W';
	} else {
	  winner = "Player 2 Won!";
	  playerTwoScore = 'W';
	}
	modeSelector = 2;
	clearScreen(0);
      }
      drawChar5x7(75, 5, playerOneScore, COLOR_WHITE, COLOR_BLACK);
      drawChar5x7(screenWidth/2 - 15, screenHeight-10, playerTwoScore, COLOR_WHITE, COLOR_BLACK);
    }
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
