/* JAKintosh Game Console

Pilot Chase and Mastermind by Kevin Wang
Color Switch by Josh Kreitz
Dynamic memory allocation by Akshay Rao
*/

#include <gamma.h>
#include <RGBmatrixPanel.h>
#include <Adafruit_GFX.h>

#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr

// define the wiring of the LED screen
const uint8_t CLK  = 8;
const uint8_t LAT = A3;
const uint8_t OE = 9;
const uint8_t A = A0;
const uint8_t B = A1;
const uint8_t C = A2;

// define the wiring of the inputs
const int POTENTIOMETER_PIN_NUMBER = A5;
const int BUTTON_PIN_NUMBER = 10;

// a global variable that represents the LED screen
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

const int CURSOR_Y = 1;
const int CHAR_WIDTH = 12;

// ======== GLOBAL VARIABLES/FUNCTIONS/CLASSES ========

// Game state
bool inMenu = true;
bool inPilotChase = false;
bool inColorSwitch = false;
bool inMastermind = false;
bool exitPilotChase = false;
bool exitColorSwitch = false;
bool exitMastermind = false;
int gameNum = 0;
int currentVal = 0;

// Input
int potentiometer_value = analogRead(POTENTIOMETER_PIN_NUMBER);
bool button_pressed = (digitalRead(BUTTON_PIN_NUMBER) == HIGH);

// Color
class Color {
  public:
    int red;
    int green;
    int blue;

    Color() {
      red = 0;
      green = 0;
      blue = 0;
    }
    Color(int r, int g, int b) {
      red = r;
      green = g;
      blue = b;
    }
    uint16_t to_333() const {
      return matrix.Color333(red, green, blue);
    }
    int get_red() {
      return red;
    }
    int get_green() {
      return green;
    }
    int get_blue() {
      return blue;
    }

    bool equals(Color c) {
      return (red == c.get_red() && green == c.get_green() && blue == c.get_blue());
    }
};

const Color BLACK(0, 0, 0);
const Color RED(4, 0, 0);
const Color ORANGE(4, 1, 0);
const Color YELLOW(4, 4, 0);
const Color GREEN(0, 4, 0);
const Color BLUE(0, 0, 4);
const Color PURPLE(4, 0, 4);
const Color WHITE(4, 4, 4);
const Color LIME(1, 4, 0);
const Color AQUA(0, 4, 4);

// ask player to keep playing, else return to menu
bool keepPlaying() {
  matrix.fillScreen(0);
  int hue = 0;

  matrix.setTextColor( BLUE.to_333() );
  matrix.setCursor(1, 1);
  matrix.print("CONT?");
  delay(1000);

  while ( inPilotChase || inColorSwitch || inMastermind ) {
    potentiometer_value = analogRead(POTENTIOMETER_PIN_NUMBER);
    button_pressed = (digitalRead(BUTTON_PIN_NUMBER) == HIGH);

    if ( potentiometer_value <= (1023 / 2) ) {
      matrix.setCursor(1, 9);
      matrix.setTextColor( GREEN.to_333() );
      matrix.print("YES");
      matrix.setTextColor( RED.to_333() );
      matrix.print("NO");

      if ( button_pressed ) {
        button_pressed = false;
        matrix.fillScreen(0);
        return true;
      }
    }
    else {
      matrix.setCursor(1, 9);
      matrix.setTextColor( RED.to_333() );
      matrix.print("YES");
      matrix.setTextColor( GREEN.to_333() );
      matrix.print("NO");

      if ( button_pressed ) {
        button_pressed = false;
        matrix.fillScreen(0);
        return false;
      }
    }

  }
}

// scroll text


void scroll( char str[], int len) {

  for (int i = -32; i < len * CHAR_WIDTH; i++) {
    matrix.fillScreen(0);
    for (int j = 0; j < len; j++) {
      matrix.setTextSize(2);
      matrix.setCursor(1 - i + (CHAR_WIDTH * j), CURSOR_Y);
      matrix.print(str[j]);
    }
    delay(30);
  }
  matrix.fillScreen(0);
  matrix.setTextSize(1);
}

// ======== MASTERMIND ========

class Cell {
  public:
    Cell() {
      x = 1;
      y = 1;
      color = BLACK;
    }
    Cell(int x_arg, int y_arg, Color c_arg) {
      x = x_arg;
      y = y_arg;
      color = c_arg;
    }
    void set_x(int x_arg) {
      x = x_arg;
    }
    void set_y(int y_arg) {
      y = y_arg;
    }
    int get_x() {
      return x;
    }
    int get_y() {
      return y;
    }
    void draw(Color c) {
      draw_with_rgb(c);
    }
  private:
    int x;
    int y;
    Color color;
    void draw_with_rgb(Color color) {
      matrix.drawPixel(x, y, WHITE.to_333());
      matrix.drawPixel(x + 1, y, color.to_333());
      matrix.drawPixel(x, y + 1, color.to_333());
      matrix.drawPixel(x + 1, y + 1, color.to_333());
    }
};

class Grader {
  public:
    Grader() {
      x = 0;
      y = 0;
    }
    Grader(int x_arg, int y_arg) {
      x = x_arg;
      y = y_arg;
    }
    void draw(Color c[4]) {
      draw_with_rgb(c);
    }
  private:
    int x;
    int y;
    void draw_with_rgb(Color c[4]) {
      matrix.drawPixel(x, y, c[0].to_333());
      matrix.drawPixel(x + 1, y, c[1].to_333());
      matrix.drawPixel(x, y + 1, c[2].to_333());
      matrix.drawPixel(x + 1, y + 1, c[3].to_333());
    }
};

