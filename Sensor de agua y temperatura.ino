#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>//libreria para el telegram bot
#include "DHT.h"//Librería para el sensor DHT

//Configuración de red
const char* NOMBRE_WIFI = "Movil Javi";//nombre de la wifi a la que estamos conectados
const char* CONTRASEÑA = "12345678";//contraseña de la wifi 

#define BOT_TOKEN "8288127741:AAE3keH0LW7ri-LRNxTSFQANVFtYT2WYSmE"//TOKEN DEL BOT
const String CHAT_ID="5762882135";//ID DEL USUARIO A QUE ENVIAMOS EL MENSAJE

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Pines  utilizados de la ESP32
const int pinTurbidez = 32; 
const int pinTDS_Meter = 35;
const int pinLED = 4;//LED de alerta de turbidez
const int pinLED2= 5;//LED de alerta de salinidad

#define DHTPIN 18 // Pin donde conecto la Señal/Data del DHT11
#define DHTTYPE DHT11// Definición del modelo DHT11
DHT dht(DHTPIN, DHTTYPE);


const int NUM_LECTURAS=100;//numero de veces que la ESP32 toma una muestra del voltaje del sensor
const float V_ref = 5.0;//voltaje que se introduce en los sensores
const float Val_max = 4095.0;

const float Leveltur_max = 25.0;// Alerta si es mayor a 25 NTU.
const float Levelsal_max = 600.0; // Alerta si la salinidad es mayor de 600 ppm 
const float V_limpio = 3; // Voltaje en agua limpia 
const float pendiente_turbidez = 400.0; //pendiente de la turbidez 

const float TDS_1V = 100;//Pendiente (Slope) TDS vs. Voltaje
const float TDS_0V = 450.0;//TDS a 0V

// Variables Control del tiempo
const long T_Alerta = 30000; // 30 segundos en milisegundos
unsigned long tiempoAnterior = 0; // Almacena el tiempo exacto en el que se realizo el ultimo envio

float Conseguir_voltaje_pin(int pinESP32) { 
    long suma = 0;
    int i = 0; 
    while (i <NUM_LECTURAS) {
        suma = suma + analogRead(pinESP32);
        i++;
    }
    float voltaje_pin = suma / NUM_LECTURAS; 
    return (voltaje_pin / Val_max) * V_ref;
}

float voltage_to_NTU(float V) {
    float ntu_val = pendiente_turbidez * (V_limpio - V);
    if (ntu_val < 0) { 
        return 0.0;
    }
    return ntu_val;
}

float voltage_to_TDS(float V) {
    float tds_val = TDS_1V * V + TDS_0V; 
    return (tds_val < 0) ? 0.0 : tds_val;
}


void setup() {
    Serial.begin(115200);
    pinMode(pinLED, OUTPUT); 
    pinMode(pinLED2, OUTPUT); 
    
    analogSetPinAttenuation(pinTurbidez, ADC_11db);//inicializa el pin de turbidez
    analogSetPinAttenuation(pinTDS_Meter, ADC_11db);//inicializa el pin de salinidad
    dht.begin();//Inicializa el sensor DHT11

    Serial.println("--- Monitor de la calidad del agua inicializandose ---");


    client.setInsecure();
}

void loop() {
    // CONDICIONAL PARA LA RECONEXIÓN DE WIFI
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("reconectando WiFi...");
        WiFi.begin(NOMBRE_WIFI, CONTRASEÑA);
        delay(10000); 
        return; 
    }
    
    //lineas para obtener el voltaje exacto de los sensores de turbidez y de salinidad asi como los valores que obtienen
    float V_turb = Conseguir_voltaje_pin(pinTurbidez);
    float turb_Valor = voltage_to_NTU(V_turb);
    float V_TDS = Conseguir_voltaje_pin(pinTDS_Meter);
    float tds_Valor = voltage_to_TDS(V_TDS);

    //lectura del sensor DHT11 
    float humedad = dht.readHumidity();
    float Temperatura= dht.readTemperature();
    
    bool alertaActiva = false;//creación de un booleano para establecer si hay que activar una alerta o no, dependiendo de si los sensores detectan unos valores por encima del umbral

    //comprobación de si el valor de turbidez supera el umbral establecido
    if (turb_Valor > Leveltur_max) {
        digitalWrite(pinLED, HIGH);
        Serial.println("!TURBIDEZ DEMASIADO ALTA!"); 
        alertaActiva = true;
    } else {
        digitalWrite(pinLED, LOW);
    }
    //comprobación de si el valor de salinidad supera el umbral establecido
    if (tds_Valor > Levelsal_max) {
        digitalWrite(pinLED2, HIGH);
        Serial.println("!SALINIDAD DEMASIADO ALTA!"); 
        alertaActiva = true;
    } else {
        digitalWrite(pinLED2, LOW);
    }
    
    // LÓGICA DE ENVÍO DE TELEGRAM
    unsigned long tiempoActual = millis();//nos da el tiempo en el que nos situamos desde el encendido o ultimo reinicio de la ESP32 

    if (alertaActiva && (tiempoActual - tiempoAnterior >=T_Alerta)) {
        
        tiempoAnterior = tiempoActual;
        //aqui definimos el mensaje exacto de la temperatura y de la humedad, el de alerta o bien permanece igual que significaria que esta todo correcto o varia dentro de los distintos condicionales de abajo
        String alerta_mensaje = "";
        String tempHum = " (Ambiente: " + String(Temperatura, 1) + " C, " + String(humedad, 1) + "%)";

        if (turb_Valor > Leveltur_max && tds_Valor > Levelsal_max) {
             alerta_mensaje = "ALERTA DOBLE: Turbidez (" + String(turb_Valor, 2) + " NTU) y Salinidad (" + String(tds_Valor, 1) + " ppm) demasiado altas." + tempHum;
        }
        else if (turb_Valor > Leveltur_max) {
             alerta_mensaje = "ALERTA Turbidez: " + String(turb_Valor, 2) + " NTU. El TDS es: " + String(tds_Valor, 1) + " ppm." + tempHum;
        } 
        else if (tds_Valor > Levelsal_max) {
             alerta_mensaje = "ALERTA Salinidad: " + String(tds_Valor, 2) + " PPM. La turbidez es: " + String(turb_Valor, 1) + " NTU." + tempHum;
        }
        
        if (alerta_mensaje.length() > 0) {
            bot.sendMessage(CHAT_ID, alerta_mensaje, "");
            Serial.println("Telegram: Mensaje de alerta enviado (temporizado).");
        }
    }
    //Impresión de todos los valores de turbidez, salinidad y temperatura/humedad
    Serial.print("Turbidez: "); 
    Serial.print(turb_Valor, 2);
    Serial.print(" NTU (V: ");
    Serial.print(V_turb, 3);
    Serial.println(")");
    
    Serial.print("TDS/Salinidad ");
    Serial.print(tds_Valor, 2);
    Serial.print(" ppm (V: ");
    Serial.print(V_TDS, 3);
    Serial.println(")");
 
    if (!isnan(humedad) && !isnan(Temperatura)) {
        Serial.print("Temperatura: "); 
        Serial.print(Temperatura, 2);
        Serial.print(" C | Humedad: "); 
        Serial.print(humedad, 1);
        Serial.println(" %");
    }
    
    delay(5000); // espera 5 segundos
}