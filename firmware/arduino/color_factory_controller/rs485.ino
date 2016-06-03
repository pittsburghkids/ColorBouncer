// see sensor code for notes

#define txPin A4

void setup_rs485_tranceiver(){
  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, LOW); // set transceiver to "receive" mode
}

void tx_msg(char *msg){
  digitalWrite(txPin, HIGH); // transmit mode
  delayMicroseconds(10); // make sure mode is set
  Serial.print(msg);
  Serial.flush(); // wait for msg to leave buffer
  digitalWrite(txPin, LOW); // back to recieve mode
}

