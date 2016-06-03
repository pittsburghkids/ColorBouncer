#define MY_ADDR '4'
#define MASTER_ADDR 'M'

/////////////////////////////////////////////////////////////
// RS485 happens over RJ45 ethernet style cables/connectors
// Pinout:
// 1 - RS485 "A"
// 2 - GND
// 3 - N/C
// 4 - +5V
// 5 - GND
// 6 - N/C
// 7 - RS485 "B"
// 8 - GND
/////////////////////////////////////////////////////////////


// recognized commands:
// (UPDATE IS_VALID() WHEN ADDING TO THIS LIST)
#define GET_CURRENT_READING 'R'
#define GET_LAST_READ 'L'
#define DUMP_READING 'D'
#define RESET_BALL_PRESENCE_THRESH 'T'
#define RESET_BUCKET_COLOR 'B'
#define KIDS_GONE_CHECK 'K'

//responses
#define YES 'Y'
#define NO 'N'
#define OK 'K'
#define ERROR 'E'
#define NONE 0

#define txPin 2

void setup_rs485_tranceiver(){
  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, LOW); // set transceiver to "receive" mode
  
  char buffer[100]; //buffer to hold message before sending out
  PString message(buffer, sizeof(buffer));
  message.begin(); // clears out string
  message.print("sensor ");
  message.print(MY_ADDR);
  message.println(" started");
  tx_msg(buffer);
}

void communicate(){
  char command= get_command();
  if (command==NONE) return;
  
  char response[3]; // leave room for null termination
  response[0]=MASTER_ADDR;
  response[1]=ERROR;
  response[2]='\0'; // make sure ends with null
  
  if (command==GET_CURRENT_READING){
    response[1]= current_color;
    tx_msg(response);
  }
  
  if (command==GET_LAST_READ){
    response[1]= last_color;
    tx_msg(response);
  }  
  if (command==RESET_BALL_PRESENCE_THRESH){
    response[1]=OK;
    tx_msg(response); // ambient_light() takes too much time to wait...
    // set_ball_presence_thresh();
  } 
  if (command==RESET_BUCKET_COLOR){
    response[1]=OK;
    tx_msg(response);
    reset_bucket_color();
  }
  
  if (command==KIDS_GONE_CHECK){
    response[1] = NO;
    tx_msg(response);
  } 
}

void reset_bucket_color(){
  if (MY_ADDR == '1') set_current_color('n');
  if (MY_ADDR == '2') set_current_color('r');
  if (MY_ADDR == '3') set_current_color('g');
  if (MY_ADDR == '4') set_current_color('b');
}


char get_command(){ // returns next command or 0 if none available.
  char command;
  if (Serial.available() < 2) return NONE;
  
  while (Serial.available() > 0) if (Serial.read() == MY_ADDR) command= Serial.read();
  //while (Serial.read() != MY_ADDR) if (Serial.available()==0) return 0;
  //command= Serial.read();
  Serial.flush();
 
  if (is_valid(command)) return command;
  else return ERROR;
}

boolean is_valid(char candidate){
  if (candidate==GET_CURRENT_READING) return true;
  if (candidate==GET_LAST_READ) return true;
  if (candidate==DUMP_READING) return true;
  if (candidate==RESET_BALL_PRESENCE_THRESH) return true;
  if (candidate==RESET_BUCKET_COLOR) return true;
  if (candidate==KIDS_GONE_CHECK) return true;

  return false;
}

void tx_msg(char *msg)
{
  digitalWrite(txPin, HIGH);
  delay(10);
  Serial.print(msg);
  delay(10);
  digitalWrite(txPin, LOW);
}

