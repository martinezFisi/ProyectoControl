#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"

//Sensor DHT11
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht( DHTPIN, DHTTYPE );

//Nuestro servidor que será esta página
EthernetServer servidor(80); // puerto de conexión

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //mac address
byte ip[] = { 192, 168, 0, 5 }; // ip de tu Arduino en la red
byte gateway[] = { 192, 168, 0, 1 }; // ip de tu router
byte subnet[] = { 255, 255, 255, 0 }; // subnet

//String que recibimos de la página cliente
String solicitud;
//String en donde mandaremos los datos al cliente 
String json;

float temperatura;
float humedad;
float mq7;
float uv;
float mq2;

int tempPin = 0;
int mq7Pin = A0;
int UVOUT = A1; //Output from the sensor
int REF_3V3 = A2; //3.3V power on the Arduino board
int mq2Pin = A3;

void setup()
{
    Serial.begin(9600);
  
    //Pines análogos de entrada
    pinMode(UVOUT, INPUT);
    pinMode(REF_3V3, INPUT);
    pinMode(mq2Pin, INPUT);
    
    //Inicializamos el servidor
    Ethernet.begin(mac, ip, gateway, subnet); 
    servidor.begin();
    
    //Inicializamos el sensor DHT
    dht.begin();
    
}

void loop() 
{ 
    //Obtenemos la temperatura del ambiente
    temperatura = dht.readTemperature();
    //Obtenemos la humedad del ambiente
    humedad = dht.readHumidity();
    //Obtenemos el MQ7
    mq7 = analogRead( mq7Pin );
    
    //Obtenemos el UV
    int uvLevel = averageAnalogRead(UVOUT);
    int refLevel = averageAnalogRead(REF_3V3);
    
    //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
    float outputVoltage = 3.3 / refLevel * uvLevel;
    
    uv = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0); //Convert the voltage to a UV intensity level
  
    //Obtenemos el MQ2
    mq2 = analogRead( mq2Pin );
    mq2 = map( mq2, 0, 1023, 0, 100 );
     
    
    //Instanciamos un cliente que será la página que solicite los datos
    EthernetClient cliente = servidor.available();
    
    //Si hay un cliente disponible
    if ( cliente.available() )
    {
        //Recolectamos caracter por caracter el mensaje de solicitud
        char c = cliente.read();
        
        if ( solicitud.length() < 100 )
        { 
            solicitud += c;
        }
        
        //Cuando llegamos al final de la solicitud
        if ( c == '\n' ) 
        {
            
            Serial.print( temperatura );
            Serial.print(" grados Celsius\n");
            Serial.print( humedad );
            Serial.print(" % \n");
            Serial.print( mq7 );
            Serial.print(" MQ7 \n");
            Serial.print(uv);
            Serial.print(" mW/cm^2 de Intensidad UV \n"); 
            Serial.print( mq2 );
            Serial.print(" de MQ2 \n");
          
            
            //Creamos la cadena con notación JSON
            json  = "{\"";
            json += "temperatura\": \"" + (String)temperatura + "\", ";
            json += "\"humedad\": \"" + (String)humedad + "\", ";
            json += "\"mq7\": \"" + (String)mq7 + "\", ";
            json += "\"uv\": \"" + (String)uv + "\", ";
            json += "\"mq2\": \"" + (String)mq2 + "\", ";
            json += "\"uptime\": \"" + (String)millis() + "\" ";
            json += " }\n ";
            
            //Enviamos los datos al cliente
            cliente.println("HTTP/1.1 200 OK"); // enviamos cabeceras
            cliente.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
            cliente.println("Content-Type: text/javascript");
            cliente.println("Access-Control-Allow-Origin: *");
            cliente.println();
            cliente.println( json ); //imprimimos datos
            
            delay(100); // esperamos un poco
            cliente.stop(); //cerramos la conexión
        }
    }
    
    if ( !cliente.connected() ) 
    { 
        cliente.stop(); 
    }
    
    
}


//Funciones para el Sensor UV
//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 

  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return(runningValue);  
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


