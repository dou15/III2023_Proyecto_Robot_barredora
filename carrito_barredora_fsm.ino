#include <stdio.h>
// includes necesarias para la comunicación de arduino con thinksboard
#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include "SoftwareSerial.h"
#include <ThingsBoard.h>

#define WIFI_AP "YOUR_WIFI_AP"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define TOKEN "YOUR_ACCESS_TOKEN"

char thingsboardServer[] = "YOUR_THINGSBOARD_HOST_OR_IP";

SoftwareSerial soft(13, 12); // RX, TX

int status = WL_IDLE_STATUS;
unsigned long lastSend;

/*** Pines Conexiones ***/

// Motor A
int ENA = 9;
int IN1 = 8;
int IN2 = 7;

// Motor B
int ENB = 3;
int IN3 = 4;
int IN4 = 2;

//Sensor proximidad frontral
const int trig01 = 5;
const int echo01 = 15;

//Sensor proximidad derecho
int trig02 = 6;
int echo02 = 16;

//Sensor proximidad izquierdo
int trig03 = 10;
int echo03 = 14;
/**************************************************/

/*** Variables ***/

//Variables calculo distancia objetos
long duracion01;
int distancia01;

long duracion02;
int distancia02;

long duracion03;
int distancia03;

//Variables calculo area barrida
unsigned long tiempoBarrido = 0;
unsigned long tiempoAhora   = 0;
unsigned long tiempoAntes   = 0;
unsigned long timepoTranscurrido = 0;

unsigned long areaBarrida   = 0;
/**************************************************/

//
struct state;
typedef void state_fn(struct state *);

struct state {
  state_fn *next;
};

state_fn avanzar, retroceder, izquierda, derecha, alto;

void avanzar(struct state *state) {
  //Serial.print("Estado actual :avanzar\n");
  while(distancia01>20){
    proximidadObjeto();
    moverAdelante();
  }
  detener();
  state->next = alto;
}

void alto(struct state *state) {
  //Serial.print("Estado actual :alto\n");
  proximidadObjeto();
  if(distancia02>20){
    state->next = derecha;
  }
  if(distancia02<20 && distancia03>20){
    state->next = izquierda;
  }
  if(distancia02<20 && distancia03<20){
    state->next = retroceder;
  }
}

void derecha(struct state *state) {
  //Serial.print("Estado actual :derecha\n");
  giroDerecha();
  delay(1000);
  detener();
  state->next = avanzar;
}

void izquierda(struct state *state) {
  //Serial.print("Estado actual :izquierda\n");
  giroIzquierda();
  delay(1000);
  detener();
  state->next = avanzar;
}

void retroceder(struct state *state) {
  //Serial.print("Estado actual :retroceder\n");
  moverAtras();
  delay(2000);
  detener();
  proximidadObjeto();
  if(distancia02>20){
    giroDerecha();
    delay(1000);
    detener();
    state->next = avanzar;
  }
  if(distancia02<20 && distancia03>20){
    giroIzquierda();
    delay(1000);
    detener();
    state->next = avanzar;
  }
  if(distancia02<20 && distancia03<20){
    moverAtras();
    delay(2000);
    detener();
    proximidadObjeto();
    if(distancia02>20){
      giroDerecha();
      delay(1000);
      detener();
      state->next = avanzar;
    }
    if(distancia02<20 && distancia03>20){
    giroIzquierda();
    delay(1000);
    detener();
    state->next = avanzar;
    }
    if(distancia02<20 && distancia03<20){
      moverAtras();
      delay(2000);
      detener();
      state->next = derecha;
    }
  }
}


/**************************************************/
void setup(){
 // Declaración pines motores
 pinMode(ENA, OUTPUT);
 pinMode(ENB, OUTPUT);
 pinMode(IN1, OUTPUT);
 pinMode(IN2, OUTPUT);
 pinMode(IN3, OUTPUT);
 pinMode(IN4, OUTPUT);

 // Declaración pines sensores proximidad
 pinMode(trig01, OUTPUT); 
 pinMode(echo01, INPUT);
 pinMode(trig02, OUTPUT); 
 pinMode(echo02, INPUT);
 pinMode(trig03, OUTPUT); 
 pinMode(echo03, INPUT); 

 //Baud rate transmisición serial
 Serial.begin(115200);
 InitWiFi();
 lastSend = 0;
}
/**************************************************/

/*** Funciones control motores ***/
void moverAdelante(){

 //Motor A
 digitalWrite(IN1, LOW);
 digitalWrite(IN2, HIGH);
 analogWrite(ENA, 255); //Motor A 100% velocidad
 //Motor B
 digitalWrite(IN3, LOW);
 digitalWrite(IN4, HIGH);
 analogWrite(ENB, 255); //Motor B 100% velocidad
}

