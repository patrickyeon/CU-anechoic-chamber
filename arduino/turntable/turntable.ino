const int stepdelay = 10; // in ms
long laststep;
int curstep;

const int gopin = 12;
const int dirpin = 11;

const int phase1 = 7;
const int phase2 = 8;
const int phase3 = 9;
// make some tri-state outputs
#define OUTZ(pin) pinMode(pin, INPUT); digitalWrite(pin, LOW);
#define OUTL(pin) pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
#define OUTH(pin) pinMode(pin, OUTPUT); digitalWrite(pin, HIGH);

void setup(){
  OUTZ(phase1);
  OUTZ(phase2);
  OUTZ(phase3);
  laststep = millis();
  curstep = 0;
  pinMode(gopin, INPUT);
  digitalWrite(gopin, HIGH);
  pinMode(dirpin, INPUT);
  digitalWrite(dirpin, HIGH);
}

void loop(){
  if(millis() - laststep < stepdelay)
    return;
  if(digitalRead(gopin) == HIGH)
    return;
  
  curstep = (digitalRead(dirpin) == HIGH)? stepL(curstep) : stepR(curstep);
  laststep = millis();
}

int stepL(int from){
  int to = (from + 1) % 6;
  stepto(to);
  return(to);
}

int stepR(int from){
  int to = from - 1;
  if(to < 0)
    to = 5;
  stepto(to);
  return(to);
}

void stepto(int pos){
  switch(pos){
    case 0:
      OUTH(phase1);
      OUTZ(phase2);
      OUTL(phase3);
      break;
    case 1:
      OUTH(phase1);
      OUTL(phase2);
      OUTZ(phase3);
      break;
    case 2:
      OUTZ(phase1);
      OUTL(phase2);
      OUTH(phase3);
      break;
    case 3:
      OUTL(phase1);
      OUTZ(phase2);
      OUTH(phase3);
      break;
    case 4:
      OUTL(phase1);
      OUTH(phase2);
      OUTZ(phase3);
      break;
    case 5:
      OUTZ(phase1);
      OUTH(phase2);
      OUTL(phase3);
      break;
    default:
      OUTZ(phase1);
      OUTZ(phase2);
      OUTZ(phase3);
  }
}
