#include <PubSubClient.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <Servo.h>

#define MQTT_SERVER "test.mosca.io"
char ssid[] = "rex";
char passwd[] = "12345678";

char* FormTopic = "/rexproject/esp/01/recipiente";

void callback(char* topic, byte* payload, unsigned int length);

WiFiEspClient wifiClient;
PubSubClient client(wifiClient);

SoftwareSerial Serial1(12, 13);

Servo motor;

//sensores
#define echo 2
#define echo0 4
#define trig 3
//leds
#define led_wifi 5
#define led_r 7
#define led_g 8
//motor
int mtr = 6;
//buzzer
#define buz 9



int _reset = 1;
float raio, lado_a, lado_b, volume, maximo;
long anterior = 0;
int form;
boolean first_config = false;


void reset();
void calculate();
void button();
float lvl(int n);
void triPulse(const int a);
void move_motor();
void alarme();

void reconectar() {
  while (!client.connected()) {
    Serial.println("Conectando ao Broker MQTT.");
    if (client.connect("ESP8266Client")) {
      Serial.println("Conectado com Sucesso ao Broker");
      client.subscribe(FormTopic);
    } else {
      Serial.print("Falha ao Conectador, rc=");
      Serial.print(client.state());
      Serial.println(" tentando se reconectar... Aguarde 5 segundos");
      delay(5000);
    }
  }
}

void setupWIFI() {
  WiFi.begin(ssid, passwd);
  Serial.print("Conectando na rede: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
   Serial.print(".");
   delay(500);
  }
}

void setup(){
   Serial.begin(9600);
   delay(100);
  Serial1.begin(9600);

  WiFi.init(&Serial1);
  setupWIFI();

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  
  motor.attach(mtr);
  motor.write(0);

  pinMode(echo0, INPUT);
  pinMode(echo, INPUT);
  pinMode(trig,OUTPUT);
  pinMode(led_r, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(buz, OUTPUT);
  pinMode(led_wifi, OUTPUT);
  digitalWrite(trig, LOW);

  digitalWrite(led_g, HIGH);
  }
  
void loop(){
  if (!client.connected()) {
    digitalWrite(led_wifi, LOW);
    reconectar();
  }
  digitalWrite(led_wifi, HIGH);
  if(!client.loop())
    client.connect("ESP8266Client");

  if (first_config){
  
  if (_reset == 1){
    reset();
    Serial.println("Iniciando...");
  }

  calculate();
  alarme();
  
Serial.print("Volume: ");
Serial.println(volume);

Serial.print("Altura: ");
Serial.println(lvl(1));

if (millis() - anterior > 1000){
  Serial.print("Mensagem a ser Puplicada: ");
  Serial.println(String(volume).c_str());
  client.publish("/rexproject/esp/01/volume", String(volume).c_str());
  delay(500);
  Serial.print("Mensagem a ser Puplicada: ");
  Serial.println(String(lvl(1)).c_str());
  client.publish("/rexproject/esp/01/altura", String(lvl(1)).c_str());
  anterior = millis();
}

}

delay(100);
digitalWrite(led_wifi, LOW);
}

void reset(){
  motor.write(0);
  delay(200);
  float l = lvl(0);
  Serial.print("Form: ");
  Serial.println(form);
  while(1){
     if (form == 1){
        raio = l;
        //raio = 10;
        delay(1000);
        calculate();
        maximo = volume;
        _reset = 0;
        break;
      }else if (form == 0) {
        lado_a = l;
        delay(1000);
        move_motor();
        calculate();
        maximo = volume;
        _reset = 0;
        break;
      }else if (form == 2){
        lado_a = lvl(0);
        delay(1000);
        lado_b = lado_a;
        calculate();
        maximo = volume;
        _reset = 0;
        break;
        }
    }
  
  tone(buz, 1000);
  delay(2000);
  noTone(buz);
}
void calculate(){
  float h = lvl(1);
  if (form == 1){
      volume = 3.1415*(raio*raio)*h;
  }else if(form == 0 || form == 2
  ){
      volume = (lado_b*lado_a)*h;
  }
}

float lvl(int sensor){
  long pulse;
  float ret = 0.0;
  if (sensor == 0){
    trigPulse(trig);
    pulse = pulseIn(echo0, HIGH);
  }else if (sensor == 1){
    trigPulse(trig);
    pulse = pulseIn(echo, HIGH);
  }
    
    return microsecondsToCentimeters(pulse);
}
float microsecondsToCentimeters(long microseconds)
{
  return microseconds/58.82;
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
void trigPulse(const int a)
{
  digitalWrite(a, HIGH);   //Pulso de trigge em nível baixo
  delayMicroseconds(10);
  digitalWrite(a, LOW);
}
void alarme(){
int i = 0;
  if (volume > maximo*0.75 || volume < maximo*0.25){
    for(i = 0; i<10; i++){
      digitalWrite(led_g, LOW);
      digitalWrite(led_r, HIGH);
      tone(buz, 1000);
      delay(100);
      digitalWrite(led_r, LOW);
      noTone(buz);
    }
    digitalWrite(led_g, HIGH);
    noTone(buz);
  }
  }

 void callback(char* topic, byte* payload, unsigned int length) {
switch (payload[0]){
    case '0':
    form = 0;
    first_config = true;
    break;

    case '1':
    form = 1;
    first_config = true;
    break;

    case '2':
    form = 2;
    first_config = true;
    break;
 }
 }
