// Colodner,Bravar,Bocci y Toledo GRUPO 2
//8025037753:AAFpbCTQcwS2Zl1ebt8SktN_9j35VvIw4xY: el token de mi bot
//6235002183: ID del chat

//librerías
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFiClientSecure.h>

//configuación de la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//defino estados
#define PANTALLA1 1
#define SW1 2
#define ON1 3
#define SW2 4
#define ON2 5
#define SW12 6
#define ON12 7
#define PANTALLA2 8
#define SUMA 9
#define RESTA 10
#define CONFIRMACION 11

//dht
#define DHTPIN 23
#define DHTTYPE DHT11 //tipo de sensor
DHT_Unified dht(DHTPIN, DHTTYPE);

//pines de la plca
#define SW1_PIN 35
#define SW2_PIN 34
#define LED 25

//defino la red y contraseña del wifi
const char* ssid = "MECA-IoT";
const char* password = "IoT$2025";

#define BOTtoken "8025037753:AAFpbCTQcwS2Zl1ebt8SktN_9j35VvIw4xY"  // your Bot Token (Get from Botfather)
#define CHAT_ID "6235002183" //el ID del chat

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);


//define los 2 loops
TaskHandle_t Task1;
TaskHandle_t Task2;

//variables globales
float umbral = 10.0; //valor del umbral
int estadoActual = PANTALLA1;
float temperatura = 0.0;

//delay
unsigned long contando = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  dht.begin();
  sensor_t sensor;

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

 // conecta al wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //un dalay tal vez
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  client.setInsecure();

  xTaskCreatePinnedToCore(
  Task1code, //crea la tarea Task1code, el loop 1
  "Task1",    
  10000, //le da 10k de stack
  NULL, // van los parámetros, este no tiene
  1, // le da prioridad 1
  &Task1,    
  0); //fija el núcleo en cero                 
  delay(1000); //cada cuanto se repite

  xTaskCreatePinnedToCore( //hace lo mismo con la Task2code, osea el loop 2
  Task2code,  
  "Task2",    
  10000,      
  NULL,        
  1,          
  &Task2,      
  1);
  delay(1000);          
}

//función que muestra en el monitor serial cuantos mensajes nuevos recibió
void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);
  //analiza 1 por 1 los mensajes ingresados
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "User no autorizado", "");
      continue; //esto es para confirmar que sea el usuario autorizado
    }
    // guarda los mensajes que recibe en text y los printea
    String text = bot.messages[i].text;
    Serial.println(text);    
    String from_name = bot.messages[i].from_name; //el from_name lo usa para poder personalizar mensajes con el nombre de la persona
    if (text == "Temperatura actual") {
      bot.sendMessage(CHAT_ID, "La temperatura actual es:   "   + String(temperatura), ""); //si el usuario manda umbral actual le va a mandar el umbral actual uwu
    }
  } 
}


//PRIMER LOOP: TELEGRAM
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core "); //es para mostrar en el monitor serie en que núcleo la tarea está corriendo
  Serial.println(xPortGetCoreID()); //es una función que muestra si la tarea está corriendo, si está corriendo devuelve 1 y si no devuelve 0

  for(;;){//si o si tiene que ir un while o un for para que nunca salga de la tarea
    //vTaskDelay(5000/portTICK_PERIOD_MS); // revisa cada 5 segundos si hay mensajes nuevos
    
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); //telegram le suma 1 al id de los mensajes nuevos, por eso hay que sumar 1 ada vez que se hace el bucle, porque sino te va a dar siempre el mismo mensaje

    while (numNewMessages) { //esto solo se ejecuta cuando se cumple lo de arria, es decir que llegan mensajes
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1); //esto es por si llegan mensajes nuevos mientras está procesando
    }   
  } 
}

