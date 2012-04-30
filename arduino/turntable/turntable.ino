int stepdelay = 5; // in ms
unsigned long laststep; // ms since last step
int servostate;

const int gopin = 12;

const int phase1 = 7;
const int phase2 = 8;
const int phase3 = 9;

// potentiometer and the limit switches
// pot is on an analog pin, the switches on dig. pins
const int potpin = 5;
// pot readings at +/- 90 degrees
int neg90 = 207;
int pos90 = 793;
const int limleft = 2;
const int limright = 4;
int lastpos, targetpos; // angles

#define CMDLEN 33
char cmdbuff[CMDLEN];
int cmd_i;

void setup(){
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);
  servo(LOW, LOW, LOW);

  laststep = millis();
  servostate = 0;

  init_din(gopin);
  init_din(limleft);
  init_din(limright);

  // TODO tie A5 here to potpin
  pinMode(A5, INPUT);
  digitalWrite(A5, LOW);

  lastpos = targetpos = angle();

  for(cmd_i = 0; cmd_i < CMDLEN; cmd_i++){
    cmdbuff[cmd_i] = '\0';
  }
  cmd_i = 0;

  Serial.begin(115200);
  Serial.println("Alive.");
}

void init_din(int pin){
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
}

void servo(uint8_t ph1, uint8_t ph2, uint8_t ph3){
  digitalWrite(phase1, ph1);
  digitalWrite(phase2, ph2);
  digitalWrite(phase3, ph3);
}

void loop(){
  // read in the command, one char at a time
  while((Serial.available() > 0) && (cmd_i < CMDLEN)){
    cmdbuff[cmd_i++] = (char)Serial.read();
    if(cmdbuff[cmd_i - 1] == ';'){
      // received a whole phrase
      cmdbuff[cmd_i] = '\0';
      runcmd(cmdbuff);
      cmd_i = 0;
    }
  }

  // skip if we haven't had time to settle from the last step
  if(millis() - laststep < stepdelay)
    return;
  // or just shouldn't go anywhere
  if(digitalRead(gopin) == HIGH)
    return;

  int pos = angle();
  if(abs(pos - targetpos) >= 2){
    servostate = step(servostate, targetpos - pos);
    laststep = millis();
  }
  else if(millis() - laststep > 200){
    // give the setup 0.2s to absorb the momentum, then turn off the servo so
    // that we're not driving it too much.
    servo(LOW, LOW, LOW);
  }
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
  Serial.println("Move to +90 degrees and type OK;");
  if(wait_ok() != 0){
    Serial.println("Calibration canceled.");
    return;
  }
  int temp_pos90 = analogRead(potpin);
  Serial.println("Move to -90 degrees and type OK;");
  if(wait_ok() != 0){
    Serial.println("Calibration canceled.");
    return;
  }
  neg90 = analogRead(potpin);
  pos90 = temp_pos90;
  // don't want to turn back to wherever we were before the cal started
  targetpos = -90;
  Serial.println("Calibration complete.");
  return;
}

int wait_ok(){
  // clear out the serial buffer first
  while(Serial.available() > 0){
    Serial.read();
  }
  char upperok[4] = "OK;";
  char lowerok[4] = "ok;";
  int i = 0;
  while(i < 3){
    if(Serial.available() > 0){
      char c = Serial.read();
      if(c != upperok[i] && c != lowerok[i])
        return(-1);
      i++;
    }
  }
  return(0);
}

void read_angle(){
  int ang = angle();
  //Serial.print("Angle: ");
  Serial.println(ang);
}

void set_angle(String *cmd){
  if((*cmd).indexOf(' ') < 0)
    ser_error(cmd);

  String moveto = (*cmd).substring((*cmd).indexOf(' ') + 1);
  char buff[18];
  moveto.toCharArray(buff, 18);

  targetpos = atoi(buff);
}

void ser_error(String *cmd){
  char bad_cmd[CMDLEN];
  (*cmd).toCharArray(bad_cmd, CMDLEN);
  Serial.print("Unrecognized command: ");
  Serial.println(bad_cmd);
}

int angle(){
  int pot = analogRead(potpin);
  return((((long)(pot - neg90) * 180) / (pos90 - neg90)) - 90);
}

int step(int from, int dir){
  if(dir > 0 && digitalRead(limright) == LOW){
    return(stepto(from - 1));
  }
  else if(dir < 0 && digitalRead(limleft) == LOW){
    return(stepto(from + 1));
  }
  else if(dir != 0){
    // we've hit a limiter switch
    targetpos = angle();
    Serial.println("Limited!");
  }
  return(from);
}

int stepto(int pos){
  // keep pos limited to [0,5]
  if(pos < 0)
    pos = 6 - (abs(pos) % 6);
  pos = pos % 6;

  switch(pos){
    case(0):
      servo(LOW, LOW, HIGH);
      break;
    case(1):
      servo(LOW, HIGH, HIGH);
      break;
    case(2):
      servo(LOW, HIGH, LOW);
      break;
    case(3):
      servo(HIGH, HIGH, LOW);
      break;
    case(4):
      servo(HIGH, LOW, HIGH);
      break;
    case(5):
      servo(HIGH, LOW, HIGH);
      break;
    default:
      servo(LOW, LOW, LOW);
  }
  return(pos);
}
