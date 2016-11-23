/** \file lcddemo.c
 *  \brief A simple demo that draws a string and circle
 */

#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"

/** Initializes everything, clears the screen, draws "hello" and the circle */
main()
{
  configureClocks();
  lcd_init();
  u_char width = screenWidth, height = screenHeight;

  clearScreen(COLOR_BLACK);

  drawString5x7(10,20, "a b c d e f g h i", COLOR_WHITE, COLOR_BLACK);
  drawString5x7(10,40, "j k l m n o p q r", COLOR_WHITE, COLOR_BLACK);
  drawString5x7(10,60, "s t u v w x y z 0", COLOR_WHITE, COLOR_BLACK);
  drawString5x7(10,60, "1 2 3 4 5 6 7 8 9", COLOR_WHITE, COLOR_BLACK);

  // fillRectangle(30,30, 60, 60, COLOR_ORANGE);
  // fillLine(COLOR_ORANGE);
}