class Mastermind {
  public:
    void shuffle(int arr[4]) {
      for (int i = 0; i < 4; i++) {
        arr[i] = random(0, 6);
      }
    }

    bool isIn(int val, int arr[4]) {
      for (int i = 0; i < 4; i++) {
        if (arr[i] == val) {
          return true;
        }
      }
      return false;
    }

    void victory() {
      delay(1000);
      matrix.setTextColor( RED.to_333() );
      scroll("YOU WIN", 7);
      matrix.fillScreen(0);
      delay(100);
      inMastermind = false;
      exitMastermind = true;
      inMenu = true;
      return;
    }

    void grade(int guess[4], int solution[4], Color out[4]) {

      int writeIndex = 0;
      int no_i[4] = { -1, -1, -1, -1};
      int no_j[4] = { -1, -1, -1, -1};

      //Check for correct order
      for (int i = 0; i < 4; i++) {
        out[i] = BLACK;
        if (guess[i] == solution[i]) {
          no_i[writeIndex] = i;
          no_j[writeIndex] = i;
          out[writeIndex] = RED;
          writeIndex++;
          if (writeIndex == 3) {
            victory();
            return;
          }
        }
      }
      //Check for incorrect order
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
          if (guess[i] == solution[j] && !isIn(i, no_i) && !isIn(j, no_j)) {
            no_j[writeIndex] = j;
            out[writeIndex] = WHITE;
            writeIndex++;
            break;
          }
        }
      }


      Serial.print("GUESS: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(guess[i]);
      }

      Serial.println("");

    }

    void setup() {
      matrix.fillScreen(0);
      randomSeed(analogRead(0));
      shuffle(solution);

      Serial.print("SOLUTION: ");
      for (int i = 0; i < 4; i++) {
        Serial.print(solution[i]);
      }
      Serial.println("");

      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
          graders[(i * 5) + j] = Grader(13 + (16 * i), 1 + (3 * j));
        }
      }

    }

    void loop(int potentiometer_value, bool button_pressed) {
      // put your main code here, to run repeatedly:
      switch (potentiometer_value / 170) {
        case 0:
          paintbrush.draw(RED);
          break;
        case 1:
          paintbrush.draw(PURPLE);
          break;
        case 2:
          paintbrush.draw(BLUE);
          break;
        case 3:
          paintbrush.draw(AQUA);
          break;
        case 4:
          paintbrush.draw(GREEN);
          break;
        case 5:
          paintbrush.draw(YELLOW);
          break;
      }

      if (button_pressed) {
        if (initial) {
          guess[currentCell % 4] = potentiometer_value / 170;
          currentCell += 1;
          if (currentCell % 4 == 0) {
            grade(guess, solution, out);
            graders[(currentCell / 4) - 1].draw(out);
            if (currentCell == 40) {
              matrix.setTextColor( RED.to_333() );
              scroll("GAME OVER", 9);
              matrix.fillScreen(0);
              delay(100);
              inMastermind = false;
              exitMastermind = true;
              inMenu = true;
              return;
            }
            paintbrush.set_y(paintbrush.get_y() + 3);
            if (currentCell == 20) {
              paintbrush.set_y(1);
            }
            if (currentCell >= 20) {
              paintbrush.set_x(17);
            } else {
              paintbrush.set_x(1);
            }
          } else {
            paintbrush.set_x(paintbrush.get_x() + 3);
          }
          initial = false;
        }
      } else {
        initial = true;
      }

      delay(100);

    }

  private:
    int guess[4] = {};
    int solution[4] = {};
    Color out[4] = {BLACK, BLACK, BLACK, BLACK};
    bool initial = true;
    int currentCell = 0;
    int x_ind = 1;
    int y_ind = 1;
    //Cell cells[40];
    Cell paintbrush;
    Grader graders[10];

};

// ======== COLOR SWITCH ========

//ints representing the different types of obstacles
const int SQUARE = 0;
const int DIAMOND = 1;
const int BAR = 2;
const int ALTER = 3;

//the number of preloaded obstacles that exist in the game at any one point
const int NUM_OBSTACLES = 3;
//the total number of obstacles, including alters
const int NUM_QUEUE = 5;
//the delay between button clicks to smooth things out
const int BTN_CLICK_DELAY = 250;

//class to handle all of the obstacles in the game
class Obstacle {
  public:
    Obstacle() {
      x = type = -1;
      set = false;
    }

    Obstacle(int x_arg, int type_arg) {
      x = x_arg;
      type = type_arg;
      set = true;
      x0 = x;
      switch (type) {
        case SQUARE:
          y0 = 1;
          side = 1;
          break;
        case BAR:
          y0 = 0;
          break;
        case DIAMOND:
          y0 = 7;
          side = 1;
          break;
        case ALTER:
          y0 = 7;
          side = 1;
          break;
      }
    }

    //returns the type of the obstacle
    int get_type() {
      return type;
    }

    int get_x() {
      return x;
    }

    //returns whether or not the obstacle has been set in the game
    bool is_set() {
      return set;
    }

    //sets whether or not the obstacle has been set
    void set_set(bool set_arg) {
      set = set_arg;
    }

