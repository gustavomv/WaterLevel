#include <PubSubClient.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#define MQTT_SERVER "test.mosca.io"
char ssid[] = "Livre";
char passwd[] = "livre2017";

char* lightTopic = "/rexproject/esp/02";

void callback(char* topic, byte* payload, unsigned int length);

WiFiEspClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

SoftwareSerial Serial1(13, 12);
Servo motor;

//sensores
#define echo 2
#define echo0 4
#define trig 5

//button_rst
#define rst 11
#define button_rst 10
//leds
#define led_r 7
#define led_g 8
//motor
int mtr = 6;
//buzzer
#define buz 9



int _reset = 1;
float raio, lado_a, lado_b, volume, maximo;

int form;


void reset();
void calculate();
void button();
float lvl();
void triPulse(const int a);
void move_motor();
void alarme();


void setup(){
  Serial.begin(9600);
  delay(100);
  Serial1.begin(9600);

  WiFi.init(&Serial1);
  //start wifi subsystem
  WiFi.begin(ssid, passwd);
  //attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();

  //wait a bit before starting the main loop
  delay(2000);



  
  digitalWrite(rst, HIGH);
  motor.attach(mtr);
  motor.write(0);

  pinMode(echo0, INPUT);
  pinMode(echo, INPUT);
  pinMode(trig,OUTPUT);
  pinMode(led_r, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(buz, OUTPUT);
  pinMode(rst, OUTPUT);
  pinMode(button_rst, INPUT);

  Serial.begin(9600);
  }
void loop(){
  if (!client.connected() && WiFi.status() == 3) {reconnect();} 
  client.loop();
  
  if (_reset == 1){
    reset();
    Serial.println("Iniciando...");
  }

  calculate();
  
  button();
  
  alarme();
  
Serial.println(lvl(1));
  
delay(1000);
}

void reset(){
digitalWrite(rst, LOW);
    motor.write(0);
  while(1){
    if (Serial.available() > 0){
      form = Serial.read() - 48;
      Serial.println(form);
      if (form == 0){
        raio = lvl(0);
        delay(1000);
        calculate();
        maximo = volume;
      }else if (form == 1) {
        lado_a = lvl(0);
        delay(1000);
        move_motor();
        calculate();
        maximo = volume;
      }
      _reset = 0;
      break;
    }
  }
}
void calculate(){
  if (form == 0){
    volume = 3.1415*(raio*raio)*lvl(1);
  }else if(form == 1){
    volume = (lado_b*lado_a)*lvl(1);
  }
Serial.println("calculo");
}

void button(){
  _reset = digitalRead(button_rst);
}

float lvl(int sensor){
  long pulse;
  float ret = 0.0;
  if (sensor == 0){
    trigPulse();
    pulse = pulseIn(echo0, HIGH);
    Serial.print("0:");
    Serial.println(pulse);
  }else if (sensor == 1){
    trigPulse();
    pulse = pulseIn(echo, HIGH);
    Serial.print("1:");
    Serial.println(pulse);
  }
    
    return microsecondsToCentimeters(pulse);
}
float microsecondsToCentimeters(long microseconds)
{
  return microseconds/29.0/2.0;
}
void move_motor(){
  int pos;
  for (pos = 0; pos <= 90; pos++) {
    motor.write(pos);
    delay(10);
  }
  lado_b = lvl(0);
  delay(1000);
  for (pos = 90; pos >= 0; pos--) { // goes from 180 degrees to 0 degrees
    motor.write(pos);              // tell servo to go to position in variable 'pos'
    delay(10);                       // waits 15ms for the servo to reach the position
  }
}
void trigPulse()
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);     //duração de 10 micro segundos
  digitalWrite(trig, HIGH);   //Pulso de trigge em nível baixo
  delayMicroseconds(5);
  digitalWrite(trig, LOW);
}
void alarme(){
int i = 0;
  if (volume > maximo*0.75 || volume < maximo*0.25){
    for(i = 0; i<10; i++){
      digitalWrite(led_g, LOW);
      digitalWrite(led_r, HIGH);
      tone(buz, 1000);
      delay(10);
      digitalWrite(led_r, LOW);
      noTone(buz);
    }
    digitalWrite(led_g, HIGH);
    noTone(buz);
  }
  Serial.println("Alarme");
}

void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 

  //Print out some debugging info
  Serial.println("\nCallback update.");
  Serial.print("Topic: ");
  Serial.print(topicStr);
  Serial.print(":");

  for(int i = 0; i < length; i++){
    Serial.print((char*)payload);
  }

  //turn the light on if the payload is '1' and publish to the MQTT server a confirmation message
  if(payload[0] == '1'){
    client.publish("/rexproject/esp/01", "Paralelepípedo");

  }

  //turn the light off if the payload is '0' and publish to the MQTT server a confirmation message
  else if (payload[0] == '0'){
    client.publish("/rexproject/esp/01", "Cilindro");
  }

}

void reconnect() {

  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "esp8266-";
      
      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str())) {
        Serial.print("\tMTQQ Connected");
        client.subscribe(lightTopic);
      }

      //otherwise print failed for debugging
      else{Serial.println("\tFailed."); abort();}
    }
  }
}
