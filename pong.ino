
/*
#######################################################################
# Pong                                                                #
# Author: Alex Dawson (alex-dawson-online.com)                        #
# A fairly basic version of pong with the facilities to use buzzers   #
# to enahnce user experience                                          #
# Pinout:                                                             #
# buzzer 1: digital 2                                                 #
# buzzer 2: digital 3                                                 #
# player 1: A0                                                        #
# player 2: A1                                                        #
# start button: digital 4                                             #
#                                                                     #
# TVout pinout (as far as I'm aware these cannot be changed):         #
# pins 7 & 9 to video                                                 #
# pin 11 to audio                                                     #
# for more information on TVout go to:                                #
# https://github.com/Avamander/arduino-tvout/                         #
#######################################################################
*/

#include <TVout.h>
#include <fontAll.h>

#define DEBUG;

const int p1 = A0;
const int p2 = A1;

int startButtonState;
bool playing = false;

int ballX = 0, ballY = 0;
int ballVelX = 0, ballVelY = 0;
const int BALL_SPEED = 1;

const int PADDLE_WIDTH = 1;
const int PADDLE_HEIGHT = 10;
const int p1X = 5;
const int p2X = 115;
int p1Y, p2Y;
int p1Score, p2Score;

TVout tv;

void setup() {
  Serial.begin(9600);
  DDRB = B00000010;
  DDRD = DDRD | B10001100;
  PORTD = PORTD & B00000011;
  playing = false;
  tv.begin(PAL, 126, 96); //128, 96
}

void loop() {
  if (!playing) {
    main_menu();
  } else {
    play_game();
  }
}

void process_inputs() {
    startButtonState = PORTD | B00001000;
    if (startButtonState == HIGH) { //Is button pressed
      if (!playing) {
        playing = true;
      }
    }

    int pot1 = analogRead(0);
    int pot2 = analogRead(1);
    p1Y = map(pot1, 0, 1023, 0 , tv.vres() - PADDLE_HEIGHT);
    p2Y = map(pot2, 0, 1023, 0, tv.vres() - PADDLE_HEIGHT);

    #ifdef DEBUG
      Serial.println(p1Y);
      Serial.println(p2Y);
    #endif
}

void main_menu() {
  #ifdef DEBUG
    Serial.println("main menu");
  #endif
  randomSeed(analogRead(2)); // set the seed using the value of an analog pin not in use
  int x = random(22, tv.hres() - 22);
  int y = random(8, tv.vres() - 8);
  int velX = 1;
  int velY = 1;

  for (int x = 0; x < tv.hres(); x++) {
    for (int y = 0; y < tv.vres(); y++) {
      if (x % 7 == 0 || y % 4 == 0) {
        continue;
      }
      if ((x < 22 || x > tv.hres() - 22) || (y < 8 || y > tv.vres() - 8)) {
        tv.set_pixel(x, y, 1);
      }
    }
  }

  tv.select_font(font8x8);
  //(tv.hres() / 2) - 16 is the offset for the middle of screen
  tv.set_cursor((tv.hres() / 2) - 16, tv.vres() / 3);
  tv.println("PONG");

  tv.select_font(font4x6);
  //(tv.hres() / 2) - 22 is the offset for middle of screen
  tv.set_cursor((tv.hres() / 2) - 22, tv.vres() / 2);
  tv.println("Press Start");

  int hitCount = 0;
  while (!playing) {
    process_inputs();
    tv.delay_frame(1);

    if (hitCount == 400) {
      break;
    }

    if (x + velX < 0 || x + velX > tv.hres() - 1) {
      velX = -velX;
    }
    if (y + velY < 0 || y + velY > tv.vres() - 1) {
      velY = -velY;
    }

    //if pixel in direction of travel is white, turn black
    if (tv.get_pixel(x + velX, y + velY) == 1) {
      tv.set_pixel(x + velX, y + velY, 0);
      tv.tone(2000, 30);
      //if pixel in opposite y value travel is black, go that way
      if (tv.get_pixel(x + velX, y - velY) == 0) {
        velY = -velY;
      } else if (tv.get_pixel(x - velX, y - velY) == 0) {//if pixel in opposite x direction of travel is black, go that way
        velX = -velX;
      } else {
        velX = -velX;
        velY = -velY;
      }
      PORTD = PORTD | B00001100; //Set both buzzers to HIGH
      hitCount++;
    } else {
      PORTD = PORTD & B00000011; // set both buzzers LOW (keep serial bits 0 and 1 the same)
    }

    tv.set_pixel(x, y, 0);
    x += velX;
    y += velY;
    tv.set_pixel(x, y, 1);
  }
  tv.clear_screen();
  #ifdef DEBUG
    Serial.println("leaving menu");
  #endif
}

