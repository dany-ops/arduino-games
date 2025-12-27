#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

MCUFRIEND_kbv tft;

// ================= JOYSTICK =================
#define JOY_X   A10
#define JOY_Y   A11
#define JOY_BTN 22

// ================= DISPLAY / BOARD =================
#define CELL 12
#define COLS 10
#define ROWS 20

#define OFFSET_X 60
#define OFFSET_Y 20

// ================= GAME =================
uint8_t board[ROWS][COLS];
uint16_t score = 0;
uint16_t lines = 0;
uint8_t gameMode = 1;   // 1 = normal, 2 = fast

unsigned long lastFall;
int speedNormal = 600;
int speedFast   = 300;

// ================= PIECES =================
uint8_t px = 3, py = 0;
uint8_t pid;

const uint8_t pieces[7][4][4] = {
  {{1,1,1,1}},
  {{1,1},{1,1}},
  {{1,1,1},{0,1,0}},
  {{0,0,1},{1,1,1}},
  {{1,1,1},{0,0,1}},
  {{1,1,0},{0,1,1}},
  {{0,1,1},{1,1,0}}
};

uint16_t colors[7] = {
  0x07FF, // cyan
  0xFFE0, // yellow
  0xF81F, // magenta
  0xFD20, // orange
  0x001F, // blue
  0xF800, // red
  0x07E0  // green
};

// ================= DRAW =================
void drawCell(int x,int y,uint16_t c){
  tft.fillRect(OFFSET_X + x*CELL, OFFSET_Y + y*CELL, CELL-1, CELL-1, c);
}

void drawBoard(){
  for(int y=0;y<ROWS;y++)
    for(int x=0;x<COLS;x++)
      if(board[y][x])
        drawCell(x,y,colors[board[y][x]-1]);
      else
        drawCell(x,y,0x0000);
}

void drawPiece(bool erase=false){
  for(int y=0;y<4;y++)
    for(int x=0;x<4;x++)
      if(pieces[pid][y][x])
        drawCell(px+x,py+y, erase ? 0x0000 : colors[pid]);
}

// ================= COLLISION =================
bool collide(int nx,int ny){
  for(int y=0;y<4;y++)
    for(int x=0;x<4;x++)
      if(pieces[pid][y][x]){
        int bx=nx+x, by=ny+y;
        if(bx<0||bx>=COLS||by>=ROWS) return true;
        if(by>=0 && board[by][bx]) return true;
      }
  return false;
}

// ================= ROTATE =================
void rotatePiece(){
  uint8_t tmp[4][4];
  for(int y=0;y<4;y++)
    for(int x=0;x<4;x++)
      tmp[x][3-y]=pieces[pid][y][x];

  for(int y=0;y<4;y++)
    for(int x=0;x<4;x++)
      ((uint8_t(*)[4][4])pieces)[pid][y][x]=tmp[y][x];
}

// ================= SCORE =================
void drawScore(){
  tft.fillRect(10,20,200,60,0x0000);
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(10,20);
  tft.print("SCORE:");
  tft.print(score);
  tft.setCursor(10,45);
  tft.print("LINES:");
  tft.print(lines);
}

// ================= GAME OVER =================
void gameOver(){
  tft.fillScreen(0x0000);
  tft.setTextColor(0xF800);
  tft.setTextSize(3);
  tft.setCursor(60,120);
  tft.print("GAME");
  tft.setCursor(60,160);
  tft.print("OVER");
  while(true);
}

// ================= CLEAR LINES =================
void checkLines(){
  for(int y=ROWS-1;y>=0;y--){
    bool full=true;
    for(int x=0;x<COLS;x++)
      if(!board[y][x]) full=false;

    if(full){
      lines++;
      score+=100;
      for(int yy=y;yy>0;yy--)
        for(int x=0;x<COLS;x++)
          board[yy][x]=board[yy-1][x];
      y++;
    }
  }
}

// ================= SETUP =================
void setup(){
  pinMode(JOY_BTN, INPUT_PULLUP);
  uint16_t ID=tft.readID();
  if(ID==0xD3D3) ID=0x9481;
  tft.begin(ID);
  tft.setRotation(1);
  tft.fillScreen(0x0000);

  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(80,80); tft.print("JOC 1");
  tft.setCursor(80,120);tft.print("JOC 2");

  int sel=0;
  while(digitalRead(JOY_BTN)){
    int jy=map(analogRead(JOY_Y),0,1023,-100,100);
    if(jy>40) sel=1;
    if(jy<-40) sel=0;
    tft.fillRect(60,80,10,60,0x0000);
    tft.fillRect(60,sel?120:80,10,20,0x07E0);
    delay(150);
  }

  gameMode=sel?2:1;
  tft.fillScreen(0x0000);
  randomSeed(millis());
  pid=random(7);
  drawBoard();
  drawScore();
}

// ================= LOOP =================
void loop(){
  static bool lastBtn=HIGH;
  bool btn=digitalRead(JOY_BTN);

  if(lastBtn && !btn){
    drawPiece(true);
    rotatePiece();
    if(collide(px,py)){
      rotatePiece();rotatePiece();rotatePiece();
    }
    drawPiece();
    delay(200);
  }
  lastBtn=btn;

  int jx=map(analogRead(JOY_X),0,1023,100,-100);
  int jy=map(analogRead(JOY_Y),0,1023,-100,100);

  if(abs(jx)>40){
    int nx=px+(jx>0?1:-1);
    if(!collide(nx,py)){
      drawPiece(true);px=nx;drawPiece();
    }
    delay(150);
  }

  int speed=(gameMode==1)?speedNormal:speedFast;
  if(jy>50) speed=80;

  if(millis()-lastFall>speed){
    lastFall=millis();
    if(!collide(px,py+1)){
      drawPiece(true);py++;drawPiece();
    }else{
      for(int y=0;y<4;y++)
        for(int x=0;x<4;x++)
          if(pieces[pid][y][x])
            board[py+y][px+x]=pid+1;

      checkLines();
      drawBoard();
      drawScore();

      px=3;py=0;pid=random(7);
      if(collide(px,py)) gameOver();
    }
  }
}
