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

#define txPin 2

void setup_rs485_tranceiver(){
  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, LOW); // set transceiver to "receive" mode
}

void tx_msg(char *msg){
  digitalWrite(txPin, HIGH);
  delayMicroseconds(10);
  Serial.print(msg);
  Serial.flush();
  digitalWrite(txPin, LOW);
}

