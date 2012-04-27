const int stepdelay = 5; // in ms
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

// potentiometer and the limit switches
// pot is on an analog pin, the switches on dig. pins
const int potpin = 5;
// pot readings at +/- 90 degrees
int neg90 = 207;
int pos90 = 793;
const int llimpin = 2;
const int rlimpin = 4;
unsigned int lastpos = 0;

void setup(){
  OUTZ(phase1);
  OUTZ(phase2);
  OUTZ(phase3);

  laststep = millis();
  curstep = 0;

  init_din(gopin);
  init_din(dirpin);
  init_din(llimpin);
  init_din(rlimpin);
  
  // TODO tie A5 here to potpin
  pinMode(A5, INPUT);
  digitalWrite(A5, LOW);

  Serial.begin(115200);
  Serial.write("Alive.\n");
}

void init_din(int pin){
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
}

void loop(){
  if(Serial.available() > 0){
    char incmd[17] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    int i = 0;
    while((Serial.available() > 0) && (Serial.peek() != ';') && (i < 16)){
      incmd[i++] = (char)Serial.read();
      if(Serial.available() == 0)
        delay(10);
    }
    if(Serial.peek() == ';'){
      // if it's not, they sent us a command that's too long
      Serial.read();
      incmd[i] = '\0';
      runcmd(incmd);
    }
    return;
  }

  if(millis() - laststep < stepdelay)
    return;
  if(digitalRead(gopin) == HIGH)
    return;

  curstep = (digitalRead(dirpin) == HIGH)? stepL(curstep) : stepR(curstep);
  laststep = millis();
}

void runcmd(char *cmd){
  String cmdstr = String(cmd);
  cmdstr.toUpperCase();
  if(cmdstr.startsWith("CAL")){
    calibrate();
  }
  else if(cmdstr.startsWith("GET")){
    read_angle();
  }
  else if(cmdstr.startsWith("SET")){
    set_angle(&cmdstr);
  }
  else{
    ser_error(&cmdstr);
  }
}

void calibrate(){
  Serial.print("Start calibration.");
}

void read_angle(){
  int ang = angle();
  Serial.print("Angle: ");
  Serial.println(ang);
}

void set_angle(String *cmd){
  String moveto = (*cmd).substring((*cmd).indexOf(' ') + 1);
  char buff[18];
  (String("Moving to ") + moveto).toCharArray(buff, 18);
  
  Serial.println(buff);
}

void ser_error(String *cmd){
  char bad_cmd[22 + 16];
  (String("Unrecognized command: ") + (*cmd) + "\n").toCharArray(bad_cmd, 22 + 16);
  Serial.print(bad_cmd);
}



int angle(){
  int pot = analogRead(potpin);
  return((((long)(pot - neg90) * 180) / (pos90 - neg90)) - 90);
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
