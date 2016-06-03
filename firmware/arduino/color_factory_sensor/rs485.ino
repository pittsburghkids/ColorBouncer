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
//
// transmission lines are fail-safe biased at the RJ-45 jack
// located in the back of the controller box. (see RS-485 application note)
//
/////////////////////////////////////////////////////////////


// recognized commands:
// (UPDATE IS_VALID() WHEN ADDING TO THIS LIST)
#define GET_CURRENT_READING 'R'
#define DUMP_READING 'D'
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
  
//  char buffer[100]; //buffer to hold message before sending out
//  PString message(buffer, sizeof(buffer));
//  message.begin(); // clears out string
//  message.print("sensor ");
//  message.print(address);
//  message.println(" started");
//  tx_msg(buffer);
}

void communicate(){
  char command= get_command();
  if (command==NONE) return;
  
  char response[3]; // leave room for null termination
  response[0]=MASTER_ADDR;
  response[1]=ERROR;
  response[2]='\0'; // make sure ends with null
  
  delay(1); // make sure master transceiver is in receive mode
  
  if (command == GET_CURRENT_READING){
    response[1]= current_color;
    tx_msg(response);
  }
  
  if (command == DUMP_READING) dump_reading();
  
  if (command == RESET_BUCKET_COLOR){
    response[1]=OK;
    tx_msg(response);
    reset_bucket_color();
  }
  
  if (command == KIDS_GONE_CHECK){
    if (millis() > last_ball_time+ 60000) response[1]=YES;
    else response[1] = NO;
    tx_msg(response);
  } 
}

char get_command(){ // returns next command or 0 if none available.
  char command;
  while(Serial.available()>0){
    if(Serial.read()== address){
      if(Serial.available()==0) delay(10);
      command= Serial.read();
      while(Serial.available()>0) Serial.read(); // clear buffer
    }
  }
  if (is_valid(command)) return command;
  else return ERROR;
}

boolean is_valid(char candidate){
  if (candidate==GET_CURRENT_READING) return true;
  if (candidate==DUMP_READING) return true;
  if (candidate==RESET_BUCKET_COLOR) return true;
  if (candidate==KIDS_GONE_CHECK) return true;

  return false;
}

void tx_msg(char *msg)
{
  digitalWrite(txPin, HIGH);
  delayMicroseconds(10);
  Serial.print(msg);
  Serial.flush(); // wait for characters to print
  digitalWrite(txPin, LOW);
}