    //moves the obstacle down one row of pixels
    void dec_x(Color playerCol) {
      erase();
      x--;
      x0--;
      draw(-2147483647, playerCol);
    }

    //updates the lead pixel's values based on the type of obstacle
    //see private variable definitions for more explination
    void update() {
      switch (type) {
        case SQUARE:
          {
            if (side == 1)
              x0--;
            else if (side == 2)
              y0++;
            else if (side == 3)
              x0++;
            else if (side = 4)
              y0--;

            if (x0 == x && y0 == 1)
              side = 1;
            else if (x0 == x - 13 && y0 == 1)
              side = 2;
            else if (x0 == x - 13 && y0 == 14)
              side = 3;
            else if (x0 == x && y0 == 14)
              side = 4;

            break;
          }
        case BAR:
          {
            y0--;

            if (y0 < 0)
              y0 = 31;

            break;
          }
        case DIAMOND:
          {
            if (side == 1) {
              x0--;
              y0--;
            }
            else if (side == 2) {
              x0--;
              y0++;
            }
            else if (side == 3) {
              x0++;
              y0++;
            }
            else if (side == 4) {
              x0++;
              y0--;
            }

            if (y0 == 0) {
              y0++;
              side = 2;
            }
            else if (x0 == x - 14 && y0 == 8) {
              x0++;
              side = 3;
            }
            else if (y0 == 15) {
              y0--;
              side = 4;
            }
            else if (x0 == x + 1 && y0 == 7) {
              x0--;
              side = 1;
            }

            break;
          }
        case ALTER:
          {
            if (side == 1) {
              x0--;
              y0--;
            }
            else if (side == 2) {
              x0--;
              y0++;
            }
            else if (side == 3) {
              x0++;
              y0++;
            }
            else if (side == 4) {
              x0++;
              y0--;
            }

            if (y0 == 5) {
              y0++;
              side = 2;
            }
            else if (x0 == x - 4 && y0 == 8) {
              x0++;
              side = 3;
            }
            else if (y0 == 10) {
              y0--;
              side = 4;
            }
            else if (x0 == x + 1 && y0 == 7) {
              x0--;
              side = 1;
            }

            break;
          }
      }
    }

    //draws each obstacle
    bool draw(int playerX, Color playerCol) {
      if (!set)
        return true;
      switch (type) {
        case SQUARE:
          {
            int count = 0;
            Color currentCol = RED;
            for (int tempX = x0, tempY = y0, tempSide = side; true;) {
              matrix.drawPixel(tempX, tempY, currentCol.to_333());

              count++;
              if (count % 13 == 0)
                if (currentCol.equals(RED))
                  currentCol = PURPLE;
                else if (currentCol.equals(PURPLE))
                  currentCol = YELLOW;
                else if (currentCol.equals(YELLOW))
                  currentCol = AQUA;
                else break;

              if (tempSide == 1)
                tempX--;
              else if (tempSide == 2)
                tempY++;
              else if (tempSide == 3)
                tempX++;
              else if (tempSide = 4)
                tempY--;

              if (tempX == x && tempY == 1)
                tempSide = 1;
              else if (tempX == x - 13 && tempY == 1)
                tempSide = 2;
              else if (tempX == x - 13 && tempY == 14)
                tempSide = 3;
              else if (tempX == x && tempY == 14)
                tempSide = 4;

              if ((tempX >= playerX && tempX <= playerX + 3) && (tempY >= 6 && tempY <= 9) && !currentCol.equals(playerCol))
                return false;
            }
            return true;
          }
        case BAR:
          {
            int count = 0;
            Color currentCol = RED;
            for (int tempY = y0; true;) {
              matrix.drawPixel(x0, tempY, currentCol.to_333());

              count++;
              if (count % 8 == 0)
                if (currentCol.equals(RED))
                  currentCol = PURPLE;
                else if (currentCol.equals(PURPLE))
                  currentCol = YELLOW;
                else if (currentCol.equals(YELLOW))
                  currentCol = AQUA;
                else break;

              tempY--;
              if (tempY < 0)
                tempY = 31;

              if ((x >= playerX && x <= playerX + 3) && (tempY >= 6 && tempY <= 9) && !currentCol.equals(playerCol))
                return false;
            }
            return true;
          }
        case DIAMOND:
          {
            int count = 0;
            Color currentCol = playerCol;

            Color otherCol;
            if (playerCol.equals(RED))
              otherCol = AQUA;
            else if (playerCol.equals(AQUA))
              otherCol = RED;
            else if (playerCol.equals(PURPLE))
              otherCol = YELLOW;
            else if (playerCol.equals(YELLOW))
              otherCol = PURPLE;
            else otherCol = GREEN;

            for (int tempX = x0, tempY = y0, tempSide = side; true;) {
              matrix.drawPixel(tempX, tempY, currentCol.to_333());

              count++;
              if (count % 14 == 0)
                if (currentCol.equals(playerCol))
                  currentCol = otherCol;
                else break;

              if (tempSide == 1) {
                tempX--;
                tempY--;
              }
              else if (tempSide == 2) {
                tempX--;
                tempY++;
              }
              else if (tempSide == 3) {
                tempX++;
                tempY++;
              }
              else if (tempSide == 4) {
                tempX++;
                tempY--;
              }

              if (tempY == 0) {
                tempY++;
                tempSide = 2;
              }
              else if (tempX == x - 14 && tempY == 8) {
                tempX++;
                tempSide = 3;
              }
              else if (tempY == 15) {
                tempY--;
                tempSide = 4;
              }
              else if (tempX == x + 1 && tempY == 7) {
                tempX--;
                tempSide = 1;
              }

              if ((tempX >= playerX && tempX <= playerX + 3) && (tempY >= 6 && tempY <= 9) && !currentCol.equals(playerCol))
                return false;
            }
            return true;
          }
        case ALTER:
          {
            matrix.fillRect(x - 2, 7, 2, 2, WHITE.to_333());

            int count = 0;
            Color currentCol = RED;
            for (int tempX = x0, tempY = y0, tempSide = side; true;) {
              matrix.drawPixel(tempX, tempY, currentCol.to_333());

              count++;
              if (count % 2 == 0)
                if (currentCol.equals(RED))
                  currentCol = PURPLE;
                else if (currentCol.equals(PURPLE))
                  currentCol = YELLOW;
                else if (currentCol.equals(YELLOW))
                  currentCol = AQUA;
                else break;

              if (tempSide == 1) {
                tempX--;
                tempY--;
              }
              else if (tempSide == 2) {
                tempX--;
                tempY++;
              }
              else if (tempSide == 3) {
                tempX++;
                tempY++;
              }
              else if (tempSide == 4) {
                tempX++;
                tempY--;
              }

              if (tempY == 5) {
                tempY++;
                tempSide = 2;
              }
              else if (tempX == x - 4 && tempY == 8) {
                tempX++;
                tempSide = 3;
              }
              else if (tempY == 10) {
                tempY--;
                tempSide = 4;
              }
              else if (tempX == x + 1 && tempY == 7) {
                tempX--;
                tempSide = 1;
              }

              if ((tempX >= playerX && tempX <= playerX + 3) && (tempY >= 6 && tempY <= 9) && !currentCol.equals(playerCol))
                return false;
            }
            return true;
          }
      }
    }

