# Arch1 Lab 3 LCD Game - Pong

This directory contains:

* A Program That Implements the Pong videogame on the MSP430 microcontroller. 

This project makes use of the MSP430's LCD Display and Buzzer to make Pong. The players are 
able to select 3 difficulty levels (Easy, Medium, Hard) and play using the MSP430's 4 buttons.
Pong consists of two players controlling a paddle (One Per Player) and bounce a ball between 
them until a player misses the ball and the opposing player is awarded a point. The first player
to score 5 times wins the game. The victorious player is displayed and the players can play again.

The Program Works In the Following Way:

  After turning on the MSP430 the main menu is displayed, which contains the name of the game and 
  the prompts the player to push the button to carry out one of the following actions:
  
  * BTN1 - Start Game
  * BTN2 - Set Easy Mode
  * BTN3 - Set Medium Mode
  * BTN4 - Set Hard Mode
  
  The game's current difficulty mode is outlined withing the menu and changes accordingly. Once the game 
  starts the player are able to move the bars the following way:
  
  * BTN1 - Move Top Bar To The Left
  * BTN2 - Move Top Bar To The Right
  * BTN3 - Move Bottom Bar To The Left
  * BTN4 - Move Bottom Bar To The Right
  
  The player controlling the top bar is player 1 and the one controlling the bottom one is player 2. This data
  is displayed on the screen, as well as each player's current score. The score for both players is always zero
  and caps at 5. The game's difficulty settings determine the ball's starting velocity and how much it can accelerate.
  Easy mode allows 6 speed boosts, Medium allows 10, and Hard allows 12. Once a player achives a score of 5 the game 
  ends and the game over screen is displayed. Easy is the default difficulty. This screen prints the victorious players,
  puts a W (Winner) on the winner's score and prompts the user to press BTN1 to go back to the main menu. Back at the
  main menu, difficulty resets to easy and players can play again.  
  
This program contains the following files:

* Pong.c - Initializes LCD, Buzzer, Clock, and Starts Game 
* buzzerFunctions.s - Plays Collision Sound And Resets Sound
* wdt_handler.s 
* Makefile

To compile:
~~~
$ make
~~~

To load the game:
~~~
$ make load
~~~

To delete binaries:
~~~
$ make clean
~~~

Acknowledgements

	* The game was based of the shape-motion-demo provided in class, and uses most libraries provided in class as well. 