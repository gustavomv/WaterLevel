#include <WiFiEsp.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial esp(6, 7); // RX, TX

//Define os pinos para o trigger e echo
#define trigger 5
#define echo 4
#define led 8
#define buzzer 10

float pulse;     //Variável que armazena o tempo de duração do echo
float dist_cm;   //Variável que armazena o valor da distância em centímetros

//Inicializa o sensor nos pinos definidos acima
char form;
int forma;
float a, b, distance, volume, maximo;

Servo myservo;
int pos = 3;

// Wifi Connection
const char* ssid = "........";
const char* password = "........";
// MQTT Server address
const char* mqtt_server = "test.mosca.io";

WiFiEspClient espClient;
PubSubClient client(espClient);

void start(){
  tone(buzzer, 1000,500);
  digitalWrite(led, HIGH);  
  delay(1000);
  digitalWrite(led, LOW);
  //form = Serial.read(); MQTT

  Serial.print("Lendo a");
  delay(2000);
  a = distancia();
  //a = 36.0; //adaptado ao do agostinho
  
  gira_motor();
  Serial.print("Lendo b");
  delay(2000);
  b = distancia();
  //b = 24.5; //adaptado ao do agostinho;
  
  maximo = teste(); //para uso no trabalho
  //maximo = 22050.00; //facilitando as coisas / em ml
}

void trigPulse()
{
  digitalWrite(trigger, HIGH);  //Pulso de trigger em nível alto
  delayMicroseconds(10);     //duração de 10 micro segundos
  digitalWrite(trigger, LOW);   //Pulso de trigge em nível baixo
}

void gira_motor(){
for (pos = 3; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }	
}

void alarme(){
  digitalWrite(led, 0);
  if(volume <= (maximo*0.15) || volume >= (maximo*0.85)){
    digitalWrite(led, 1);
    tone(buzzer, 1000, 200); 
  }
}

void setup_wifi() {
  delay(2);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("espClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/rexproject/recipiente/analise","hello world");
      // ... and resubscribe
      client.subscribe("/rexproject/esp/01");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float paralele_cubo(float x, float y, int z){
  float vol = x*y*z; 
  return vol;
  }
  
float tronco(float x, int z){
  float vol = 3.1415*(x*x)*z; 
  return vol;
  }

int distancia(){
    int i = 0;
    distance = 0.0;
    trigPulse();                 //Aciona o trigger do módulo ultrassônico
    pulse = pulseIn(echo, HIGH); //Mede o tempo em que o pino de echo fica em nível alto
    dist_cm = pulse/58.82;       //Valor da distância em centímetros
    
    while(i<5){
      distance = dist_cm + distance;
      delay(50);
      i++;
    }
    distance = distance/5.0;
  return distance;
  }
  
float teste(){
  float valor;
  if (forma == 3 || forma == 1)
    valor = paralele_cubo(a, b, distancia());
  else
    valor = tronco(a, distancia()); 

  return valor;
}
   
void setup(){
  Serial.begin(9600);
  esp.begin(9600);
  WiFi.init(&esp);
  
  pinMode(trigger, OUTPUT);   //Pino de trigger será saída digital
  pinMode(echo, INPUT);    //Pino de echo será entrada digital
  
  digitalWrite(trigger, LOW); //Saída trigger inicia em nível baixo

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo.write(pos);

  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  start();
  
  
}
 
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
    
    Serial.print("\nVolume(L): ");
    volume = maximo - teste();
    float show = volume/1000.0;
    Serial.print(show);
    alarme();
    delay(500);
    
 }