    //erases the obstacle, making sure not to erase where the player might be, which would result in flashing
    void erase() {
      switch (type) {
        case SQUARE:
          matrix.drawRect(x - 13, 1, 14, 14, 0);
          break;
        case BAR:
          matrix.drawLine(x, 0, x, 15, 0);
          break;
        case DIAMOND:
          matrix.drawLine(x, 7, x - 6, 1, 0);
          matrix.drawLine(x - 7, 1, x - 13, 7, 0);
          matrix.drawLine(x - 13, 8, x - 7, 14, 0);
          matrix.drawLine(x - 6, 14, x, 8, 0);
          break;
        case ALTER:
          matrix.fillRect(x - 3, 6, 4, 4, 0);
          break;
      }
    }

  private:
    //the type of the obstacle
    int type;
    //the x topmost x value of the obstacle
    int x;
    //determines if the obstacle is set in the level yet
    bool set = false;

    //the coordinates of the "lead pixel" -- this is how I allowed the colors to rotate.
    //I track the movement of this one pixel, then draw all the colors behind this pixel,
    //so when this pixel moves, all the colors shift as well
    int x0;
    int y0;
    //a variable used to aid calculations in the lead pixel's movement
    int side;
};

//handles player location and movement
class ColorSwitchPlayer {
  public:
    ColorSwitchPlayer() {
      x = 2;
      y = 6;
      randColor();
    }

    //sets the player's color to a random color
    void randColor() {
      Color oldCol = color;
      do {
        switch (random(0, 4)) {
          case 0:
            color = RED;
            break;
          case 1:
            color = PURPLE;
            break;
          case 2:
            color = YELLOW;
            break;
          case 3:
            color = AQUA;
            break;
        }
      } while (oldCol.equals(color));
    }

    // Getters
    int get_x() const {
      return x;
    }

    int get_y() const {
      return y;
    }

    int get_gravity() const {
      return gravity;
    }

    Color get_color() {
      return color;
    }

    // Setters

    void set_x(int x_arg) {
      erase();
      x = x_arg;
      if (x < MIN_X)
        x = MIN_X;
      draw();
    }

    void set_y(int y_arg) {
      erase();
      y = y_arg;
      draw();
    }

    void set_color(Color col) {
      color = col;
    }

    void set_gravity(int g_arg) {
      gravity = g_arg;
    }

    void incGravity() {
      if (gravity < 2) {
        gravity += 1;
      }
    }

    // Animation

    void draw() {
      draw_with_rgb(color);
    }

    void erase() {
      draw_with_rgb(BLACK);
    }

  private:
    int x;
    int MIN_X = 0;
    int y;
    int gravity;
    Color color;

    // sets private data members x and y to initial valuess
    void initialize(int x_arg, int y_arg) {
      x = x_arg;
      y = y_arg;
    }

    // draws the player
    void draw_with_rgb(Color color) {
      matrix.drawPixel(x + 1, y, color.to_333());
      matrix.drawPixel(x + 2, y, color.to_333());
      matrix.drawPixel(x + 1, y + 3, color.to_333());
      matrix.drawPixel(x + 2, y + 3, color.to_333());
      matrix.drawPixel(x, y + 1, color.to_333());
      matrix.drawPixel(x, y + 2, color.to_333());
      matrix.drawPixel(x + 3, y + 1, color.to_333());
      matrix.drawPixel(x + 3, y + 2, color.to_333());
    }
};

//object to run the game
class ColorSwitch {
  public:
    ColorSwitch() {
      timeElapsed = 0;
    }

    // sets up a new game of color switch
    void setup() {
      matrix.fillScreen(0);
      player.draw();
      reset_level();
    }

