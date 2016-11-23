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

// Playing Field Frame
AbRectOutline fieldOutline = {	
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 3, screenHeight/2 - 1}
};

// Frame Around Difficulty Settings On Main Menu
AbRectOutline selectorOutline = {
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {55, 8}
};

static int topPongBarXPosition = 0; // Controls Movement Of The Top Pong Bar
static int bottomPongBarXPosition = 0; // Controls Movement Of The Bottom Pong Bar
static int startingXSpeed = 2; // Controls The Pong Ball's Starting Speed On the X Axis
static int startingYSpeed = 1; // Controls The Pong Ball's Starting Speed On the Y Axis
static int velocityLimit = 0; // Controls How Much Can The Pong Ball Accelerate
static int incrementX = 0; // Controls Wether X-Axis Velocity Should Be Increased, Once Every 2 Y-Axis Increments
static int collisionBottomOccurred = 0; // Determines Wether A Collision Has Occurred On The Bottom Pong Bar
static int collisionTopOccured = 0; // Determines Wether A Collision Has Occurred On The Top Pong Bar
static int difficultyMode = 0; // Controls The Game's Difficulty Setting
static int modeSelector = 0; // To Determine Current Game State
static char playerOneScore; // Controls Player One's Score
static char playerTwoScore; // Controls Player Two's Score
static char* winner; // To Print Out A Winner When Game Ends
int switches; // To Read Input From Switches

// Layer Around Difficulty Settings On Main Menu
Layer selectorLayer = {
  (AbShape *) &selectorOutline,
  {65, 83},
  {0,0}, {0,0},				    
  COLOR_WHITE,
  0
};

// Playing Field Layer
Layer fieldLayer = {		
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},
  {0,0}, {0,0},				    
  COLOR_BLACK,
  0
};

// Top Pong Bar Layer
Layer topPongBar = {
  (AbShape *)&pongBar,
  {(screenWidth/2), 30},
  {0,0}, {0,0},
  COLOR_WHITE,
  &fieldLayer
};

// Bottom Pong Bar Layer
Layer bottomPongBar = {
(AbShape *)&pongBar,
  {(screenWidth/2), screenHeight-30},
  {0,0}, {0,0},
  COLOR_WHITE,
  &topPongBar
};

// Pong Ball Layer
Layer pongBall = {		
  (AbShape *)&circle7,
  {(screenWidth/2)+10, (screenHeight/2)-10}, 
  {0,0}, {0,0},				    
  COLOR_WHITE,
  &bottomPongBar,
};

// Moving Layer For The Top Pong Bar
MovLayer mtopPongBar = {&topPongBar, {1,1}, 0};
// Moving Layer For The Bottom Pong Bar
MovLayer mbottomPongBar = {&bottomPongBar, {1,1}, 0};
// Moving Layer For The Pong Ball
MovLayer mpongBall = {&pongBall, {2,1}, 0}; 

// Draws A Moving Layer On The LCD Panel
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

// Increments The Ball's Velocity
void incrementBallVelocity(MovLayer *pongBall){
  // Only Increment A Set Amount Of Times, Determined By The Difficulty Mode
  if (velocityLimit < (8 + difficultyMode)){
    // Only Increment The X-Axis Velocity Once The Y-Axis Velocity Has Been Incremented Twice
    if ( incrementX == 2) {
      if (pongBall->velocity.axes[0] < 0){
        pongBall->velocity.axes[0]--;
      } else {
        pongBall->velocity.axes[0]++;
      }
      incrementX = 0;
    } else {
      if (pongBall->velocity.axes[1] < 0){
        pongBall->velocity.axes[1]--;
      } else {
        pongBall->velocity.axes[1]++;
      }
      incrementX++;
    }
    velocityLimit++;
  }
}

// Resets The Ball's Velocity To It's Starting Values
void resetBallVelocity(MovLayer *pongBall){
  velocityLimit = 0; // Restores The Velocity Limit, Allowing The Ball To Accelerate Again
  pongBall->velocity.axes[0] = startingXSpeed;
  pongBall->velocity.axes[1] = startingYSpeed;
}

