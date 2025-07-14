//GRUPO 2 5MB
//COLODNER, BOCCI, TOLEDO, BRAVAR 
//token del bot: 8025037753:AAFpbCTQcwS2Zl1ebt8SktN_9j35VvIw4xY
//6235002183: ID del chat


//LIBRERIAS
//General
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
//Telegram
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

//ESTADOS
int ESTADO = 1;
#define PANTALLA1 1
#define PANTALLA2 2
#define SW1 3
#define ON1 4
#define SW2 5
#define ON2 6
#define SW12 7
#define CONFIRMACION 8
#define SUMA 9
#define RESTA 10

//VARIABLES DE TIMERS
unsigned long timer;
unsigned long timer1000;
int segs;
int segs5;

//LED
#define LED 25 //LED: Se prende al superar el umbral

//BOTONES
#define SW1 34
int val1;
#define SW2 35 
int val2;

//PANTALLA
#define LONGITUD 128 // longitud
#define ALTURA 64 // altura
Adafruit_SSD1306 display(LONGITUD, ALTURA, &Wire, -1);

//SENSOR DE TEMPERATURA
#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temp;
float umbral = 24; //limite de temperatura

//TAREAS (LOOPS 1 Y 2)
TaskHandle_t Task1;
TaskHandle_t Task2;

//Variable de confirmación de checkeo de mensaje del BOT
int checkeo;

//WIFI
const char* ssid = "MECA-IoT";     //Nombre de red
const char* password = "IoT$2025"; //Contraseña de red

//Token del bot
#define BOTtoken "8025037753:AAFpbCTQcwS2Zl1ebt8SktN_9j35VvIw4xY"
//ID del chat con el bot
#define CHAT_ID "6235002183"  

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);


void setup() 
{
  Serial.begin(115200);
  Serial.println("Programa iniciado");
  
  //Conexión al wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print("-"); //Se manda indefinidamente si no se logra establecer la conexión al wifi
  }
  Serial.println("WiFi conectado"); //Se manda al verificar que se logró establecer la conexión al wifi

  //El BOT manda un mensaje a telegram para anunciar que se inició
  bot.sendMessage(CHAT_ID, "BOT iniciado", ""); 

  //Botones
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  //LED
  pinMode(LED, OUTPUT);

  dht.begin(); //inicializa en sensor de temperatura
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //inicializa el display

  xTaskCreatePinnedToCore(
    Task1code,       //Nombre de la funcion
    "Task1",         //Nombre de la tarea
    10000,           //Limite de espacio de la tarea
    NULL,            //Parametro
    1,               //Prioridad de la tarea
    &Task1,          //A que tarea se refiere
    0                //A que nucleo del ESP32 se asigna la tarea
  );

  xTaskCreatePinnedToCore(
    Task2code,      //Nombre de la funcion
    "Task2",        //Nombre de la tarea
    10000,          //Limite de espacio de la tarea
    NULL,           //Parametro
    1,              //Prioridad de la tarea
    &Task2,         //A que tarea se refiere
    1               //A que nucleo del ESP32 se asigna la tarea
  );
}


void Task1code( void * pvParameters ) //PRIMER LOOP || LOOP DE TELEGRAM
{
  Serial.println("Task1");
  for(;;)
  {
    //Se actualiza el reloj
    timer = millis();

    //Si temp supera al umbral, y el bot aún no lo anunció (checkeo == 0)
    if (temp > umbral && checkeo == 0) 
    { 
      //Se cambia checkeo a 1, para que el BOT solo mande 1 vez el mensaje
      checkeo = 1;
      //El bot manda un mensaje indicando que se supero el umbral, y se manda también la temperatura actual
      bot.sendMessage(CHAT_ID, "Se superó el umbral, TEMP ACTUAL: " + String(temp), ""); 
    }

    //Cuando temp es más baja que umbral, se hace checkeo = 0, para que cuando temp vuelva a superar a umbral, el BOT vuelva a anunciarlo
    if (temp < umbral)
    {
      checkeo = 0;
    }
                                                       
    if (timer >= timer1000) //Esto ocurre una vez cada segundo
    { 
      int no_leidos = bot.getUpdates(bot.last_message_received + 1); //Se revisa si hay algún mensaje nuevo / no procesado
      
      if (no_leidos > 0) //Si hay algun mensaje no procesado
      {
        String recibido = bot.messages[0].text; //El nuevo mensaje se iguala a recibido

        if (recibido == "TEMP")
        {
          bot.sendMessage(CHAT_ID, "TEMP ACTUAL: " + String(temp), ""); //Si se recibe "TEMP" el bot envía temp
        }
      }

      timer1000 = timer + 1000;
    }

  }
}


