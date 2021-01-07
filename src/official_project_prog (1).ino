#include <DHT.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
// Déclaration des bibliothèques nécessaires

String writeAPIKey = "CGRCOK867LD3HPP2"; //clé API d'écriture dans ThingSpeak 
const char * readAPIKey="W0HAY9LWP1DKMLSC"; //clé API de lecture dans ThingSpeak 
String ssid = "nour"; // ssid pour la connexion au WIFI
String password = "nouraito"; //mot de passe pour la connexion au WiFi
String server = "api.thingspeak.com"; //Adresse de ThingSpeak utilisé plus loin dans la requête http
unsigned long pumpChannelNumber = 758741; //numéro de la channel (canal) dans ThingSpeak
unsigned int pumpFieldNumber = 1; //numéro du field de ThingSpeak associé à la pompe
int pumpState=0; //variable contenant l'état de la pompe lu à partir du field associé dans ThingSpeak
int waterSensorVal=0; //variable pour stocker la valeur retournée par le water sensor 
const int soilSensor=A0; //déclaration du pin analog pour le soil sensor
int soilHumid=0; //variable pour stocker la valeur retournée par le soil sensor 
int soilHumidPourc=0; //variable pour stocker la valeur retournée par le soil sensor en pourcentage 

#define PUMP D3 //pin de la pompe
#define WATERSENSOR D5 //pin du water sensor
#define DHTPIN D4 //pin du dht11
#define DHTTYPE DHT11 //déclaration du type de capteur dht
 
DHT dht(DHTPIN, DHTTYPE); //déclaration de l'objet de type dht
WiFiClient client; //client pour communiquer avec ThingSpeak
 
void setup() 
{
  Serial.begin(115200); //lancement du moniteur série 
  ThingSpeak.begin(client);//initialisation de ThingSpeak
  delay(10);
  //Mettre les pins des capteurs sur input et celui de la pompe sur output
  pinMode(soilSensor,INPUT);
  pinMode(PUMP,OUTPUT);
  digitalWrite(PUMP,LOW);
  pinMode(WATERSENSOR,INPUT);

  dht.begin(); //lancement du dht11
  WiFi.begin(ssid, password); //initialisation du ssid et mot de passe du WiFi

  Serial.println();
  Serial.println();
  //connexion au wifi
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  while (WiFi.status() != WL_CONNECTED) //connexion au WiFi
 {
  delay(500);//on doit attentre 500 millisecones entre é essaies consecutifs
  Serial.print(".");
 }
  Serial.println("");
  Serial.println("WiFi connected"); 
}

void loop() 
{ 
  //lecture des valeurs mesurées des capteurs
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  soilHumid=analogRead(soilSensor);
  soilHumidPourc=map(soilHumid,1023,0,0,100);
  waterSensorVal=digitalRead(WATERSENSOR);
  //on vérifie si la lecture c'est passé sans erreur pour le dht11
  if (isnan(h) || isnan(t)) 
  {
    Serial.println("Can't read from dht sensor");
    return;
  }
 
  if (client.connect(server,80)) { 
  //requete HTTP pour envoyer les mesures des capteurs à ThingSpeak 
  String httpRequest = writeAPIKey;
  httpRequest +="&field2=";
  httpRequest += String(t);
  httpRequest +="&field3=";
  httpRequest += String(h);
  httpRequest +="&field4=";
  httpRequest += String(soilHumidPourc);
  httpRequest +="&field5=";
  httpRequest += String(waterSensorVal);
  httpRequest += "\r\n\r\n";

  //entete de la requete 
  client.print("POST /update HTTP/1.1\n");
  client.print("Host: api.thingspeak.com\n");
  client.print("Connection: close\n");
  client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(httpRequest.length());
  client.print("\n\n");
  //corps de la requete
  client.print(httpRequest);
  //fin de la requete et affichage des valeurs mesurées par les capteurs
  
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" degrees Celsius Humidity: ");
  Serial.print(h);
  Serial.print(" soil humidity");
  Serial.print(soilHumidPourc);
  Serial.print(" water sensor val");
  Serial.print(waterSensorVal);
  
  Serial.println("Sending data to Thingspeak");

  //lecture du field de la pompe à partir de ThingSpeak
  Serial.println("reading from thingspeak ");
  pumpState = readFromThingSpeak( pumpChannelNumber, pumpFieldNumber ); //appel à la fonction qui effectue la lecture
  Serial.print(pumpState);
  }
  //si le sol n'est pas assez humide, que l'application est sur on et qu'il ne pleut pas on lance la pompe sinon on l'éteint
  if ((soilHumidPourc<=30)&&(pumpState!=0)&&(h<90)){
    digitalWrite(PUMP,HIGH);
  }
  else digitalWrite(PUMP,LOW);
  delay(3000);
  /*deuxième lecture du soil sensor, si le sol est assez humide on l'éteint, ceci n'est pas à faire dans un cas réel mais il est nécessaire dans notre cas car nous avons une mini ferme et
  la pompe ne peut rester allumer trop longtemps*/
  soilHumid=analogRead(soilSensor);
  soilHumidPourc=map(soilHumid,1023,0,0,100);
  if ((soilHumidPourc<=30)&&(pumpState!=0)&&(h<90)){
  digitalWrite(PUMP,HIGH);
  }
  else digitalWrite(PUMP,LOW);

  delay(20000);  
  client.stop();
  Serial.println("Waiting 20 seconds");// intervalle de temps qu'il faut attendre avant d'effectuer une seconde lecture/écritue dans ThingSpeak
}

int readFromThingSpeak( long channelNumber,unsigned int fieldNumber )
{  
  int data =  ThingSpeak.readIntField( channelNumber, fieldNumber, readAPIKey ); //appel à la fonction prédefinis dans la bibliothèque ThingSpeak.h
  return data; 
}