void play_game() {
  #ifdef DEBUG
    Serial.println("playing game");
  #endif

  //set buzzers to LOW (maintaining serial bits)
  PORTD = PORTD & B00000011;

  ballVelX = 1;
  ballVelY = 1;
  ballX = tv.hres() / 2;
  ballY = tv.vres() / 2;

  p1Score = 0;
  p2Score = 0;

  tv.clear_screen();

  long buzzDurMillis = 50;
  long startMillis = 0;
  long currentMillis = 0;
  while (p1Score != 10 && p2Score != 10) {
    update_players();
    tv.delay_frame(1);
    //Draw centre line every (tv.vres() / 6) pixels
    for (int i = 0; i < tv.vres(); i += 6) {
      tv.draw_line(tv.hres() / 2, i, tv.hres() / 2, i + 2, 1);
    }

    //top / bottom hit check
    if (ballY <= 0 || ballY >= tv.vres()) {
      ballVelY = -ballVelY;
      tv.tone(500, 50);
    }

    //hit paddle check
    if (ballVelX < 0 && (ballX <= p1X)) {
      if (ballY > p1Y && ballY < p1Y + PADDLE_HEIGHT) {
        PORTD = PORTD | B00000100; //Set left buzzer HIGH
        startMillis = millis();
        currentMillis = millis();
        tv.tone(800, 30);
        ballVelX = -ballVelX;
        ballVelY += 2 * ((ballY - p1Y) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
      }
    }
    if (ballVelX > 0 && (ballX >= p2X)) {
      if (ballY > p2Y && ballY < p2Y + PADDLE_HEIGHT) {
        PORTD = PORTD | B00001000; //Set right buzzer HIGH
        startMillis = millis();
        currentMillis = millis();
        tv.tone(800, 30);
        ballVelX = -ballVelX;
        ballVolY += 2 * ((ballY - p2Y) - (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
        Serial.println(ballVelY);
      }
    }

    if (currentMillis - startMillis > buzzDurMillis) {
      PORTD = PORTD & B00000011; //set buzzers to low
      currentMillis = 0;
      startMillis = 0;
    } else {
      currentMillis = millis();
    }

    if (ballX + ballVelX < 3) { //player 2 scores
      tv.set_pixel(ballX, ballY, 0);
      p2Score++;
      PORTD = PORTD & B00000011;
      draw_scores();
      reset_play(2);
    }
    if (ballX + ballVelX > tv.hres() - 3) { //player 1 scores
      tv.set_pixel(ballX, ballY, 0);
      p1Score++;
      PORTD = PORTD & B00000011;
      draw_scores();
      reset_play(1);
    }

    //update ball coords
    tv.set_pixel(ballX, ballY, 0); //remove old ball
    ballX += ballVelX;
    ballY += ballVelY;
    //draw ball
    tv.set_pixel(ballX, ballY, 1);
  }
  show_win_screen();
}

void draw_scores() {
    tv.select_font(font4x6);
    tv.set_cursor(tv.hres() / 4, tv.vres() / 8); //p1 score coords
    tv.print(p1Score);
    tv.set_cursor((tv.hres() / 4) * 3, tv.vres() / 8); //p2 score coords
    tv.print(p2Score);
}

void show_win_screen() {
    playing  = false;
    tv.clear_screen();
    tv.select_font(font4x6);

    //check who won
    if (p1Score > p2Score) {
      tv.set_cursor((tv.hres() / 2) - 32, (tv.vres() / 2) - 4);
      tv.print("LEFT PLAYER WINS");
    } else if (p2Score > p1Score) {
      tv.set_cursor((tv.hres() / 2) - 34, (tv.vres() / 2) - 4);
      tv.print("RIGHT PLAYER WINS");
    } else {
      tv.print("DRAW");
    }
    delay(5000);
    tv.clear_screen();

}

//pass 0 for no one, 1 for p1, or 2 for p2
void reset_play(int winningPlayer) {
  ballX = tv.hres() / 2;
  randomSeed(analogRead(2));
  ballY = random(0, tv.vres());
  switch (winningPlayer) {
    case 0:
      ballVelX = 1;
      break;
    case 1:
      ballVelX = 1;
      break;
    case 2:
      ballVelX = -1;
      default:
        break;
  }
  unsigned long resetDelay = 2000;
  unsigned long startMillis = millis();
  unsigned long totalMillis = 0;
  unsigned long currentMillis = millis();
  while (totalMillis <= resetDelay) {
    update_players();
    tv.delay_frame(1);
    currentMillis = millis();
    totalMillis = currentMillis - startMillis;
  }

}

void update_players() {
  //Clear players
  for (int x = p1X; x < p1X + PADDLE_WIDTH; x++) {
    for (int y = p1Y; y < p1Y + PADDLE_HEIGHT; y++) {
      tv.set_pixel(x, y, 0);
    }
  }
  for (int x = p2X; x < p2X + PADDLE_WIDTH; x++) {
    for (int y = p2Y; y < p2Y + PADDLE_HEIGHT; y++) {
      tv.set_pixel(x, y, 0);
    }
  }

  //get new coords
  process_inputs();

  //draw new player positions
  for (int x = p1X; x < p1X + PADDLE_WIDTH; x++) {
    for (int y = p1Y; y < p1Y + PADDLE_HEIGHT; y++) {
      tv.set_pixel(x, y, 1);
    }
  }
  for (int x = p2X; x < p2X + PADDLE_WIDTH; x++) {
    for (int y = p2Y; y < p2Y + PADDLE_HEIGHT; y++) {
      tv.set_pixel(x, y, 1);
    }
  }

}
