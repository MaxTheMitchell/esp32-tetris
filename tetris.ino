#include <PxMatrix.h>

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 16
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#define matrix_width 64
#define matrix_height 64

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=10; //30-70 is usually fine

PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

const uint16_t WHITE = display.color565(255, 255, 255);
const uint16_t RED = display.color565(255, 0, 0);
const uint16_t BLUE = display.color565(0, 255, 0);
const uint16_t GREEN = display.color565(0, 0, 255);
const uint16_t YELLOW = display.color565(255, 0, 255);
const uint16_t PURPLE = display.color565(255, 255, 0);
const uint16_t ORANGE = display.color565(255, 0, 125);
const uint16_t LIGHT_BLUE = display.color565(173, 255, 216);

const int POT_PIN_MOVE = 34;
const int POT_PIN_ROTATE = 32;
const int BLOCK_SIZE = 2;
const int PIECE_SIZE = 4;
const int NUMBER_OF_PIECES = 7;
const int GRID_START_X = 2;
const int GRID_START_Y = 2;
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 30;
const int PIECES_PER_LEVEL = 10;
int GRID_END_X = GRID_START_X+GRID_WIDTH*BLOCK_SIZE;
const float CURVE_CONST = .3;

uint16_t grid[GRID_WIDTH][GRID_HEIGHT];
uint16_t PIECES[NUMBER_OF_PIECES][PIECE_SIZE][PIECE_SIZE] ={
    {
      {PURPLE,0,0,0},
      {PURPLE,0,0,0},
      {PURPLE,0,0,0},
      {PURPLE,0,0,0}
    },
    {
      {LIGHT_BLUE,LIGHT_BLUE,0,0},
      {LIGHT_BLUE,LIGHT_BLUE,0,0},
      {0,0,0,0},
      {0,0,0,0}
    },
    {
      {0,BLUE,0,0},
      {BLUE,BLUE,BLUE,0},
      {0,0,0,0},
      {0,0,0,0}
    },
    {
      {RED,0,0,0},
      {RED,0,0,0},
      {RED,RED,0,0},
      {0,0,0,0}
    },
    {
      {0,YELLOW,0,0},
      {0,YELLOW,0,0},
      {YELLOW,YELLOW,0,0},
      {0,0,0,0}
    },
    {
      {0,ORANGE,ORANGE,0},
      {ORANGE,ORANGE,0,0},
      {0,0,0,0},
      {0,0,0,0}
    },
    {
      {GREEN,GREEN,0,0},
      {0,GREEN,GREEN,0},
      {0,0,0,0},
      {0,0,0,0}
    }
  };
int DELAY = 50;
int fallSpeed = 10;
int fallIter = 0;
int peicesFallen =0;
int pieceType;
int nextPieceType;
int y=0;
int pastX;
uint16_t currentPiece[PIECE_SIZE][PIECE_SIZE];
float lastRotationVal;
float lastMoveVal;
int score = 0;

void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void display_update_enable(bool is_enable){
  if (is_enable){
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 4000, true);
    timerAlarmEnable(timer);
  }else{
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  display.begin(32);
  display.clearDisplay();
  display_update_enable(true);
  delay(4000);
  for(int x=0;x<GRID_WIDTH;x++)
    for(int y=0;y<GRID_HEIGHT;y++)
      grid[x][y] = 0;
  pieceType = randomPieceType();
  nextPieceType = randomPieceType();
  lastRotationVal = potValue(POT_PIN_ROTATE);
  lastMoveVal = potValue(POT_PIN_MOVE);
  pastX = xValue(currentPiece);
}

void loop() {
  if(lost())
    displayLoseScreen();
  else
    playGame();
}

void playGame(){
  display.clearDisplay();
  displayGrid();
  displayNext(nextPieceType);
  displayScore(score);
  rotate(PIECES[pieceType]);
  displayPiece(xValue(currentPiece),y,currentPiece);
  if(fallIter>=fallSpeed){
    fall(xValue(currentPiece),y,currentPiece);
    fallIter=0;
  }
  fallIter++;
  delay(DELAY);
}

void displayLoseScreen(){
  display.clearDisplay();
  displayPiece(xValue(currentPiece),y,currentPiece);
  displayGrid();
  displayScore(score);
  while(true){
    displayYouLose();
    delay(150);
  }
}

void displayNext(int next){
  displayBox(GRID_END_X+1,GRID_START_Y-1,matrix_width-2,(matrix_height/2)-1);
  display.setTextColor(WHITE);
  display.setCursor(GRID_END_X+(matrix_width-GRID_END_X-24)/2,GRID_START_Y+2);
  display.print("Next");
  displayNextPiece(GRID_END_X+(matrix_width-GRID_END_X-4*4)/2,GRID_START_Y+12,PIECES[next],4);
}

