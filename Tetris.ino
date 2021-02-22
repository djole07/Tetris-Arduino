#include "LedControl.h"
//#include <IRremote.h>     // za kontrolu preko daljinskog

#define MIN_DELAY 10        // for potentiometer.Potentiometerdefines delay between each drop
#define MAX_DELAY 100
#define NUMBER_OF_SHAPES 7

/*
  Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
  pin 12 is connected to the DataIn
  pin 11 is connected to the CLK
  pin 10 is connected to LOAD
  We have only a single MAX72XX.
*/

LedControl lc = LedControl(12, 11, 10, 1);

int latchPin = 5;
int clockPin = 6;
int dataPin = 4;

int potPin = 5;   // potentiometer connected to A5 pin

// const int recievePin = 7;   // prijemni pin za daljinski
// IRrecv irrecv(recievePin);
// decode_results results;


enum DIR {NOTHING, UP, LEFT, DOWN, RIGHT};        // moving directions

int Table[9][10];

int Shape[4][4];
int ShapeRot[4][4];

// AllShapes[NUMBER_OF_SHAPES][ROWS][COLOMNS]
int AllShapes[7][4][4] = {

  {
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {0, 0, 0, 0},
    {0, 1, 0, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0}
  },

  { {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
  },

  {
    {0, 0, 0, 0},
    {0, 0, 1, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0}
  },

  { {0, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 1},
    {0, 0, 0, 0}
  },

  { {0, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
  },

  { {0, 0, 0, 0},
    {0, 1, 1, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0}
  }
};

int NubmersDisplay[11] = {B00011000, B11011110, B00110100, B10010100, B11010010, B10010001, B00010001, B11011100, B00010000, B10010000, B11101111};    // ukljucuje 1 ako je LSB
int ShapesDisplay[7] = {B11011110, B11010110, B00010111, B00111011, B10011110, B01110110, B11010011};


int value;    // for storing Serial or signal from IR reciever


int shape_x, shape_y;   // koordinate gornjeg levog coska 4x4 figure. Treba nam da bi je kasnije prikazali na displeju

long delayTime = 1200;
int BRIGHT = 0;

int currentPieceIndex;     // represents index
int nextPieceIndex;

int myMove;

int score;
int clearedLines;

bool gameOver;
bool resetGame;
bool newFig;

void clearTable() {
  for (int i = 0; i < 8 ; i++) {
    for (int j = 1; j < 9; j++) {
      Table[i][j] = 0;
      lc.setLed(0, i, j - 1, false);
    }
  }
}

void brightnessDown() {
  if (BRIGHT >= 0) {
    BRIGHT--;
    lc.setIntensity(0, BRIGHT);
  }
}
void brightnessUp() {
  if (BRIGHT <= 15) {
    BRIGHT++;
    lc.setIntensity(0, BRIGHT);
  }
}
void speedDown() {
  delayTime += 50;
}
void speedUp() {
  delayTime -= 50;
}

void initialize() {

  // brisemo tablu
  clearTable();

  score = 0;
  gameOver = false;
  resetGame = false;
  newFig = true;

  shape_x = 3;
  shape_y = 0;

  // delayTime = 1200;    we will control delay using potentiometer
  clearedLines = 0;

  int cifre[10] = {B00011000, B11011110, B00110100, B10010100, B11010010, B10010001, B00010001, B11011100, B00010000, B10010000};    // ukljucuje 1 ako je LSB
  int figure[7] = {B11011110, B00010111, B11010110, B00111011, B10011110, B01110110, B11010011};
  //                  long      square    triangle      L        L inv

  updateShiftRegister(~0);
  delay(500);

  updateShiftRegister(NubmersDisplay[3]);
  delay(500);
  updateShiftRegister(~0);
  delay(500);

  updateShiftRegister(NubmersDisplay[2]);
  delay(500);
  updateShiftRegister(~0);
  delay(500);


  updateShiftRegister(NubmersDisplay[1]);
  delay(500);
  updateShiftRegister(~0);
  delay(500);

  updateShiftRegister(~0);
  delay(5);

}

void generateShape() {
  currentPieceIndex = nextPieceIndex;
  nextPieceIndex = random(0, NUMBER_OF_SHAPES - 1);

  // copy to Shape
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      Shape[i][j] = AllShapes[currentPieceIndex][i][j];
    }
  }

  newFig = false;
}

void copyToShape() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      Shape[i][j] = ShapeRot[i][j];
    }
  }
}

void copyToShapeRot() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ShapeRot[i][j] = Shape[i][j];
    }
  }
}

void rotateShape() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ShapeRot[- j + 3][i] = Shape[i][j];
    }
  }
}