    // displays and runs color switch
    void loop(int potentiometer_value, bool button_pressed) {
      timeElapsed += 1;

      //updates the obstacles
      if (timeElapsed % 750 == 0)
        updateObstacles();

      //updates Player position and decreses the obstacles if needed
      if (timeElapsed % 500 == 0) {
        player.incGravity();
        player.set_x(player.get_x() - player.get_gravity());
        player.draw();

        if (player.get_gravity() < 0 && player.get_x() + 3 > 15) {
          for (int i = 0; i < NUM_QUEUE; i++)
            if (obstacles[i].is_set()) {
              obstacles[i].dec_x(player.get_color());
              if (obstacles[i].get_x() == -1)
                obstacles[i] = generate();
            }
            else obstacles[i] = generate();
          obstacles_x--;
        }
      }

      //handles player motion
      if (button_pressed) {
        if (allowButton) {
          player.set_gravity(-3);
          allowButton = false;
          button_time = BTN_CLICK_DELAY;
        }
      }
      else if (--button_time == 0)
        allowButton = true;
    }

    //updates and redraws all the obstacles
    void updateObstacles() {
      for (int i = 0; i < NUM_QUEUE; i++) {
        obstacles[i].update();
        if (obstacles[i].is_set() && !obstacles[i].draw(player.get_x(), player.get_color())) {
          if (obstacles[i].get_type() == ALTER) {
            player.randColor();
            obstacles[i].erase();
            obstacles[i].set_set(false);
            score++;
          }
          else if ( keepPlaying() ) {
            restart();
            player.set_x(2);
          }
          else {
            matrix.fillScreen(0);
            delay(100);
            inColorSwitch = false;
            exitColorSwitch = true;
            inMenu = true;
            return;
          }
        }
      }
    }



  private:
    //represents number of loop iterations
    int timeElapsed;
    //whether or not to allow the button to be pressed
    bool allowButton = true;
    //cooldown of button press
    int button_time;

    //score -- how many obstacles they've jumped through
    //incremented each time they hit an alter
    int score = 0;

    //baseline player object
    ColorSwitchPlayer player;
    //all the obstacles that are loaded at any time
    Obstacle obstacles[NUM_QUEUE];

    //handles player death
    void restart() {
      matrix.fillScreen(0);

      matrix.setRotation(1);
      matrix.setTextColor(0x00FF00);
      matrix.setCursor(1, 12);
      matrix.print(score / 10);
      matrix.setCursor(10, 12);
      matrix.print(score % 10);
      matrix.setRotation(0);
      matrix.fillRect(0, 0, 12, 16, 0);
      matrix.fillRect(20, 0, 12, 16, 0);
      delay(2500);
      matrix.fillScreen(0);

      score = 0;
      reset_level();
    }

    // set up a level
    void reset_level() {
      randomSeed(analogRead(0));
      obstacles_x = 10;
      numObstacles = 1;

      for (int i = 0; i < NUM_QUEUE; i++)
        obstacles[i] = generate();
      player.randColor();
    }

    //determines the next obstacle to add to the queue
    int numObstacles = 1;
    int obstacles_x = 10;
    Obstacle generate() {
      numObstacles++;
      if (numObstacles % 2 == 1) {
        obstacles_x += 10;
        return Obstacle(obstacles_x, ALTER);
      }
      else {
        switch (random(0, NUM_OBSTACLES)) {
          case SQUARE:
            obstacles_x += 25;
            return Obstacle(obstacles_x, SQUARE);
          case DIAMOND:
            obstacles_x += 25;
            return Obstacle(obstacles_x, DIAMOND);
          case BAR:
            obstacles_x += 10;
            return Obstacle(obstacles_x, BAR);
        }
      }
    }
};

// ======== PILOT CHASE ========

const int NUM_WALLS = 16;
const int MIN_GAP = 6;

class Exhaust {
  public:

    Exhaust() {
      x = 0;
      y = 0;
      color = 0;
    }
    Exhaust(int x_arg, int y_arg, int spd) {
      x = x_arg;
      y = y_arg;
      color = spd;
    }

    void move(int spd, int player_y) {
      erase();
      x -= 1;
      if (x < 0) {
        color = spd;
        x += 8;
        y = player_y;
      }
      draw();
    }

    void draw() {
      matrix.drawPixel(x, y, Color(7, min(color, 7), max(color - 10, 0)).to_333());
    }
    void erase() {
      matrix.drawPixel(x, y, BLACK.to_333());
    }
  private:
    int x;
    int y;
    int color;
};

class Tunnel {
  public:
    // Constructors
    Tunnel() {
      x = 0;
      y = 0;
      gap_y = 0;
    }

    Tunnel(int x_arg, int y_arg, int gy, int gsize) {
      x = x_arg;
      y = y_arg;
      gap_y = gy;
      gap_size = gsize;
    }



    // Getters
    int get_x() const {
      return x;
    }
    int get_y() const {
      return y;
    }

    int get_gap_y() const {
      return gap_y;
    }
    int get_gap_size() const {
      return gap_size;
    }

    // Moves the Tunnel down the screen by one row
    // Modifies: y
    void move(int g_y, int g_size) {
      erase();
      x -= 1;
      if (x < -1) {
        gap_y = g_y;
        gap_size = g_size;
        x += 32;
      }
      draw();
    }