void displayYouLose(){
  displayBox(GRID_END_X+1,GRID_START_Y-1,matrix_width-2,(matrix_height/2)-1);
  display.setTextColor(display.color565(random(255),random(255),random(255)));
  display.setCursor(GRID_END_X+(matrix_width-GRID_END_X-16)/2,GRID_START_Y+5);
  display.print("YOU");
  display.setCursor(GRID_END_X+(matrix_width-GRID_END_X-20)/2,GRID_START_Y+15);
  display.print("LOSE");
}

void displayScore(int score){
  String scoreText = (String)score;
  displayBox(GRID_END_X+1,(matrix_height/2)+1,matrix_width-2,matrix_height-2);
  display.setTextColor(WHITE);
  display.setCursor(GRID_END_X+(matrix_width-GRID_END_X-30)/2,(matrix_height/2)+2);
  display.print("Score");
  display.setCursor(GRID_END_X+(matrix_width-GRID_END_X-7*scoreText.length())/2,(matrix_height/2)+15);
  display.print(scoreText);
}

void displayGrid(){
  displayBoarder();
  for(int x=0;x<GRID_WIDTH;x++)
    for(int y=0;y<GRID_HEIGHT;y++)
      if(isBlock(grid[x][y]))
        displayBlock(x,y,grid[x][y]);
}

void displayNextPiece(int matrix_x, int matrix_y, uint16_t piece[PIECE_SIZE][PIECE_SIZE],int blockSize){
  for(int piece_x=0;piece_x<PIECE_SIZE*blockSize;piece_x++)
    for(int piece_y=0;piece_y<PIECE_SIZE*blockSize;piece_y++)
      display.drawPixel(matrix_x+piece_x,matrix_y+piece_y,piece[piece_x/blockSize][piece_y/blockSize]);
}

void displayPiece(int grid_x, int grid_y, uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  for(int piece_x=0;piece_x<PIECE_SIZE;piece_x++)
    for(int piece_y=0;piece_y<PIECE_SIZE;piece_y++)
      if(isBlock(piece[piece_x][piece_y]))
        displayBlock(actualPos(grid_x,piece_x,pieceStartX(piece)),actualPos(grid_y,piece_y,pieceStartY(piece)),piece[piece_x][piece_y]);      
}

void displayBlock(int x,int y,uint16_t color){
  for(int width=x*BLOCK_SIZE;width<x*BLOCK_SIZE+BLOCK_SIZE;width++)
    for(int height=y*BLOCK_SIZE;height<y*BLOCK_SIZE+BLOCK_SIZE;height++)
      display.drawPixel(GRID_START_X+width,GRID_START_Y+height,color);
}

void displayBoarder(){
  displayBox(GRID_START_X-1,GRID_START_Y-1,GRID_END_X,GRID_START_Y+GRID_HEIGHT*BLOCK_SIZE);
}

void displayBox(int x1,int y1, int x2,int y2){
  display.drawLine(x1,y1,x1,y2,WHITE);
  display.drawLine(x1,y1,x2,y1,WHITE);
  display.drawLine(x1,y2,x2,y2,WHITE);
  display.drawLine(x2,y1,x2,y2,WHITE);
}

void fall(int grid_x,int grid_y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  if(blockUnder(grid_x,grid_y,piece))
    land(grid_x,grid_y,piece);
  else 
    y++;
}

void land(int grid_x,int grid_y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  y=0;
  pieceType = nextPieceType;
  nextPieceType = randomPieceType();
  addToGrid(grid_x,grid_y,piece);
  clearRows(grid_y,piece);
  if(peicesFallen++>PIECES_PER_LEVEL && fallSpeed > 1){
    peicesFallen=0;
    fallSpeed--;
  }
}

void addToGrid(int grid_x,int grid_y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  for(int piece_x=0;piece_x<PIECE_SIZE;piece_x++)
    for(int piece_y=0;piece_y<PIECE_SIZE;piece_y++)
      if(isBlock(piece[piece_x][piece_y]))
        grid[actualPos(grid_x,piece_x,pieceStartX(piece))][actualPos(grid_y,piece_y,pieceStartY(piece))] = piece[piece_x][piece_y];
}

void rotate(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  rotate90(piece);
  for(int i =0; i<rotationValue()*4;i++)
    rotate90(currentPiece);
}