void moverAtras(){
 //Motor A
 digitalWrite(IN1, HIGH);
 digitalWrite(IN2, LOW);
 analogWrite(ENA, 255); //Motor A 100% velocidad
 //Motor B
 digitalWrite(IN3, HIGH);
 digitalWrite(IN4, LOW);
 analogWrite(ENB, 255); //Motor B 100% velocidad
}

void giroIzquierda(){
 //Motor A
 digitalWrite(IN1, LOW);
 digitalWrite(IN2, HIGH);
 analogWrite(ENA, 255); //Motor A 100% velocidad
 
 //Motor B
 digitalWrite(IN3, LOW);
 digitalWrite(IN4, LOW);
 analogWrite(ENB, 0); //Motor B 0% velocidad
}

void giroDerecha(){
 //Motor A
 digitalWrite(IN1, LOW);
 digitalWrite(IN2, LOW);
 analogWrite(ENA, 0); //Motor A 0% velocidad
 //Motor B
 digitalWrite(IN3, LOW);
 digitalWrite(IN4, HIGH);
 analogWrite(ENB, 255); //Motor B 100% velocidad
}

void detener(){
 //Motor A
 digitalWrite (IN1, LOW);
 digitalWrite (IN2, LOW);
 analogWrite (ENA, 0); //Motor A 0% velocidad
 //Motor B
 digitalWrite (IN3, LOW);
 digitalWrite (IN4, LOW);
 analogWrite (ENB, 0); //Motor B 0% velocidad
}
/**************************************************/


void proximidadObjeto(){
   //Captura sensor distancia01
   digitalWrite(trig01, LOW); //Limpiar trigger pin
   delayMicroseconds(2);
   digitalWrite(trig01, HIGH); //trigger en alto por 10 micro segundos
   delayMicroseconds(10);
   digitalWrite(trig01, LOW);
   duracion01 = pulseIn(echo01, HIGH); // Lectura onda de sonido pin echo, unidades microsegundos
   distancia01 = duracion01*0.034/2; // Calculo de la distancia
  
   //Captura sensor distancia02
   digitalWrite(trig02, LOW);
   delayMicroseconds(2);
   digitalWrite(trig02, HIGH);
   delayMicroseconds(10);
   digitalWrite(trig02, LOW);
   duracion02 = pulseIn(echo02, HIGH);
   distancia02 = duracion02*0.034/2;

   //Captura sensor distancia03
   digitalWrite(trig03, LOW);
   delayMicroseconds(2);
   digitalWrite(trig03, HIGH);
   delayMicroseconds(10);
   digitalWrite(trig03, LOW);
   duracion03 = pulseIn(echo03, HIGH);
   distancia03 = duracion03*0.034/2;

  // Prints the distance on the Serial Monitor
  /*Serial.print("distancia01 ");
  Serial.print(String(distancia01));
  Serial.println(" cm \n");
  Serial.print("distancia02 ");
  Serial.print(String(distancia02));
  Serial.println(" cm \n");
  Serial.print("distancia03 ");
  Serial.print(String(distancia03));
  Serial.println(" cm \n");*/

  delay(400);
}
/**************************************************/

/*El carrito mide 20cmx15cm (300cm^2). En 4 segundos recorre 1 m lineal,
  1500 cm^2 (0.15 m^2) de área. 
*/
void areaRecorrida(){
  tiempoAhora = millis();
  timepoTranscurrido = tiempoAhora - tiempoAntes;
  tiempoBarrido += timepoTranscurrido;
  tiempoBarrido = tiempoBarrido / 1000;

  areaBarrida = tiempoBarrido/4;
  areaBarrida *=0.15; //En metros cuadrados
  Serial.println("areaBarrida ");
  Serial.println(areaBarrida);
}

/**************************************************/
//Funciones transisión wifi

void InitWiFi()
{
  // initialize serial for ESP module
  soft.begin(9600);
  // initialize ESP module
  WiFi.init(&soft);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  while (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if ( tb.connect(thingsboardServer, TOKEN) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( " : retrying in 5 seconds" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

void transmisionDatos() {
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to WPA SSID: ");
      Serial.println(WIFI_AP);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("Connected to AP");
  }

  if ( !tb.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after 1 seconds
    areaRecorrida(); 
    lastSend = millis();
  }

  tb.loop();
}


/**************************************************/

void loop(){

 /*** Aquí ba el loop***/
 proximidadObjeto();
 //moverAdelante();
 //moverAtras();
 //giroDerecha();
 ///giroIzquierda();
 //delay(1000);
 //detener();
 //delay(2000);

  //llama fsm
  struct state state = {avanzar};
  while (state.next) state.next(&state);

  //calcula area barrida
  //areaRecorrida(); 

}