void Task2code( void * pvParameters ) //SEGUNDO LOOP || LOOP DE LA MAQUINA DE ESTADOS
{
  Serial.println("Task2");
  for(;;) 
  {
    //Se actualiza el reloj
    timer = millis();

    if (timer >= timer1000)
    {
      timer1000 = timer + 1000;
      segs = segs + 1;
    }

    //Se lee la temperatura (en celcious)
    temp = dht.readTemperature();

    //Se leen los botones
    val1 = digitalRead(SW1);
    val2 = digitalRead(SW2);

    switch (ESTADO) //INICIO DE MÁQUINA DE ESTADOS
    {
      case PANTALLA1: //DISPLAY DE TEMPERATURA, UMBRAL
      Serial.println("pantalla 1");
      display.clearDisplay(); //Se reinicia el display
      display.setTextSize(2); //Establece el tamaño de texto
      display.setTextColor(WHITE);
      
      display.setCursor(0, 10);
      display.println("TEMP:");
      display.setCursor(60, 10);
      display.println(temp); //display de temperatura

      display.setCursor(0, 50);
      display.println("UMB:");
      display.setCursor(60, 50);
      display.println(umbral); //display de umbral

      display.display(); //Muestra el display

      if (temp > umbral)
      {
        digitalWrite(LED, HIGH);
      }
      if (temp < umbral)
      {
        digitalWrite(LED, LOW);
      }

      //CAMBIO DE ESTADO
      if (val1 == 0 && val2 == 1)
      {
        ESTADO = SW1;
        segs5 = segs + 5;
      }
  
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;

      case SW1:
      Serial.println("sw1");
      if (val1 == 1 && val2 == 1)
      {
        ESTADO = ON1;
        segs5 = segs + 5;
      }
      
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;
      
      case ON1:
      Serial.println("on1");
      if (val1 == 1 && val2 == 0)
      {
        ESTADO = SW2;
        segs5 = segs + 5;
      }
      
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;

      case SW2:
      Serial.println("sw2");
      if (val1 == 1 && val2 == 1)
      {
        ESTADO = ON2;
        segs5 = segs + 5;
      }
      
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;

      case ON2:
      Serial.println("on2");
      if (val1 == 0 && val2 == 1)
      {
        ESTADO = SW12;
        segs5 = segs + 5;
      }
      
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;
      
      case SW12:
      Serial.println("sw12");
      if (val1 == 1 && val2 == 1)
      {
        ESTADO = PANTALLA2;
      }
      
      if (segs >= segs5)
      {
        ESTADO = PANTALLA1;
      }

      break;
      
      case PANTALLA2:
      Serial.println("pantalla 2");
      display.clearDisplay(); //Se reinicia el display
      display.setTextSize(2); //Establece el tamaño de texto
      display.setTextColor(WHITE);
      
      display.setCursor(0, 10);
      display.println("SETEAR UMB"); //display de temperatura
      display.setCursor(0, 40);
      display.println(umbral); //display de umbral

      display.display(); //Muestra el display

      if (val1 == 0)
      {
        ESTADO = SUMA;
      }

      if (val2 == 0)
      {
        ESTADO = RESTA;
      }

      break;
      
      case SUMA:
      Serial.println("suma");
      if (val1 == 1)
      {
        ESTADO = PANTALLA2;
        umbral = umbral + 1;
      }

      if (val2 == 0)
      {
        ESTADO = CONFIRMACION;
      }

      break;

      case RESTA:
      Serial.println("resta");
      if (val2 == 1)
      {
        ESTADO = PANTALLA2;
        umbral = umbral - 1;
      }

      if (val1 == 0)
      {
        ESTADO = CONFIRMACION;
      }

      break;

      case CONFIRMACION:
      Serial.println("confirmacion");
      if (val1 == 1 && val2 == 1)
      {
        ESTADO = PANTALLA1;
      }

      break;
    }

  }
}


void loop() 
{

}