    // draws the Tunnel if its strength is greater than 0
    // calls: draw_with_rgb
    void draw() {
      draw_with_rgb(GREEN);
    }

    // draws black where the Tunnel used to be
    // calls: draw_with_rgb
    void erase() {
      draw_with_rgb(BLACK);
    }

  private:
    int x;
    int y;
    int gap_y;
    int gap_size;

    // draws the Tunnel
    void draw_with_rgb(Color body_color) {
      for (int i = 0; i < 2; i++) {
        matrix.drawPixel(x + i, 0, GREEN.to_333());
        matrix.drawPixel(x + i, 15, GREEN.to_333());
        for (int j = 1; j < gap_y; j++) {
          matrix.drawPixel(x + i, y + j, body_color.to_333());
        }
        for (int j = gap_y + gap_size; j < 15; j++) {
          matrix.drawPixel(x + i, y + j, body_color.to_333());
        }
      }
    }
};

class Item {
  public:

    // Constructors
    Item() {
      x = 31;
      y = 7;
      type = 's';
    }

    // Setters
    void set_x(int x_arg) {
      x = x_arg;
    }

    void set_y(int y_arg) {
      y = y_arg;
    }

    void set_type(char type_arg) {
      type = type_arg;
    }

    void setInUse(bool use_arg) {
      inUse = use_arg;
    }

    // Getters

    int get_x() {
      return x;
    }

    int get_y() {
      return x;
    }

    char get_type() {
      return type;
    }

    bool isSpawned() {
      return spawned;
    }

    // Animation

    void move( int timeElapsed ) {
      erase();
      if (timeElapsed % 5 == 0) {
        x -= 1;
      }
      if (timeElapsed % 15 == 0) {
        rotate();
      }
      draw();
    }

    void rotate() {
      if (rotated) {
        rotated = false;
      }
      else {
        rotated = true;
      }
    }

    void draw() {
      Color color;

      switch (type) {
        case 's':
          color = PURPLE;
          break;
      }

      if (rotated) {
        draw_with_rgb_rotated(color);
      }
      else {
        draw_with_rgb(color);
      }

    }

    void erase() {
      if (rotated) {
        draw_with_rgb_rotated(BLACK);
      }
      else {
        draw_with_rgb(BLACK);
      }
    }



    // States

    void despawn() {

      spawned = false;
      erase();
    }

    void spawn() {

      if ( !inUse ) {

        spawned = true;
        x = 31;
        y = 7;
        draw();
      }
    }

  private:
    int x = 0;
    int y = 0;
    char type = 's';

    // item exists on playing field
    bool spawned = false;

    // item picked up by player
    bool inUse = false;

    // rotate item animation cause it looks cool
    bool rotated = false;


    void draw_with_rgb(Color color) {
      matrix.drawPixel(x + 1, y, color.to_333());
      matrix.drawPixel(x + 1, y + 1, color.to_333());
      matrix.drawPixel(x + 1, y + 2, color.to_333());
      matrix.drawPixel(x, y + 1, color.to_333());
      matrix.drawPixel(x + 2, y + 1, color.to_333());
    }

    void draw_with_rgb_rotated(Color color) {
      matrix.drawPixel(x, y, color.to_333());
      matrix.drawPixel(x + 2, y, color.to_333());
      matrix.drawPixel(x + 1, y + 1, color.to_333());
      matrix.drawPixel(x, y + 2, color.to_333());
      matrix.drawPixel(x + 2, y + 2, color.to_333());
    }

};

class PilotChasePlayer {
  public:

    // Constructors

    PilotChasePlayer() {
      x = 8;
      y = 2;
    }

    // Getters

    int get_x() const {
      return x;
    }

    int get_y() const {
      return y;
    }

    int get_gravity() const {
      return gravity;
    }

    bool shieldActive() {
      return shield;
    }

    // Setters

    void set_x(int x_arg) {
      erase();
      x = x_arg;
      draw();
    }

    void set_y(int y_arg) {
      erase();
      y = y_arg;
      draw();
    }

    void set_gravity(int g_arg) {
      gravity = g_arg;
    }

    void incGravity() {
      if (gravity < 2) {
        gravity += 1;
      }
    }

    // Animation

    void draw() {
      draw_with_rgb(AQUA);
    }

    void erase() {
      draw_with_rgb(BLACK);
      if (shield) {
        draw_shield(BLACK);
      }
    }

    // States

    void shieldGet() {
      shield = true;
    }

    void shieldUse() {
      shield = false;
      draw_shield(BLACK);
    }

  private:
    int x;
    int y;
    int gravity;

    bool shield = false;

    // sets private data members x and y to initial valuess
    void initialize(int x_arg, int y_arg) {
      x = x_arg;
      y = y_arg;
    }

    // draws the player
    void draw_with_rgb(Color color) {
      matrix.drawPixel(x, y, color.to_333());
      matrix.drawPixel(x + 1, y, color.to_333());
      matrix.drawPixel(x + 1, y + 1, color.to_333());
      matrix.drawPixel(x + 2, y + 1, color.to_333());
      matrix.drawPixel(x, y + 2, color.to_333());
      matrix.drawPixel(x + 1, y + 2, color.to_333());

      // draws shield if active
      if (shield) {
        draw_shield(PURPLE);
      }
    }

    // draws shield

