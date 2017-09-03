#include <PubSubClient.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <Servo.h>

#define MQTT_SERVER   "test.mosca.io"
#define FormTopic     "/rexproject/esp/01/recipiente"
#define ResponseTopic "/rexproject/esp/01/response"
void callback(char* topic, byte* payload, unsigned int length);

char ssid[] = "MiniRex";
char passwd[] = "rexproject0684";

WiFiEspClient wifiClient;
PubSubClient client(wifiClient);

SoftwareSerial Serial1(12, 13);
Servo motor;

//sensores
#define Echo_S0   4 //Sensor 0 - Comprimento, Largura, Raio
#define Echo_S1   2 //Sensor 1 - Altura
#define Trig      3

//leds
#define Led_Wifi  5
#define LedRed    7
#define LedGreen  8
//buzzer
#define Buzzer    9

//motor
#define ServMtr   6

float Volume, Maximo, Lado_B;
long Anterior = 0;
int Form;
boolean First_Config = false;

void Calculate();
float WaterLevel(int n);
void TrigPulse(const int a);
void MovServ();
void Alarme();

void setupWIFI() {
  WiFi.begin(ssid, passwd);
  if (WiFi.status() != WL_CONNECTED) {
    setupWIFI();
  }
}

void Revive() {
  while (!client.connected()) {
    if (client.connect("ESP8266")) {
      client.subscribe(FormTopic);
    } else {
      delay(5000);
    }
  }
}

void setup(){
  Serial1.begin(9600);

  WiFi.init(&Serial1);
  setupWIFI();
   
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  
  motor.attach(ServMtr);
  motor.write(3);

  pinMode(Echo_S0, INPUT);
  pinMode(Echo_S1, INPUT);
  pinMode(Trig,OUTPUT);
  
  pinMode(LedRed, OUTPUT);
  pinMode(LedGreen, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  
  pinMode(Led_Wifi, OUTPUT);

  digitalWrite(LedGreen, HIGH);
  digitalWrite(Trig, LOW);
}
  
void loop(){
  if (!client.connected()) {
    digitalWrite(Led_Wifi, LOW);
    Revive();
  }
  digitalWrite(Led_Wifi, HIGH);
    if(!client.loop())
      client.connect("ESP8266Client");

  if (First_Config){
    Calculate();
    Alarme();
    if (millis() - Anterior > 1000){
      client.publish("/rexproject/esp/01/volume", String(Volume).c_str());
      delay(500);
      client.publish("/rexproject/esp/01/altura", String(WaterLevel(1)).c_str());
      Anterior = millis();
    }
  }
  delay(100);
  digitalWrite(Led_Wifi, LOW);
}

void Calculate(){
  float H = WaterLevel(1);
  float Raio = WaterLevel(0);
  float Lado_A = Raio;

  switch (Form){
    case 0:
      MovServ();
      //Acrescentar diferenca das dimensoes da caixa pros sensores
      Volume = (Lado_A*Lado_B)*H;
      Maximo = Volume;
      break;

    case 1:
      //Acrescentar diferenca das dimensoes da caixa pros sensores
      Volume = 3.1415*(Raio*Raio)*H;
      Maximo = Volume;
      break;

    case 2:
      //Acrescentar diferenca das dimensoes da caixa pros sensores
      Volume = Lado_A*Lado_A*Lado_A;
      Maximo = Volume;
      break;
  }
}

float WaterLevel(int Sensor){
  long pulse;
  int i, j, aux, vetor[9];
  float ret = 0.0;

  switch (Sensor){
    case 0:
      TrigPulse(Trig);
      pulse = pulseIn(Echo_S0, HIGH);
      break;

    case 1:
      TrigPulse(Trig);
      pulse = pulseIn(Echo_S1, HIGH);
      break;
  }
  //recebe 9 analises no intervalo de 40 ms
    for( i = 0; i < 9; i++ ){
      vetor[i] = Distance_cm(pulse);
      delay(5);
    }
  //organiza o vetor em ordem crescente
    for( i = 0; i < 9; i++ ){
      for( j = i + 1; j < 9; j++ ){
        if ( vetor[i] > vetor[j] ){
          aux = vetor[i];
          vetor[i] = vetor[j];
          vetor[j] = aux;
        }
      }
    }
    ret = vetor[4]; //recebe a mediana
    return ret;   
}

float Distance_cm(long microseconds){
  return microseconds/58.82;
}

void MovServ(){
  int pos;
  for (pos = 3; pos <= 90; pos++) {
    motor.write(pos);
    delay(10);
  }
  Lado_B = WaterLevel(0);
  delay(1000);
  for (pos = 90; pos >= 3; pos--) { 
    motor.write(pos);              
    delay(10);                       
  }
}

void TrigPulse(int a){
  digitalWrite(a, HIGH);   
  delayMicroseconds(10);
  digitalWrite(a, LOW);
}

void Alarme(){
if (Volume > Maximo*0.75 || Volume < Maximo*0.25){
    for(int i = 0; i < 10; i++){
      digitalWrite(LedGreen, LOW);
      digitalWrite(LedRed, HIGH);
      tone(Buzzer, 1000);
      delay(100);
      digitalWrite(LedRed, LOW);
      noTone(Buzzer);
    }
    digitalWrite(LedGreen, HIGH);
    noTone(Buzzer);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  switch (payload[0]){
    case '0':
      Form = 0;
      client.publish(ResponseTopic, "OK");
      First_Config = true;
      tone(Buzzer, 1000);
      delay(2000);
      noTone(Buzzer);
      break;

    case '1':
      Form = 1;
      client.publish(ResponseTopic, "OK");
      First_Config = true;
      tone(Buzzer, 1000);
      delay(2000);
      noTone(Buzzer);
      break;
      
    case '2':
      Form = 2;
      client.publish(ResponseTopic, "OK");
      First_Config = true;
      tone(Buzzer, 1000);
      delay(2000);
      noTone(Buzzer);
      break;
  }
}
