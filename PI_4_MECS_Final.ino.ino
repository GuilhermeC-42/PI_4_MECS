/*
//  Projeto: PI_4_MECS
//  Autores: Enzo Chang Casceano
//           Guilherme Cordeiro Peixoto
//           Letícia Silva Melo
//  Data: 21/11/2021
//  
//  Objetivo: Integrar as leituras de um sensor de distância HC-SR04
*/

//============== Declaração de Bibliotecas ====================
#include <NTPClient.h>    // Biblioteca do NTP.
#include <WiFiUdp.h>      // Biblioteca do UDP.
#include <ESP8266WiFi.h>  // Biblioteca do WiFi.
#include <Ultrasonic.h>    // Biblioteca do sensor de distância.
#include <PubSubClient.h> // Biblioteca para enviar dados MQTT.
#include <ArduinoJson.h>  // Biblioteca para criar mensagem Json.

// ------------------------------------------------ --------------------------------------------
// INFORMAÇÕES RELACIONADAS AO AMBIENTE DA IBM E DE REDE
// ------------------------------------------------ --------------------------------------------

#define MQTT_HOST "q2micp.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:q2micp:NODEMCU:dev01"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "PI_4_MECS" // change your auth_id 
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"
char ssid[] = "VIVOFIBRA-F738"; 
char pass[] = "9C4B3777D4";


// ------------------------------------------------ --------------------------------------------
// VARIAVEIS GLOBAIS
// ------------------------------------------------ --------------------------------------------

#define rele_pin 13
int hour; // Armazena o horário obtido do NTP.
int last_hour; // Armazena horário da última medida realizada.
float power = 0; // Armazena o valor economizado em Watts (W)
int diff_hour; // Armazena diferença em segundos entre duas medidas.
unsigned long last_time; // Armazena intervalo de tempo entre medidas.


// --------------------------------------------------------------------------------------------
// LOGICA DE CONEXAO E LEITURA
// --------------------------------------------------------------------------------------------

Ultrasonic dtc(4,5); // Declaração do sensor ultrassonico

// Função de callback para evitar erros no transporte de informacoes
void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0;
  Serial.println((char *)payload);
}

// Objetos MQTT 
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient cliente;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, cliente);

// Variaveis para o transporte de informacoes
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject json = payload.createNestedObject("d");
static char msg[50];

void setup(){
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("PI_4_CONECTANDO_O_MECS");

  // Inicialização das conexões com a rede
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi Conectado");

  // Conexão via MQTT - IBM Watson IoT Platform
  if(mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)){
    Serial.println("MQTT Conectado");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);
  } else {
    Serial.println("Falha na conexao MQTT!");
    ESP.reset();

  }
  // Definicao do pino para o modulo relé
  pinMode(rele_pin,OUTPUT);
}

void loop(){
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Tentando iniciar conexao...");
    // Função de conexão via MQTT
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Conectado");
      mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.loop();
    } else {
      Serial.println("Falha na conexao MQTT!");
      delay(5000);
    }
  }
  
  float cmMsec;
  long microsec = dtc.timing();
  cmMsec = dtc.convert(microsec, Ultrasonic::CM);

   if ( millis() - last_time > 5000 ) {
    if(cmMsec <= 60.00){
      digitalWrite(rele_pin, LOW); 
    }
    else{
      power += 0.011111; // Calcula valor aproximado da Potência Economizada.      
      digitalWrite(rele_pin, HIGH);   
    }
   }

   last_time = millis();
   
  // Verifica se alguma leitura apresentou erros e tenta realizar novamente.
  if(isnan(power)) {
    Serial.println("Falha ao se conectar com o sensor de distancia");
  } else {
    json["dtc"] = power;
    serializeJson(jsonDoc, msg, 50) ;
    Serial.println(msg) ;
    if(!mqtt.publish(MQTT_TOPIC, msg)){
      Serial.println("MQTT Publish falhou") ;
    }
  }

  // Pause - mas continue pesquisando MQTT para mensagens recebidas
  for( int i  =  0 ; i < 10 ; i ++ )  {
    mqtt.loop() ;
    delay(1000) ;
  }
}