    void draw_shield(Color color) {

      for (int i = 0; i <= 2; ++i) {
        matrix.drawPixel(x + i, y - 1, color.to_333());
        matrix.drawPixel(x + i, y + 3, color.to_333());
      }
      for (int i = 0; i <= 2; ++i) {
        matrix.drawPixel(x + 3, y + i, color.to_333());
      }
    }
};

class PilotChase {
  public:
    PilotChase() {
      timeElapsed = 0;
    }

    void setup() {
      player.set_y(7);
      newLevel();
    }

    void loop(int potentiometer_value, bool button_pressed) {

      timeElapsed += 1;

      // Player position
      if (timeElapsed % 2 == 0) {
        if (active) {
          player.incGravity();
          player.set_y(player.get_y() + player.get_gravity());
          if (player.get_y() > 16) {
            player.set_y(16);
          }
        }
        player.draw();
      }

      // Jumping
      if (button_pressed) {
        if (!active) {
          active = true;
          timeElapsed = 1;
        }
        jump();
      }

      // Item position/collisions

      // Shield:

      // Spawning
      if (score >= 3000 &&
          !shieldItem.isSpawned() &&
          !player.shieldActive()) {

        shieldItem.spawn();
      }

      if (shieldItem.isSpawned()) {

        // Shield item movement
        shieldItem.move( timeElapsed );

        // Shield item collisions

        if ( player.get_x() + 2 >= shieldItem.get_x() &&
             player.get_y() <= shieldItem.get_y() + 2 &&
             player.get_y() + 2 >= shieldItem.get_y() ) {

          shieldItem.despawn();
          player.shieldGet();
          shieldItem.setInUse( true );
        }
      }


      // Walls

      if (timeElapsed % 2 == 0) {
        if (active) {
          score += (14 - std_size) * (1 + pow(potentiometer_value / 50, 1.12));
        }
        std_y += 1 - random(0, 3);
        if (std_y < 1) {
          std_y = 1;
        }
        if (std_y > 15 - std_size) {
          std_y = 15 - std_size;
        }
      }

      // Exhaust trails
      for (int i = 0; i < 5; i++) {
        trail[i].move(potentiometer_value / 73, player.get_y() + 1);
      }
      for (int i = 0; i < NUM_WALLS; i++) {
        walls[i].move(std_y, std_size);
      }



      if (timeElapsed % 250 == 0 && std_size > MIN_GAP && active) {
        std_size -= 1;
      }

      if ( !inPilotChase ) {
        matrix.fillScreen(0);
      }

      // Player collisions
      for (int i = 0; i < NUM_WALLS; i++) {

        // Has shield
        if ( player.shieldActive() ) {

          if ( player.get_y() <= walls[i].get_gap_y() &&
               player.get_x() <= walls[i].get_x() + 1 &&
               player.get_x() + 1 >= walls[i].get_x()) {

            player.set_y( player.get_y() + 3 );
            player.shieldUse();
            shieldItem.setInUse( false );
          }

          else if ( player.get_y() + 3 >= walls[i].get_gap_y() + walls[i].get_gap_size() &&
                    player.get_x() <= walls[i].get_x() + 1 &&
                    player.get_x() + 1 >= walls[i].get_x() ) {

            player.set_gravity(-3);
            player.shieldUse();
            shieldItem.setInUse( false );
          }
        }

        // No shield

        //Collision at top
        else if ((player.get_y() + 1 <= walls[i].get_gap_y()
                  //Collision at bottom
                  || player.get_y() + 2 >= walls[i].get_gap_y() + walls[i].get_gap_size())
                 //Collision on left
                 && player.get_x() <= walls[i].get_x() + 1
                 //Collision on right
                 && player.get_x() + 1 >= walls[i].get_x()) {

          button_pressed = false;
          displayScore();
          if ( keepPlaying() ) {

            newLevel();
          }
          else {

            matrix.fillScreen(0);
            delay(100);
            inPilotChase = false;
            exitPilotChase = true;
            inMenu = true;
            return;
          }
        }
      }

      // Timestep
      delay(120 - (potentiometer_value / 10));
    }



  private:
    bool active = false;
    int std_y = 2;
    int std_size = 13;
    int timeElapsed;

    long score = 0;
    long high = 0;

    PilotChasePlayer player;
    Item shieldItem;

    //number of walls in a row
    Tunnel walls[NUM_WALLS];

    //width of an enemy
    const int WIDTH = 2;

    Exhaust trail[5];

    // Resets level after player death
    void newLevel() {

      matrix.fillScreen(0);
      getReady();
      buildLevel();
    }

    // Countdown to begin the level
    void getReady() {
      int hue = 0;

      matrix.setTextColor(matrix.ColorHSV(hue, 255, 255, true));
      matrix.setCursor(1, 1);
      matrix.print("READY");

      int countdown = 3;
      matrix.setCursor(1, 9);
      matrix.print("-=");
      matrix.print(countdown);
      matrix.print("=-");
      delay(1000);
      countdown--;
      matrix.fillRect(13, 9, 5, 7, 0);
      matrix.setCursor(13, 9);
      matrix.print(countdown);
      delay(1000);
      countdown--;
      matrix.fillRect(13, 9, 5, 7, 0);
      matrix.setCursor(13, 9);
      matrix.print(countdown);
      delay(1000);
      matrix.fillScreen(0);
      active = false;
    }

