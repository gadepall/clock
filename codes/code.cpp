// ---------------- Shared 7447 Inputs ----------------
const int PIN_A = 0, PIN_B = 1, PIN_C = 2, PIN_D = 3;

// ---------------- Enable pins for multiplexing ----------------
const int EN[] = {4,5,6,7,8,9};  // sec1, sec10, min1, min10, hr1, hr10

// ---------------- Buttons ----------------
const int PAUSE_BTN = 10;
const int NEXT_BTN  = 11;
const int INC_BTN   = 12;
const int DEC_BTN   = 13;

bool paused = false;
int lastPauseState = HIGH;
int lastNextState  = HIGH;
int lastIncState   = HIGH;
int lastDecState   = HIGH;

// Digit selection when paused
int selectedDigit = 0;  // 0..5 → sec1..hr10
unsigned long lastBlink = 0;
bool blinkOn = true;

// ---------------- Current States ----------------
// Seconds
int W1=0,X1=0,Y1=0,Z1=1;          
int W2=1,X2=0,Y2=1;               
// Minutes
int W3=1,X3=0,Y3=0,Z3=1;          
int W4=1,X4=0,Y4=1;               
// Hours
int W5=1,X5=1,Y5=0,Z5=0;          
int W6=0,X6=1,Y6=0;               

// Timer
unsigned long lastSecUpdate=0;

// ---------------- Setup ----------------
void setup(){
  pinMode(PIN_A,OUTPUT); pinMode(PIN_B,OUTPUT);
  pinMode(PIN_C,OUTPUT); pinMode(PIN_D,OUTPUT);
  for(int i=0;i<6;i++){ pinMode(EN[i],OUTPUT); digitalWrite(EN[i],LOW); }
  pinMode(PAUSE_BTN,INPUT_PULLUP);
  pinMode(NEXT_BTN,INPUT_PULLUP);
  pinMode(INC_BTN,INPUT_PULLUP);
  pinMode(DEC_BTN,INPUT_PULLUP);
}

// ---------------- Increment Helper ----------------
void incrementDigit(int d){
  int A,B,C,D;
  switch(d){
    case 0: { // sec ones 0–9
      A = !W1;
      B = (W1 && !X1 && !Z1)||(!W1 && X1);
      C = (!X1 && Y1)||(!W1 && Y1)||(W1 && X1 && !Y1);
      D = (!W1 && Z1)||(W1 && X1 && Y1);
      W1=A; X1=B; Y1=C; Z1=D;
    } break;
    case 1: { // sec tens 0–5
      A = !W2;
      B = (W2 && !X2 && !Y2)||(!W2 && X2);
      C = (W2 && X2)||(!W2 && !X2 && Y2);
      W2=A; X2=B; Y2=C;
    } break;
    case 2: { // min ones 0–9
      A = !W3;
      B = (W3 && !X3 && !Z3)||(!W3 && X3);
      C = (!X3 && Y3)||(!W3 && Y3)||(W3 && X3 && !Y3);
      D = (!W3 && Z3)||(W3 && X3 && Y3);
      W3=A; X3=B; Y3=C; Z3=D;
    } break;
    case 3: { // min tens 0–5
      A = !W4;
      B = (W4 && !X4 && !Y4)||(!W4 && X4);
      C = (W4 && X4)||(!W4 && !X4 && Y4);
      W4=A; X4=B; Y4=C;
    } break;
    case 4: { // hr ones
      if(X6==0){ // hr tens=0/1 → 0–9
        A = !W5;
        B = (W5 && !X5 && !Z5)||(!W5 && X5);
        C = (!X5 && Y5)||(!W5 && Y5)||(W5 && X5 && !Y5);
        D = (!W5 && Z5)||(W5 && X5 && Y5);
        W5=A; X5=B; Y5=C; Z5=D;
      } else { // hr tens=2 → 0–3
        A = !W5;
        B = (W5 && !X5)||(!W5 && X5);
        W5=A; X5=B; Y5=0; Z5=0;
      }
    } break;
    case 5: { // hr tens 0–2
      if(!(X6==0 && W6==1 && Y5==1)){
        A = !W6 && !X6;
        B = W6 && !X6;
        W6=A; X6=B; Y6=0;
      }
    } break;
  }
}

// ---------------- Decrement Helper ----------------
void decrementDigit(int d){
  int A,B,C,D;
  switch(d){
    case 0: { // sec ones 0–9
      A = !W1;
      B = (!X1 && !W1 && ((!Z1 && Y1)||(Z1 && !Y1))) || (!Z1 && W1 && X1); 
      C = (!Z1 && Y1 && (X1||W1)) || (Z1 && !X1 && !W1 && !Y1);
      D = !X1 && !Y1 && ((Z1 && W1) || (!Z1 && !W1));
      W1=A; X1=B; Y1=C; Z1=D;
    } break;
    case 1: { // sec tens 0–5
      A = !W2;
      B = (Y2 && !X2 && !W2) || (!Y2 && X2 && W2);
      C = !X2 && ((Y2 && W2) || (!Y2 && !W2));
      D = 0;
      W2=A; X2=B; Y2=C;
    } break;
    case 2: { // min ones 0–9
      A = !W3;
      B = (!X3 && !W3 && ((!Z3 && Y3)||(Z3 && !Y3))) || (!Z3 && W3 && X3);
      C = (!Z3 && Y3 && (X3||W3)) || (Z3 && !X3 && !W3 && !Y3);
      D = !X3 && !Y3 && ((Z3 && W3) || (!Z3 && !W3));
      W3=A; X3=B; Y3=C; Z3=D;
    } break;
    case 3: { // min tens 0–5
      A = !W4;
      B = (Y4 && !X4 && !W4) || (!Y4 && X4 && W4);
      C = !X4 && ((Y4 && W4) || (!Y4 && !W4));
      D = 0;
      W4=A; X4=B; Y4=C;
    } break;
    case 4: { // hr ones
      if(X6==0){ // hr tens=0/1
        A = !W5;
        B = (!X5 && !W5 && ((!Z5 && Y5)||(Z5 && !Y5))) || (!Z5 && W5 && X5);
        C = (!Z5 && Y5 && (X5||W5)) || (Z5 && !X5 && !W5 && !Y5);
        D = !X5 && !Y5 && ((Z5 && W5) || (!Z5 && !W5));
        W5=A; X5=B; Y5=C; Z5=D;
      } else { // hr tens=2 → 0–3
        A = !W5;
        B = (X5 && W5) || (!X5 && !W5);
        W5=A; X5=B; Y5=0; Z5=0;
      }
    } break;
    case 5: { // hr tens 0–2
      if(!(X6==0 && W6==0 && Y5==1)){
        A = X6 && !W6;
        B = !X6 && !W6;
        W6=A; X6=B; Y6=0;
      }
    } break;
  }
}