// Returns The Pong Bars To It's Starting Positions
void resetPongBars(){
  mtopPongBar.layer->posNext.axes[0] = screenWidth/2;
  mbottomPongBar.layer->posNext.axes[0] = screenWidth/2;
  movLayerDraw(&mtopPongBar, &topPongBar);
  movLayerDraw(&mbottomPongBar, &bottomPongBar);
}

// Moves The Top Pong Bar
void movtopPongBar(int switches){
  // Moves The Bar According To It's Current Position And How Much Its Set To Move
  mtopPongBar.layer->posNext.axes[0] = mtopPongBar.layer->pos.axes[0] + topPongBarXPosition; 
  // If The 1st Button Is Pressed Move Bar To The Left
  if (!(BIT0 & switches)){
    // Only Move Provided The Move Won't Send The Bar Past The Fence
    if (mtopPongBar.layer->posNext.axes[0] >= 31){
      topPongBarXPosition = -13;
    } else {
      topPongBarXPosition = 0;
    }
  } 
  // If The 2nd Button Is Pressed Move Bar To The Right
  else if (!(BIT1 & switches)){
    // Only Move Provided The Move Won't Send The Bar Past The Fence
    if (mtopPongBar.layer->posNext.axes[0] <= screenWidth - 31){
      topPongBarXPosition = 13;
    } else {
      topPongBarXPosition = 0;
    }
  }
  // If No Button Is Pressed Don't Move Bar
  else {
    topPongBarXPosition = 0;
  }
  // Redraw The Bar To Reflect Change
  movLayerDraw(&mtopPongBar, &topPongBar);
}

// Moves The Bottom Pong Bar
void movbottomPongBar(int switches){
  // Moves The Bar According To It's Current Position And How Much Its Set To Move
  mbottomPongBar.layer->posNext.axes[0] = mbottomPongBar.layer->pos.axes[0] + bottomPongBarXPosition;
  // If The 3rd Button Is Pressed Move Bar To The Left
  if (!(BIT2 &switches)){
    // Only Move Provided The Move Won't Send The Bar Past The Fence
    if (mbottomPongBar.layer->posNext.axes[0] >= 31){
      bottomPongBarXPosition = -13;
    } else {
      bottomPongBarXPosition = 0;
    }
  } 
  // If The 4th Button Is Pressed Move Bar To The Right
  else if (!(BIT3 & switches)){
    // Only Move Provided The Move Won't Send The Bar Past The Fence
    if (mbottomPongBar.layer->posNext.axes[0] <= screenWidth - 31){
      bottomPongBarXPosition = 13;
    }  else {
      bottomPongBarXPosition = 0;
    }
  }
  // If No Button Is Pressed Don't Move Bar  
  else {
    bottomPongBarXPosition = 0;
  }
  // Redraw The Bar To Reflect Change
  movLayerDraw(&mbottomPongBar, &bottomPongBar);
}

// Handles Fence Collisions, Resets The Pong Ball and Bars
void handleCollisionOnFence(Vec2* newPos, MovLayer* ml){
  newPos->axes[0] = (screenWidth/2)+10; // Reset The Pong Ball's X-Position
  newPos->axes[1] = (screenHeight/2)-10; // Reset The Pong Ball's Y-Position
  resetBallVelocity(ml);
  resetPongBars();
  collisionBottomOccurred = 0;
  collisionTopOccured = 0;
  playCollisionSoundOnFence(); // In buzzerFunctions Assembly File
}

// Detects If A Collision Ocurred On The Bottom Section Of The Fence
int detectCollisionBottomFence(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos; // Retrieves The Fence's Center Position
  // Traverses All The Pixels On The Bottom Section Of The Fence 
  for (i = -100; i < 100; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] += 50;
    int collision = abRectCheck(rect, &centerPos1, pixel); // Checks If The Ball Center Position And Fence Collided
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] -= 50;
  }
  return 0;
}

// Detects If A Collision Ocurred On The Top Section Of The Fence
int detectCollisionTopFence(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  Vec2 centerPos1 = *centerPos; // Retrieves The Fence's Center Position
  // Traverses All The Pixels On The Top Section Of The Fence 
  for (i = -100; i < 100; i++){
    centerPos1.axes[0] += i;
    centerPos1.axes[1] -= 50;
    int collision = abRectCheck(rect, &centerPos1, pixel); // Checks If The Ball Center Position And Fence Collided
    if (collision){
      return 1;
    }
    centerPos1.axes[0] -= i;
    centerPos1.axes[1] += 50;
  }
  return 0;
}