void rotate90(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  uint16_t tmpPiece[PIECE_SIZE][PIECE_SIZE];
  for(int x=0;x<PIECE_SIZE;x++)
    for(int y=0;y<PIECE_SIZE;y++){
      if(isBlock(piece[x][y]))
        tmpPiece[y][regularPoint(pointAroundOrigin(x)*-1)] = (uint16_t)piece[x][y];
      else
        tmpPiece[y][regularPoint(pointAroundOrigin(x)*-1)] = 0;
     }
  if(!pieceInBlock(xValue(tmpPiece),y,tmpPiece))
    for(int x=0;x<PIECE_SIZE;x++)
      for(int y=0;y<PIECE_SIZE;y++)
        currentPiece[x][y] = (uint16_t)tmpPiece[x][y];
}

int pointAroundOrigin(int point){
  if(point >= PIECE_SIZE/2)
    return point-1;
  return point-2;
}

int regularPoint(int point){
  if(point < 0)
    return point+2;
  return point+1;
}

int xValue(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  int currentX = (GRID_WIDTH-pieceWidth(piece))*moveValue();
  if(!pieceInBlock(currentX,y,piece))
    pastX = currentX;
  return pastX;
}

int actualPos(int grid,int piece,int pieceStart){
  return grid+piece-pieceStart;
}

int pieceStartX(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  for(int x=0;x<PIECE_SIZE;x++)
    for(int y=0;y<PIECE_SIZE;y++)
      if(isBlock(piece[x][y]))
        return x;
}

int pieceStartY(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
   for(int y=0;y<PIECE_SIZE;y++)
    for(int x=0;x<PIECE_SIZE;x++)
      if(isBlock(piece[x][y]))
        return y;
}

int pieceWidth(uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  int width = 0;
  for(int x=0;x<PIECE_SIZE;x++)
    for(int y=0;y<PIECE_SIZE;y++)
      if(isBlock(piece[x][y])){
        width++;
        break;
      }
  return width;
}

bool pieceInBlock(int grid_x,int grid_y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  for(int piece_x=0;piece_x<PIECE_SIZE;piece_x++)
    for(int piece_y=0;piece_y<PIECE_SIZE;piece_y++)
      if(
        isBlock(piece[piece_x][piece_y]) && 
        isBlock(grid[actualPos(grid_x,piece_x,pieceStartX(piece))][actualPos(grid_y,piece_y,pieceStartY(piece))])
        )return true;
  return false;
}

bool blockUnder(int grid_x,int grid_y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  for(int piece_x=0;piece_x<PIECE_SIZE;piece_x++)
    for(int piece_y=0;piece_y<PIECE_SIZE;piece_y++)
      if(
        isBlock(piece[piece_x][piece_y]) && 
        (isBlock(grid[actualPos(grid_x,piece_x,pieceStartX(piece))][actualPos(grid_y,piece_y,pieceStartY(piece))+1]
        || actualPos(grid_y,piece_y,pieceStartY(piece))+1 >= GRID_HEIGHT))
        )return true;
  return false;
}

void clearRows(int y,uint16_t piece[PIECE_SIZE][PIECE_SIZE]){
  int rowsCleared = 0;
  for(int row=0;row<PIECE_SIZE;row++)
    if(rowFull(y+row)){
      clearRow(y+row);
      rowsCleared++;
    }
  increaseScore(rowsCleared);
}

void clearRow(int row){
  for(int x=0;x<GRID_WIDTH;x++)
    grid[x][row] = 0;
  while(row >0){
    swapRows(row,row-1);
    row--;
  }
}

void swapRows(int row1,int row2){
  for(int x=0;x<GRID_WIDTH;x++){
    int tmp = grid[x][row1];
    grid[x][row1] = grid[x][row2];
    grid[x][row2] = tmp;
  }
}

void increaseScore(int rows){
  score += rows*rows;
}

bool rowFull(int y){
  for(int x=0;x<GRID_WIDTH;x++)
    if(!isBlock(grid[x][y]))
      return false;
  return true;
}

bool isBlock(uint16_t value){
  return value != 0;
}

bool lost(){
  return pieceInBlock(pastX,y,currentPiece);
}

int randomPieceType(){
  return random(NUMBER_OF_PIECES);
}

float rotationValue(){
  lastRotationVal = exponentialSmooth(potValue(POT_PIN_ROTATE),lastRotationVal);
  return lastRotationVal;
}

float moveValue(){
  lastMoveVal = exponentialSmooth(potValue(POT_PIN_MOVE),lastMoveVal);
  return lastMoveVal;
}

float exponentialSmooth(float newVal, float pastVal){
  return CURVE_CONST*newVal+(1-CURVE_CONST)*pastVal;
}

float potValue(int potPin){
  return (float)analogRead(potPin)/4000;
}