bool canMove(int S[][4], int offset, int down) {    // S je skraceno od Shape. Prosledjujem ga kao argument jer cu nekad proslediti
  // rotiranu figuru i proveravati da li sme da se rotira. Ako ne sme onda ce funkcija vratiti false, u suprotom cu kasnije pozvati i funkcjiu
  // copyToShape koja ce primeniti rotaciju. Dok ako se figura ne rotira vec samo pomera (levo, desno, dole) onda cu proslediti obicnu figuru
  // jer nema potrebe da opet saljem rotiranukoju u koju bih morao prethodno da kopiram sadrzaj Shape[][]

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (Table[shape_y + i + down][shape_x + j + offset + 1] && S[i][j]) {
        // ideja je da poredimo trenutnu figuru sa delom table koji se nalazi u smeru u kom se krecemo pomerene za 1
        // problem je ako je i figuru crtamo na tablu pa ce ce gotovo javljati problem
        // resenje?  ne upisivati na Tablu dok se ne zavrsi padanje figure, vec samo stampati na matrici
        return false;         // ako imamo poklapanje 2 polja koja su zauzeta, imamo koliziju
      }
    }
  }

  return true;

}

void moveFigure(int offset, int down) {
  shape_x = shape_x + offset;    // ne moramo da proveravamo da li je figura van matrice jer ce funkcija canMove detektovati. To je razlog
  // zasto smo prosirili matricu za jednu kolonu levo i desno

  shape_y += down;  // takodje isti razlog zasto smo i redove prositili za 1
}

void displayTable() {
  int i, j;

  // ovo mozda da stampam samo kada se zavrsi padanje figure jer je ovo staticno do sledece figure
  for (i = 0 ; i < 8 ; i++) {
    for (j = 0 ; j < 8 ; j++) {
      if (Table[i][j + 1])
        lc.setLed(0, i, j, true);
      //else
      //lc.setLed(0, i, j, false);
    }
  }
}

void turnOnFigure() {
  //posebno ispisujemo figuru, al pre toga treba da pozovemo turnOffFigure jer ce nam se preklapati prethodni prikazi figure

  // ispis figure
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {

      if (Shape[i][j]) {
        lc.setLed(0, shape_y + i, shape_x + j, true);
      }
      //else {
      //  lc.setLed(0, shape_y + i, shape_x + j, false);
      //}

    }
  }
}

void turnOffFigure() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (Shape[i][j])
        lc.setLed(0, shape_y + i, shape_x + j, false);
    }
  }
}

void addFigureToTable() {     // kraj veka jedne padajuce figure koju stavljamo na tablu
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (Shape[i][j]) {
        Table[i + shape_y][j + shape_x + 1] = Shape[i][j];
      }
    }
  }
}

void checkLines() {
  int i, j;

  int counter = 0;
  bool flag;

  for (i = 0 ; i < 8 ; i++) {
    flag = false;

    for (j = 1 ; j < 9 && !flag ; j++) {
      if (!Table[i][j]) {
        flag = true;
      }
    }

    if (j == 9 && !flag) {
      clearLine(i);
      counter++;
      clearedLines++;

      displayScore();
    }
  }

  score += (counter > 2) * 100 + (counter > 3) * 100 + counter * 100 ;



  /*  if you don't use potentiometer
    if (delayTime > 201 && counter > 0) {
    delayTime -= 200;
    }
  */

}

void clearLine(int line) {
  for (int i = line - 1 ; i > 0 ; i--) {
    for (int j = 1 ; j < 9; j++) {
      Table[i + 1][j] = Table[i][j];

      if (Table[i + 1][j]) {
        lc.setLed(0, i + 1, j - 1, true);
      }
      else {
        lc.setLed(0, i + 1, j - 1, false);
      }
    }

  }
}

void updateShiftRegister(int data)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, data);
  digitalWrite(latchPin, HIGH);
}

void updateDelay() {
  int val = analogRead(potPin);
  delayTime = map(val, 0, 1024, MAX_DELAY, MIN_DELAY);

}

void displayScore() {
  updateShiftRegister(~(NubmersDisplay[clearedLines] ^ NubmersDisplay[10]));
  delay(1000);
}


void displayNextPiece() {
  updateShiftRegister(ShapesDisplay[nextPieceIndex]);
  delay(10);
}

void detectMove() {
  turnOffFigure();
  delayMicroseconds(1);

  bool newMove;


  switch (myMove) {
    case UP: rotateShape() ; newMove = canMove(ShapeRot, 0, 0); break;
    case RIGHT: newMove = canMove(Shape, 1, 0); break;
    case LEFT: newMove = canMove(Shape, -1, 0); break;
    case DOWN: newMove = canMove(Shape, 0, 1); break;
    case NOTHING : newMove = true; break;
  }

  if (newMove == false && myMove == DOWN) {
    newFig = true;
    addFigureToTable();
    checkLines();
    return;
  }

  if (newMove == true) {
    switch (myMove) {
      case UP: copyToShape(); break;        // ne treba nam rotateShape jer smo je vec rotirali
      case RIGHT: moveFigure(1, 0); break;
      case LEFT: moveFigure(-1, 0); break;
      case DOWN: moveFigure(0, 1); break;
      case NOTHING : moveFigure(0, 0); break;
    }
  }

  //turnOffFigure();
  //delay(1);

}

