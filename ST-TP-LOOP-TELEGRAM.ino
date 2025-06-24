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

//configuación de la pantalla
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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
#define SW1_PIN 34
#define SW2_PIN 35
#define LED 25

//defino la red y contraseña del wifi
const char* ssid = "MECA-IoT";
const char* password = "IoT$2025";

#define BOTtoken "8025037753:AAFpbCTQcwS2Zl1ebt8SktN_9j35VvIw4xY";  // your Bot Token (Get from Botfather)
#define CHAT_ID "6235002183"; //el ID del chat

//define los 2 loops
TaskHandle_t Task1;
TaskHandle_t Task2;

//variables globales
float umbral = 20.0; //valor del umbral
float temperatura = dht.readTemperature();
int estadoActual = PANTALLA1;

//variables antirebote 
bool ARBOT1 = HIGH;
bool ARBOT2 = HIGH;
unsigned long tiempoBotones = 0;
unsigned long intervaloBotones = 200;

//variables delay
unsigned long TiempoUltimoCambio = 0;
const long intervalo = 500;

//variables delay 5 segundos
unsigned long TimeUltimoCambio = 0;
const long intervalo5seg = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

 // conecta al wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    delaySB();
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");




  xTaskCreatePinnedToCore(
  Task1code, //crea la tarea Task1code, el loop 1
  "Task1",    
  10000, //le da 10k de stack
  NULL, // van los parámetros, este no tiene
  1, // le da prioridad 1
  &Task1,    
  0); //fija el núcleo en cero                 
  delaySB(); //cada cuanto se repite

  xTaskCreatePinnedToCore( //hace lo mismo con la Task2code, osea el loop 2
  Task2code,  
  "Task2",    
  10000,      
  NULL,        
  1,          
  &Task2,      
  1);          
  delaySB();
}

//funcion delay sin bloqueo
bool delaySB () {
  unsigned long TiempAhora = millis();
  if (TiempoAhora - TiempoUltimoCambio >= intervalo) {
    TiempoUltimoCambio = TiempoAhora;
    return true;
  }
  return false;
}

bool delay5seg () {
  unsigned long TiempoAtual = millis();
  if (TiempoActual - TimeUltimoCambio >= intervalo5seg) {
    TimeUltimoCambio = TiempoActual;
    return true;
  }
  return false;
}

void antiRebote () {
  unsigned long tiempoAhora = millis();
  if (tiempoAhora - tiempoBotones >= intervaloBotones) {
    ARBOT1 = digitalRead(SW1_PIN);
    ARBOT2 = digitalRead(SW2_PIN);
    tiempoBotones = tiempoAhora;
  }
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
      bot.sendMessage(CHAT_ID, "La temperatura actual es:   "   + String(temperatura), "");
    } //si el usuario manda umbral actual le va a mandar el umbral actual uwu
  }
}


//PRIMER LOOP: TELEGRAM
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core "); //es para mostrar en el monitor serie en que núcleo la tarea está corriendo
  Serial.println(xPortGetCoreID()); //es una función que muestra si la tarea está corriendo, si está corriendo devuelve 1 y si no devuelve 0

  for(;;){//si o si tiene que ir un while o un for para que nunca salga de la tarea
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); //telegram le suma 1 al id de los mensajes nuevos, por eso hay que sumar 1 ada vez que se hace el bucle, porque sino te va a dar siempre el mismo mensaje

    while (numNewMessages) { //esto solo se ejecuta cuando se cumple lo de arria, es decir que llegan mensajes
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1); //esto es por si llegan mensajes nuevos mientras está procesando
    }
    delay5seg(); // revisa cada 5 segundos si hay mensajes nuevos
  }
}

//SEGUNDO LOOP: MAQUINA DE ESTADOS
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){ //si o si tiene que ir un while o un for para que nunca salga de la tarea
    switch (estadoActual){
      case PANTALLA1:
        antiRebote();
        //leer la temperatura
        sensors_event_t event; //guarda los datos que vienen del sensor
        dht.temperature().getEvent(&event); //llama al sensor y para que obtenga el nuevo dato de la temperatura y guarda los datos en la variable event
        float temperatura = event.temperature; //saca el dato guardado en event y lo guadra en la variable temperatura

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
        display.print(" C");
        display.display();

        if (ARBOT1 == LOW){
          estadoActual = SW1;
        }
        break;
     
      case SW1:
        antiRebote();
        if (ARBOT1 == HIGH) {
          estadoActual = ON1;
        }
        break;

      case ON1:
        antiRebote();
        if (delay5seg() && ARBOT2 == HIGH) { //si pasan 5 segundos y el boton 2 no esta apretado
          estadoActual = PANTALLA1;
        }
        else if (ARBOT2 == LOW) {
          estadoActual = SW2;
        }
        break;
        
      case SW2:
        antiRebote();
        if (ARBOT2 == HIGH){
          estadoActual = ON2;
        }
        break;

      case ON2:
        antiRebote();
        if (delay5seg() && ARBOT1 == HIGH) {
          estadoActual = PANTALLA1;
          }
        else if (ARBOT1 == LOW){
          estadoActual = SW12;
          }
          break;
          
      case SW12:
        antiRebote();
        if (ARBOT1 == HIGH) {
          estadoActual = ON12;
          }
          break;

      case ON12:
        antiRebote();
        if (ARBOT1 == HIGH) {
          estadoActual = PANTALLA2;
          }
          break;
          
      case PANTALLA2:
        antiRebote();

        if (ARBOT1 == LOW) {
          estadoActual = SUMA;
        }

        if (ARBOT2 == LOW) {
          estadoActual = RESTA;
        }

        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("Ajuste umbral:");
        display.setCursor(0, 20);
        display.setTextSize(2);
        display.print(umbral);
        break;

      case SUMA:
        antiRebote();

        if (ARBOT1 == LOW){
          umbral += 1;
        }

        if (ARBOT1 == HIGH && ARBOT2 == HIGH){
          estadoActual = PANTALLA2;
        }
        
        if (ARBOT2 == LOW){
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
        antiRebote();

        if (ARBOT2 == LOW) {
          umbral -= 1;
        }

        if (umbral <= 0){ //que no pueda mostrar numeros negativos, preguntarle a chat
          umbral = 0;
        }

        if (ARBOT1 == HIGH && ARBOT2 == HIGH){
          estadoActual = PANTALLA2;
        }

        if (ARBOT1 == LOW){
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
        antiRebote();
        if (ARBOT1 == HIGH && ARBOT2 == HIGH) {
          estadoActual = PANTALLA1;
        }
        break;
    }
  }
}