//SEGUNDO LOOP: MAQUINA DE ESTADOS
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){ //si o si tiene que ir un while o un for para que nunca salga de la tarea
    //leer la temperatura
    if (millis() - contando >= 5000) {
      sensors_event_t event;
      dht.temperature().getEvent(&event);
      temperatura = event.temperature; //saca el dato guardado en event y lo guadra en la variable temperatura
    }

    if (temperatura >= umbral) {
      bot.sendMessage(CHAT_ID, "La temperatura superó al umbral");
    } 


    switch (estadoActual){
      case PANTALLA1:

        Serial.println("PANTALLA1");

        if (temperatura >= umbral) {
          digitalWrite(LED, HIGH);
        }
        else {
          digitalWrite(LED, LOW);
        }

        //muestra la temperatura en la pantalla
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 10); 
        display.print("Temp: ");
        display.print(temperatura);
        display.display();

        if (digitalRead(digitalRead(SW1_PIN) == LOW)){
          estadoActual = SW1;
        }
        break;
     
      case SW1:
    

        Serial.println("SW1");

        if (digitalRead(SW1_PIN) == HIGH) {
          contando = millis();
          estadoActual = ON1;
        }
        break;

      case ON1:
        Serial.println("ON1");

        if ((millis() - contando >= 5000) && digitalRead(SW2_PIN) == HIGH) { //si pasan 5 segundos y el boton 2 no esta apretado
          contando = 0;
          estadoActual = PANTALLA1;
        }
        else if (digitalRead(SW2_PIN) == LOW) {
          estadoActual = SW2;
        }
        break;
        
      case SW2:
        Serial.println("SW2");
        if (digitalRead(SW2_PIN) == HIGH){
          contando = millis();
          estadoActual = ON2;
        }
        break;

      case ON2:
        Serial.println("ON2");
        if ((millis() - contando >= 5000) && digitalRead(SW1_PIN) == HIGH) {
          contando = 0;
          estadoActual = PANTALLA1;
          }
        else if (digitalRead(SW1_PIN) == LOW){
          estadoActual = SW12;
          }
          break;
          
      case SW12:
        Serial.println("SW12");
        if (digitalRead(SW1_PIN) == HIGH) {
          estadoActual = ON12;
          }
          break;

      case ON12:
        Serial.println("ON12");
        if (digitalRead(SW1_PIN) == HIGH) {
          estadoActual = PANTALLA2;
          }
          break;
          
      case PANTALLA2:
        Serial.println("PANTALLA2");

        if (digitalRead(SW1_PIN) == LOW) {
          estadoActual = SUMA;
        }

        if (digitalRead(SW2_PIN) == LOW) {
          estadoActual = RESTA;
        }
        display.clearDisplay(); 
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Ajuste el umbral:");
        display.setCursor(0, 20);
        display.setTextSize(2);
        display.print(umbral);
        display.display();
        break;

      case SUMA:
        Serial.println("SUMA");
        if (digitalRead(SW1_PIN) == HIGH){
          umbral += 1;
        }

        if (digitalRead(SW1_PIN) == HIGH && digitalRead(SW2_PIN) == HIGH){
          estadoActual = PANTALLA2;
        }
        
        if (digitalRead(SW2_PIN) == LOW){
          estadoActual = CONFIRMACION;
        }

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Ajuste umbral:");
        display.setCursor(0, 20);
        display.setTextSize(2);
        display.print(umbral);
        break;

      case RESTA:
        Serial.println("RESTA");
        if (digitalRead(SW2_PIN) == HIGH) { 
          umbral -= 1;
        }

        if (digitalRead(SW1_PIN) == HIGH && digitalRead(SW2_PIN) == HIGH){
          estadoActual = PANTALLA2;
        }

        if (digitalRead(SW1_PIN) == LOW){
          estadoActual = CONFIRMACION;
        }
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Ajuste umbral:");
        display.setCursor(0, 20);
        display.setTextSize(2);
        display.print(umbral);
        break;

      case CONFIRMACION:
        Serial.println("CONFIRMACION");
        if (digitalRead(SW1_PIN) == HIGH && digitalRead(SW2_PIN) == HIGH) {
          estadoActual = PANTALLA1;
        }
        break;
    }
  }
}

void loop(){}