// Detects If A Collision Ocurred On The Top Pong Bar
int detectCollisionTopPongBar(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i;
  int j;
  Vec2 centerPos1 = *centerPos; // Retrieves The Top Pong Bar's Center Position
  // Traverses A Small Area On The Bottom Edge Of The Top Pong Bar
  if (!collisionTopOccured){
    for (j = 8; j < 12; j++){
      for (i = -7; i < 7; i++){
        centerPos1.axes[0] += i;
        centerPos1.axes[1] += j;
        int collision = abRectCheck(rect, &centerPos1, pixel); // Checks If The Ball Center Position And The Top Pong Bar Collided
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

// Detects If A Collision Ocurred On The Bottom Pong Bar
int detectCollisionBottomPongBar(const AbRect* rect, const Vec2* centerPos, const Vec2 *pixel){
  int i = 0;
  int j = 10;
  Vec2 centerPos1 = *centerPos; // Retrieves The The Bottom Pong Bar's Center Position
  // Traverses A Small Area On The Top Edge Of The Bottom Pong Bar
  if (!collisionBottomOccurred){
    for (j = 8; j < 12;j++){
      for (i = -7; i < 7; i++){
        centerPos1.axes[0] += i;
        centerPos1.axes[1] -= j;
        int collision = abRectCheck(rect, &centerPos1, pixel); // Checks If The Ball Center Position And And The Bottom Pong Bar Collided
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

/** Advances The Ball Within The Fence, Probing For Collision With The Fence Or Pong Bars.
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence){
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
        // If The Ball Hits The Top Part Of The Fence
        if (detectCollisionTopFence(&fieldOutline, &(fieldLayer.pos), &(ml->layer->pos)) && axis == 1){
          handleCollisionOnFence(&newPos, ml);
          playerTwoScore++;
        } 
        // If The Ball Hits The Bottom Part Of The Fence
        else if (detectCollisionBottomFence(&fieldOutline, &(fieldLayer.pos), &(ml->layer->pos)) && axis == 1){
          handleCollisionOnFence(&newPos, ml);
          playerOneScore++;
        } 
        else { 
          int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
          newPos.axes[axis] += (2*velocity); // Inverts Ball Direction
        }
      }
      // If the ball hits the top pong bar, only checking on the Y-Axis 
      if (detectCollisionTopPongBar(&pongBar, &(topPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
        newPos.axes[axis] += (2*velocity); // Inverts Ball Direction
        incrementBallVelocity(ml);
        collisionBottomOccurred = 0;
        collisionTopOccured = 1;
        playCollisionSoundOnBar(); // In buzzerFunctions Assembly File
      }
      // If the ball hits the bottom pong bar, only checking on the Y-Axis
      if (detectCollisionBottomPongBar(&pongBar, &(bottomPongBar.pos), &(ml->layer->pos)) && axis == 1){
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
        newPos.axes[axis] += (2*velocity); // Inverts Ball Direction
        incrementBallVelocity(ml); 
        collisionBottomOccurred = 1;
        collisionTopOccured = 0;
        playCollisionSoundOnBar(); // In buzzerFunctions Assembly File
      }
    }
    // Sets The Ball's Next Position
    ml->layer->posNext = newPos;
  }
}
 
void selectMode(){
  // Menu Screen, Lets User Set Difficulty Mode And Start Game 
  if (modeSelector == 0){
    drawString5x7(15, 40, "    PONG", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 60, "BTN1 - Start", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 80, "BTN2 - Set Easy", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 100, "BTN3 - Set Medium", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 120, "BTN4 - Set Hard", COLOR_WHITE, COLOR_BLACK);
    // Starts The Game
    if (!(BIT0 & switches)) { 
      clearScreen(0);
      _delay(50);
      playerTwoScore = '0';
      playerOneScore = '0';
      modeSelector = 1;
      mpongBall.velocity.axes[0] = startingXSpeed;
      mpongBall.velocity.axes[1] = startingYSpeed;
    }
    // Sets Easy Difficulty Mode
    else if (!(BIT1 & switches)){
      selectorLayer.pos.axes[1] = 83;
      layerDraw(&selectorLayer);
      startingXSpeed = 2;
      startingYSpeed = 1;
      difficultyMode = -2;
    }
    // Sets Medium Difficulty Mode
    else if (!(BIT2 & switches)){
      selectorLayer.pos.axes[1] = 103;
      layerDraw(&selectorLayer);
      startingXSpeed = 3;
      startingYSpeed = 2;
      difficultyMode = 2;
    }
    // Sets Hard Difficulty Mode
    else if (!(BIT3 & switches)){
      selectorLayer.pos.axes[1] = 123;
      layerDraw(&selectorLayer);
      startingXSpeed = 4;
      startingYSpeed = 3;
      difficultyMode = 6;
    }
  }
  // Game Over Screen, Displays Winner And Lets User Restart The Game
  if (modeSelector == 2){
    drawString5x7(7, 5, "Player 1 - ", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(screenWidth/2 - 2, screenHeight-10, "- Player 2", COLOR_WHITE, COLOR_BLACK);
    drawChar5x7(75, 5, playerOneScore, COLOR_WHITE, COLOR_BLACK);
    drawChar5x7(screenWidth/2 - 15, screenHeight-10, playerTwoScore, COLOR_WHITE, COLOR_BLACK);
    drawString5x7(10, 60, "  Game Over", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 80, winner, COLOR_WHITE, COLOR_BLACK);
    drawString5x7(15, 100, " BTN1 - Menu", COLOR_WHITE, COLOR_BLACK);
    // Restart Game, Going Back To Menu 
    if (!(BIT0 & switches)) {
      clearScreen(0);
      _delay(50);
      selectorLayer.pos.axes[1] = 83;
      startingXSpeed = 2;
      startingYSpeed = 1;
      difficultyMode = 0;
      modeSelector = 0;
      layerDraw(&selectorLayer);
    } 
  }
}

// Initializes The Buzzer 
void initializeBuzzer(){
  timerAUpmode();
  P2SEL &= ~(BIT6 | BIT7);
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;
}

u_int bgColor = COLOR_BLACK; // The background color 
int redrawScreen = 1; // Boolean for whether screen needs to be redrawn 
Region fieldFence; // Fence Around Playing Field 

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;
  
  // Initializes Everything
  configureClocks();
  lcd_init();
  p2sw_init(BIT0 + BIT1 + BIT2 + BIT3);
  initializeBuzzer();
  clearScreen(0);
  shapeInit();
  layerInit(&pongBall);
  layerDraw(&selectorLayer);
  layerGetBounds(&fieldLayer, &fieldFence); 
   
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) {
    // To Read Input From Switches
    switches = p2sw_read();
    // Game Only Starts When modeSelector Is 1, Otherwise A Menu Is Being Displayed 
    if (modeSelector != 1){
      selectMode();
    } 
    else {
      while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
        P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
        or_sr(0x10);	      /**< CPU OFF */
      }
      // Turns On The Green LED When CPU Is On
      P1OUT |= GREEN_LED;      
      redrawScreen = 0;
      // Draws The Player's Name In A Corresponding Corner
      drawString5x7(7, 5, "Player 1 - ", COLOR_WHITE, COLOR_BLACK);
      drawString5x7(screenWidth/2 -2, screenHeight-10, "- Player 2", COLOR_WHITE, COLOR_BLACK);
      // Checks Wether The Pong Bars Should Be Moved
      movtopPongBar(switches);
      movbottomPongBar(switches);
      // Moves The Ball
      movLayerDraw(&mpongBall, &pongBall);
      // Checks If Either Player Has Scored 10 Times And Terminates The Game
      if (playerOneScore == ':' || playerTwoScore == ':'){
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
      // Draws The Player's Current Score
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
  if (count == 14) {
    // To Keep Game From Starting During Menus
    if (modeSelector == 1){
      mlAdvance(&mpongBall, &fieldFence);
    }
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}