void endScreen() {

  clearTable();

  turnOffFigure();
  delayMicroseconds(1);

  Table[2][3] = 1;
  Table[2][6] = 1;

  Table[5][3] = 1;
  Table[5][4] = 1;
  Table[5][5] = 1;
  Table[5][6] = 1;


  displayTable();
  
  delay(1000);

  gameOver = false;
  resetGame = true;
}

void setup() {
  for (int i = 0 ; i < 8; i++) {  // prva i poslednja kolona, a to znaci da ce nam ispis na matricu poceti od pozicije 1  (i-1 ?)
    Table[i][0] = 1;
    Table[i][9] = 1;
  }

  for (int i = 0; i < 10; i++) {      // poslednji red stavimo na 1
    Table[8][i] = 1;
  }


  // irrecv.enableIRIn();    // inicijalizacija daljinskog
  // irrecv.blink13(true);

  pinMode(7, INPUT_PULLUP);    // left
  pinMode(8, INPUT_PULLUP);    // rotate
  pinMode(9, INPUT_PULLUP);    // right


  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);


  lc.shutdown(0, false);

  /* Set the brightness to a medium values */
  lc.setIntensity(0, BRIGHT);
  /* and clear the display */
  lc.clearDisplay(0);
  delay(500);


  // za unos komandi sa tastature
  Serial.begin(9600);
  randomSeed(analogRead(0));  // za random generator

  nextPieceIndex = random(0, NUMBER_OF_SHAPES - 1);
  initialize();

}


void loop() {

  //initialize();

  newFig = true;

  shape_x = 3;
  shape_y = 0;


  turnOffFigure();
  delayMicroseconds(1);
  // generisemo novu figuru
  generateShape();

  displayNextPiece();

  bool newMove = canMove(Shape, 0, 0);

  if (newMove == false) {
    Serial.println("GameOver");
    Serial.print("Vas skor je: ");
    Serial.println(score);

    displayScore();

    endScreen();
    initialize();
    generateShape();

    displayNextPiece();
  }


  int button1_counter = 0;
  int button2_counter = 0;
  int button3_counter = 0;

  while (!newFig) {
    updateDelay();

    while (delayTime--) {

      myMove = NOTHING;

      displayTable();
      turnOnFigure();
      delayMicroseconds(1);  // hocemo da nam izmedju svakog pada imamo 900ms
      delay(15);
      /*
            if (irrecv.decode(&results)) {
              value = results.value;
              Serial.print("Vrednost je :");
              Serial.println(value, HEX);

              switch (value) {
                case 0xFE30CF: myMove = UP; Serial.print("UP"); break;
                case 0xFE708F: myMove = RIGHT; Serial.print("RIGHT"); break;
                case 0xFEF00F: myMove = LEFT; Serial.print("LEFT"); break;
                //case 0xFEB04F: myMove = DOWN; Serial.print("DOWN"); movesRemaining--; break;
                case 0xFEA857: resetGame = true; break;                         // restart   crveno dugme
                //case 0xFE6897: lc.shutdown(0, true); standBy = true; break;     // standBy   zeleno dugme
                case 0xFED827: brightnessUp(); break;           // VOL+
                case 0xFE58A7: brightnessDown(); break;         // VOL-
                case 0xFE9867: speedUp(); break;                // CH+
                case 0xFE18E7: speedDown(); break;              // CH-
                default: myMove = NOTHING; Serial.println("NESTO TRECE"); break;
              }
              irrecv.resume();
            }
      */

      /*
        if (Serial.available() > 0) {
        value = Serial.read();
        switch (value) {
          case '2': myMove = DOWN; break;
          case '4': myMove = LEFT; break;
          case '6': myMove = RIGHT; break;
          case '8': myMove = UP; break;
          default : myMove = NOTHING; break;
        }
        }
      */
      int button1 = digitalRead(7);
      int button2 = digitalRead(8);
      int button3 = digitalRead(9);

      Serial.print(button1);
      Serial.print(button2);
      Serial.println(button3);

      if (button1 == LOW) {
        button1_counter++;
      }

      if (button2 == LOW) {
        button2_counter++;
      }

      if (button3 == LOW) {
        button3_counter++;
      }

      if (button1_counter == 8) {
        myMove = LEFT;
        button1_counter = 0;
      }

      if (button2_counter == 8) {
        myMove = UP;
        button2_counter = 0;
      }

      if (button3_counter == 8) {
        myMove = RIGHT;
        button3_counter = 0;
      }


      //turnOffFigure();
      //delay(10);

      detectMove();

      //turnOnFigure();
      //delay(delayTime * 2 / 4);   // hocemo da nam izmedju svakog pada imamo 900ms
      //turnOffFigure();
      //delay(1);
    }

    myMove = DOWN;
    detectMove();
  }

}
