int stepdelay = 5; // in ms
unsigned long laststep; // ms since last step
int servostate;
int alerted = 1;
int badcmdlen = 0;

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
  // initialize a digital input pin
  pinMode(pin, INPUT);
  digitalWrite(pin, HIGH);
}

void servo(uint8_t ph1, uint8_t ph2, uint8_t ph3){
  // servo control
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
  if(cmd_i == CMDLEN){
    cmd_i = 0;
    // signal so that we can send len error
    badcmdlen = 1;
  }
  // TODO deal with command when it is longer than CMDLEN

  // skip if we haven't had time to settle from the last step
  if(millis() - laststep < stepdelay)
    return;
  // or just shouldn't go anywhere
  if(digitalRead(gopin) == HIGH)
    return;

  int pos = angle();
  if(abs(pos - targetpos) >= 2){
    // accurate to +/- 1 degree, (+/- whatever the pot accuracy is)
    servostate = step(servostate, targetpos - pos);
    laststep = millis();
  }
  else if(millis() - laststep > 200){
    // give the setup 0.2s to absorb the momentum, then turn off the servo so
    // that we're not driving it too much.
    servo(LOW, LOW, LOW);
    // if we haven't notified the user yet, let them know where we've turned
    // to, and wait for them to read that data before doing anything more
    if(alerted == 0){
      Serial.println(angle());
      Serial.flush();
      alerted = 1;
    }
  }
}

void runcmd(char *cmd){
  String cmdstr = String(cmd);
  // case-sensitive commands are a pain
  cmdstr.toUpperCase();
  cmdstr.trim();

  if (badcmdlen != 0){
    ser_too_long();
  }
  else if(cmdstr.startsWith("CAL")){
    calibrate();
  }
  else if(cmdstr.startsWith("GET")){
    read_angle();
  }
  else if(cmdstr.startsWith("TURN")){
    set_angle(&cmdstr);
  }
  else if(cmdstr.startsWith("SET")){
    set_vals(&cmdstr);
  }
  else{
    ser_error(&cmdstr);
  }
}

void calibrate(){
  Serial.println("Move to +90 degrees and type OK;");
  if(wait_ok() != 0){
    Serial.println("Calibration canceled.");
    Serial.flush();
    return;
  }
  // don't want to save it right away, in case the user cancels out
  int temp_pos90 = analogRead(potpin);
  Serial.println("Move to -90 degrees and type OK;");
  if(wait_ok() != 0){
    Serial.println("Calibration canceled.");
    Serial.flush();
    return;
  }
  neg90 = analogRead(potpin);
  pos90 = temp_pos90;
  // don't want to turn back to wherever we were before the cal started
  targetpos = -90;
  Serial.println(neg90);
  Serial.println(pos90);
  // make sure the user reads the values before we move on
  Serial.flush();
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

void set_vals(String *cmd){
  // these have the form "SET SUBCOMMAND value;"
  int cmd_end = (*cmd).indexOf(' ');
  if(cmd_end < 0)
    // no subcommand
    ser_error(cmd);
  int sub_end = (*cmd).indexOf(' ', cmd_end + 1);
  if(sub_end <= 0)
    // no value after the subcommand
    ser_error(cmd);

  String sub = (*cmd).substring(cmd_end + 1, sub_end);
  sub.toUpperCase();
  char buff[8];
  (*cmd).substring(sub_end + 1).toCharArray(buff, 8);
  int val = atoi(buff);

  if(sub.equals("POS"))
    pos90 = val;
  else if(sub.equals("NEG"))
    neg90 = val;
  else if(sub.equals("DELAY"))
    stepdelay = val;
  else
    ser_error(cmd);
}

void read_angle(){
  Serial.println(angle());
}

void set_angle(String *cmd){
  if((*cmd).indexOf(' ') < 0)
    // didn't give us an angle to set
    ser_error(cmd);

  String moveto = (*cmd).substring((*cmd).indexOf(' ') + 1);
  char buff[18];
  moveto.toCharArray(buff, 18);

  targetpos = atoi(buff);
  // clear alerted so that we will tell the user when it's done moving
  alerted = 0;
}

void ser_error(String *cmd){
  // TODO send a special code, so that the user can know to clear the buffer
  // right out?
  char bad_cmd[CMDLEN];
  (*cmd).toCharArray(bad_cmd, CMDLEN);
  Serial.print("Unrecognized command: ");
  Serial.println(bad_cmd);
}

void ser_too_long(){
  Serial.println("Commands cannot be longer than 32 characters.");
  badcmdlen = 0;
}

int angle(){
  // read the pot value, and calculate the angle that represents
  int pot = analogRead(potpin);
  return((((long)(pot - neg90) * 180) / (pos90 - neg90)) - 90);
}

int step(int from, int dir){
  // from is the current servostate
  if(dir > 0 && digitalRead(limright) == LOW){
    return(stepto(from - 1));
  }
  else if(dir < 0 && digitalRead(limleft) == LOW){
    return(stepto(from + 1));
  }
  else if(dir != 0){
    // we've hit a limiter switch
    targetpos = angle();
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
      // really should not be able to happen
      servo(LOW, LOW, LOW);
  }
  return(pos);
}