// ---------------- Main Loop ----------------
void loop(){
  // Pause toggle
  int pauseState = digitalRead(PAUSE_BTN);
  if(pauseState==LOW && lastPauseState==HIGH){
    paused=!paused;
    if(paused) selectedDigit=0;
    delay(50);
  }
  lastPauseState=pauseState;

  // Next cursor
  int nextState=digitalRead(NEXT_BTN);
  if(paused && nextState==LOW && lastNextState==HIGH){
    selectedDigit=(selectedDigit+1)%6;
    delay(50);
  }
  lastNextState=nextState;

  // Increment
  int incState=digitalRead(INC_BTN);
  if(paused && incState==LOW && lastIncState==HIGH){
    incrementDigit(selectedDigit);
    delay(50);
  }
  lastIncState=incState;

  // Decrement
  int decState=digitalRead(DEC_BTN);
  if(paused && decState==LOW && lastDecState==HIGH){
    decrementDigit(selectedDigit);
    delay(50);
  }
  lastDecState=decState;

  // Blink
  if(paused && millis()-lastBlink>=500){
    blinkOn=!blinkOn;
    lastBlink=millis();
  }

  // Display
  for(int d=0; d<6; d++){
    bool showThis=true;
    if(paused && d==selectedDigit && !blinkOn) showThis=false;
    if(showThis){
      switch(d){
        case 0: showDigit(W1,X1,Y1,Z1,EN[0]); break;
        case 1: showDigit(W2,X2,Y2,0,EN[1]);  break;
        case 2: showDigit(W3,X3,Y3,Z3,EN[2]); break;
        case 3: showDigit(W4,X4,Y4,0,EN[3]);  break;
        case 4: showDigit(W5,X5,Y5,Z5,EN[4]); break;
        case 5: showDigit(W6,X6,Y6,0,EN[5]);  break;
      }
      delay(1);
    }
  }

  // Time update
  if(!paused && millis()-lastSecUpdate>=1000){
    lastSecUpdate=millis();
    // Seconds Ones
    int A = !W1;
    int B = (W1 && !X1 && !Z1)||(!W1 && X1);
    int C = (!X1 && Y1)||(!W1 && Y1)||(W1 && X1 && !Y1);
    int D = (!W1 && Z1)||(W1 && X1 && Y1);
    W1=A; X1=B; Y1=C; Z1=D;
    // Seconds Tens
    if((W1|X1|Y1|Z1)==0){
      A = !W2;
      B = (W2 && !X2 && !Y2)||(!W2 && X2);
      C = (W2 && X2)||(!W2 && !X2 && Y2);
      W2=A; X2=B; Y2=C;
    }
    // Minutes Ones
    if((W1|X1|Y1|Z1|W2|X2|Y2)==0){
      A = !W3;
      B = (W3 && !X3 && !Z3)||(!W3 && X3);
      C = (!X3 && Y3)||(!W3 && Y3)||(W3 && X3 && !Y3);
      D = (!W3 && Z3)||(W3 && X3 && Y3);
      W3=A; X3=B; Y3=C; Z3=D;
      // Minutes Tens
      if((W3|X3|Y3|Z3)==0){
        A = !W4;
        B = (W4 && !X4 && !Y4)||(!W4 && X4);
        C = (W4 && X4)||(!W4 && !X4 && Y4);
        W4=A; X4=B; Y4=C;
      }
      // Hours Ones
      if((W3|X3|Y3|Z3|W4|X4|Y4)==0){
        if(X6==0){
          A = !W5;
          B = (W5 && !X5 && !Z5)||(!W5 && X5);
          C = (!X5 && Y5)||(!W5 && Y5)||(W5 && X5 && !Y5);
          D = (!W5 && Z5)||(W5 && X5 && Y5);
          W5=A; X5=B; Y5=C; Z5=D;
        } else {
          A = !W5;
          B = (W5 && !X5)||(!W5 && X5);
          W5=A; X5=B; Y5=0; Z5=0;
        }
        // Hours Tens
        if((W5|X5|Y5|Z5)==0){
          A = !W6 && !X6;
          B = W6 && !X6;
          W6=A; X6=B; Y6=0;
        }
      }
    }
  }
}

// ---------------- Display Helper ----------------
void showDigit(int A,int B,int C,int D,int ENpin){
  digitalWrite(PIN_A,A); digitalWrite(PIN_B,B);
  digitalWrite(PIN_C,C); digitalWrite(PIN_D,D);
  for(int i=0;i<6;i++) digitalWrite(EN[i],LOW);
  digitalWrite(ENpin,HIGH);
}