    // Display score after player death
    void displayScore() {

      delay(1000);

      matrix.fillScreen(0);
      matrix.setTextColor(0x00FF00);
      matrix.setCursor(1, 1);
      matrix.print("SCORE");
      matrix.setCursor(1, 9);
      if (score < 100000) {
        matrix.print(score);
      } else {
        matrix.print("wtfm8");
      }
      delay(2500);
      high = max(high, score);
      matrix.fillScreen(0);
      matrix.setCursor(1, 1);
      matrix.print("HIGH");
      matrix.setCursor(1, 9);
      if (high < 100000) {
        matrix.print(high);
      } else {
        matrix.print("wtfm8");
      }
      delay(2500);
    }

    // set up a level
    void buildLevel() {
      std_size = 13;
      score = 0;
      timeElapsed = 0;

      player.set_y(7);
      player.set_gravity(2);
      player.shieldUse();
      shieldItem.despawn();

      for (int i = 0; i < NUM_WALLS; i++) {
        walls[i] = Tunnel(WIDTH * i, 0, 1, 14);
      }
      // draw all of the Tunnels
      for (int i = 0; i < NUM_WALLS; i++) {
        walls[i].draw();
      }
      for (int i = 0; i < 5; i++) {
        trail[i] = Exhaust(i * 2, 8, 0);
      }
      walls[0].draw();
    }

    void jump() {
      player.set_gravity(-2);
    }
};

PilotChase* pilotchase;
ColorSwitch* colorswitch;
Mastermind* mastermind;

void setup() {
  Serial.println("Use Serial.println() to print to the console!");

  Serial.begin(9600);
  pinMode(BUTTON_PIN_NUMBER, INPUT);
  matrix.begin();
}

void loop() {
  potentiometer_value = analogRead(POTENTIOMETER_PIN_NUMBER);
  button_pressed = (digitalRead(BUTTON_PIN_NUMBER) == HIGH);

  // Destroy active pointers, if request to exit is received

  if ( exitPilotChase ) {
    delete pilotchase;
    Serial.print("Exit pilotchase success");
    matrix.fillScreen(0);
    button_pressed = false;
  }
  else if ( exitColorSwitch ) {
    delete colorswitch;
    Serial.print("Exit colorswitch success");
    matrix.fillScreen(0);
    button_pressed = false;
  }
  else if ( exitMastermind ) {
    delete mastermind;
    Serial.print("Exit mastermind success");
    matrix.fillScreen(0);
    button_pressed = false;
  }

  // Menu

  if ( inMenu ) {
    exitColorSwitch = false;
    exitPilotChase = false;
    exitMastermind = false;

    // Print menu text

    matrix.setTextColor( WHITE.to_333() );
    matrix.setCursor(1, 1);
    matrix.print("Game?");

    // Read Game selection
    if ( potentiometer_value <= 333 ) {

      if ( currentVal != 1) {
        matrix.fillRect(0, 8, 32, 16, BLACK.to_333() );
      }
      gameNum = 1;
      currentVal = gameNum;
      matrix.setCursor(1, 9);
      matrix.setTextColor( GREEN.to_333() );
      matrix.print("PILOT");

    }
    else if ( potentiometer_value <= 667 ) {

      if ( currentVal != 2) {
        matrix.fillRect(0, 8, 32, 16, BLACK.to_333() );
      }
      gameNum = 2;
      currentVal = gameNum;
      matrix.setCursor(1, 9);
      matrix.setTextColor( RED.to_333() );
      matrix.print("C");
      matrix.setTextColor( ORANGE.to_333() );
      matrix.print("O");
      matrix.setTextColor( YELLOW.to_333() );
      matrix.print("L");
      matrix.setTextColor( GREEN.to_333() );
      matrix.print("O");
      matrix.setTextColor( BLUE.to_333() );
      matrix.print("R");
    }
    else {

      if ( currentVal != 3) {
        matrix.fillRect(0, 8, 32, 16, BLACK.to_333() );
      }

      gameNum = 3;
      currentVal = gameNum;
      matrix.setCursor(1, 9);
      matrix.setTextColor( RED.to_333() );
      matrix.print("MMIND");
    }

    // Select game
    if ( button_pressed ) {

      switch ( gameNum ) {
        case 1:
          inPilotChase = true;
          inMenu = false;
          matrix.fillScreen(0);
          matrix.setTextColor( GREEN.to_333() );
          scroll("PILOT CHASE", 11);

          pilotchase = new PilotChase;
          pilotchase->setup();
          button_pressed = false;
          break;

        case 2:
          inColorSwitch = true;
          inMenu = false;
          matrix.fillScreen(0);
          matrix.setTextColor( PURPLE.to_333() );
          scroll("COLOR SWITCH", 12);

          colorswitch = new ColorSwitch;
          colorswitch->setup();
          button_pressed = false;
          break;

        case 3:
          inMastermind = true;
          inMenu = false;
          matrix.fillScreen(0);
          matrix.setTextColor( RED.to_333() );
          scroll("MASTERMIND", 10);
          Serial.println("mastermind about to start!");
          mastermind = new Mastermind;
          Serial.println("mastermind really about to start!");
          mastermind->setup();
          Serial.println("mastermind start");
          button_pressed = false;
          break;
      }

    }
  }


  // Check which game is active
  if ( inPilotChase ) {

    pilotchase->loop(potentiometer_value, button_pressed);
  }
  else if ( inColorSwitch ) {

    colorswitch->loop(potentiometer_value, button_pressed);
  }
  else if ( inMastermind ) {

    mastermind->loop(potentiometer_value, button_pressed);
  }
  else {
    inMenu = true;
  }
}

