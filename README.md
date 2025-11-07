# Sensor-del-nivel-de-agua-y-temperatura
Código y archivo de texto necesarios para la creación de un sensor de la turbidez y salinidad del agua, así como para medir la temperatura en la que se encuentra el espacio, en el código Arduino

En este archivo explico una serie de formulas y de lineas de códigos que considero necesarias para la compresión total del código en arduino que adjunto.
Lo primero de todo es la formula necesaria para obtener la linea del código "const float pendiente_turbidez = 400.0;" a través de la cual podemos calcular en las
lineas siguientes el valor exacto de la turbidez obtenida por el sensor, esta pendiente es una calibración que nos permite obtener un valor más real de la turbidez
medida por el sensor. Los pasos para obtener son los siguientes:
 1º Medir agua con cierta turbidez, algo que provoque que el voltaje varie, por ejemplo yo en mis puebras utilice agua ligeramente turbia obteniendo una turbidez de 
alrededor de 687 NTU con un voltaje de 2.982
 2º Obtener el voltaje con el que tu circuito obtiene 0 NTU, en mi caso a 4.7 voltios aproximadamente 
 
A traves de una formula que divide dos restas siendo el numerador la diferencia entre los valores de turbidez obtenidos (687 NTU en mi caso) y el denominador 
la diferencia entre el voltaje con turbidez y el voltaje de agua limpia (-1.718 en mi caso). Es cierto que la división produce un valor negativo, pero por lógica 
puse el valor de la pendiente en positivo, ya que en otro caso, a medida que aumentaba el valor de la turbidez disminuia el valor del voltaje, y esto no deberia 
ser una relación inversa, deberia ser una relación directa, a medida que aumenta la turbidez aumenta el voltaje.

Ahora paso a especificar las librerias utilizadas en mi código que son cuatro en total: 
#include <WiFi.h> Libreria utilizada para conectar la ESP32 a una red wifi o para crear un punto de acesso 
#include <WiFiClientSecure.h> utilizada para establecer conexiones WIFI cifradas con aplicaciones y páginas como Telegram
#include <UniversalTelegramBot.h> Libreria que usamos para establecer conexión directa con un bot de Telegram
#include "DHT.h" Libreria especifica para el uso del sensor de temperatura y humedad TDH11 

Destacando lineas del código que pueden resultar mas liosas o dificiles de comprender, hay que entender que... 
Las lineas que dicen: 
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
La primera crea un objeto llamado client que va a establecer una conexión totalmente segura para que no haya ningun tipo de problema en el envio de datos.
la segunda linea crea el objeto bot pasandole dos parametros:
 1º BOT_TOKEN: llave que sirve para identificarse en el telegram, obtenida cuando he creado mi bot 
 2º client: el canal seguro creado en la linea anterior, para que asi el bot pueda comunicarse de forma totalmente segura.

Posteriormente nos encontramos con la declaración de todas las variables, algo que yo creo que simplemente con los comentarios del código se entiende bastante bien

Una vez acaba las declaraciones de las variables nos encontramos con 3 módulos cada uno con una función
A través del primero conseguimos el voltaje que circula por un determinado pin de la ESP32.
El segundo y el tercero nos transforman el voltaje que tenemos en un valor de turbidez y salinidad respectivamente.

Posteriormente en el void setup se inicializan los pines de turbidez, salinidad y temperatura y humedad, adenás de los pines utilizados para los lEDSl también se 
establece que la comunicación con el monitor serial a 115200 baudios, además de establecer una conexión un poco menos compleja con Telegram para el envio de 
datos a través de la linea: client.setInsecure()

A partir de aqui creo que todo queda claro con los comentarios del código, simplemente destacar que la función "millis();" nos da el tiempo en el que nos situamos 
desde el encendido o ultimo reinicio de la ESP32 y que con el siguiente condicional: 
 if (alerta_mensaje.length() > 0) {
            bot.sendMessage(CHAT_ID, alerta_mensaje, "");
            Serial.println("Telegram: Mensaje de alerta enviado (temporizado).");
        }
Provocamos que la ESP32 unicamente envie un mensaje al telegram si la longitud de este es mayor que cero, es decir, si hay algo escrito. Asi al telegram no
llegaran mensajes sin ninguna información, enviando asi al telegram con el CHAT_ID especificado en los parametros del comando bot.sendMessage, el contenido 
presente en la variable alerta_mensaje, modificada en los condicionales anteriormente.

Ya en la parte última del código lo único que se hace es imprimir los varoles en el serial monitor y la única linea a destacar es "if (!isnan(humedad) &&
!isnan(Temperatura))" donde a traves de la función isnan comprobamos si los valores registrados en las variables humedad y temperatura son correctos o por el 
contrario son valores no validos, irreales. Si isnan es falso es que entonces la variable float tiene un valor valido en su interior, un valor que si es un numero.

Para concluir este texto, tengo que decir que hay una gran cantidad de ampliaciones para este código que se me han pasado por la cabeza(la adicción de más 
sensores como por ejemplo un sensor que mida el pH, una alarma más profesional que los leds, la creación de una pagina web sencilla para mostrar los resultados, la fecha y hora exacta en la que se mandaron por última vez...) Pero debido a la escasez de tiempo que he tenido no he podido desarrollarlas.

