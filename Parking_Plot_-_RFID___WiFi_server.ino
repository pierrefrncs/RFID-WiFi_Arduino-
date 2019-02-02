#include <SoftwareSerial.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>

//#include "index.h"

#define distanceMin 100 //distance de détection du véhicule

const int PIN_LED_R =8;  //Pin LED Rouge
const int PIN_LED_V =9;  //Pin LED Verte
const int PIN_TX= 6;     //Pin RFID
const int PIN_RX= 7;     //  ----
const int PIN_ECHO= 4;   //Pin US
const int PIN_TRIG= 3;   //    ----

//-----déclaration des variable spour le RFID-----
SoftwareSerial SoftSerial(PIN_TX, PIN_RX);
unsigned char buffer[64];         // buffer array for data receive over serial port
int count = 0;                    // counter for buffer array
String carte;
String carteMem;

//-----déclaration des variables pour les US-----
int trig = PIN_TRIG; 
int echo = PIN_ECHO; 
long lecture_echo; 
long cm;

//-----déclaration des variable spour le WiFi-----
String MY_IP;
String MY_SSID;
String MY_PWD;
ESP8266WebServer server(80); // Serveur web


//-----déclaration des variables pour les LED-----
const int rouge = PIN_LED_R;
const int vert = PIN_LED_V;

//-----déclaration des variables pour la lecture de la page HTML-----
String tabID[100]; //tableau contenant les ID RFID valide (max 100 ID)
File fichierSD;
String str="";

void setup()
{
  //Set Up LED
    pinMode(rouge, OUTPUT);
    pinMode(vert, OUTPUT);
    digitalWrite(rouge, LOW);
    digitalWrite(vert, LOW);
  
  //Set Up Carte SD
    if(!SD.begin(4)) {  //4= la pin de connexion (optionnel, dépend de où on la branche je crois)
        Serial.println(F("Initialisation impossible !"));
        return;
    }
    Serial.println(F("Initialisation OK"));  

    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    
    if (!SD.exists("BDD.txt")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find data base file
    }
    Serial.println("SUCCESS - Found BDD.txt file.");
    recupBDD_SD();
    
 //SetUp Wifi
    connect_AP("Jarvis","12341234"); // Mettre le n° de votre groupe
    
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to your ESP
    server.on ("/", handleRoot );
    server.on("/readWeb", handleAff);
    server.on("/setTXT", handleId);
    server.begin();
    
    Serial.println ( "Serveur http démarré" );
    
    SoftSerial.begin(9600);     // the SoftSerial baud rate
    Serial.begin(9600);         // the Serial port of Arduino baud rate.
}
 
void loop(){
    server.handleClient();
}

//-----fonction liant les différentes fonctions et affichant les valeures
void affichage(){
  str="";       
  if(!distanceUS()){      //cas où il n'y a pas de véhicule
        str="Pas de véhicule sur la place"; 
        
    }

  else{                   //un véhiucle est détecté 
    str+="Véhicule détecté sur la place \nEn attente d'identification "; 
    //server.send(200, "text/plane", str); 
    digitalWrite(rouge, HIGH);
    if(compare()){
        str="Autorisation validée \nID de l'utilisateur : " +carte; // afficheage réussite dans la page web
        digitalWrite(rouge, LOW);
        digitalWrite(vert, HIGH);
        while(distanceUS()){//met le programme en pause tant que la voiture est en place 
        }
    }
    else{
        str="Autorisation refusée, carte non reconnue \nID de la : " +carte; //affichage erreur dans la page web                   
    }
  } 
}

//-----connexion à un réseau wifi via partage de co
void connect_AP(const char *ssid,const char *password ){
  delay(500);
  WiFi.mode(WIFI_AP);
  WiFi.softAP( ssid, password ); //8 caractères uniquement !
  Serial.print ( "Connected to access point : " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  MY_IP=WiFi.softAPIP().toString();
  MY_SSID=ssid;
  MY_PWD=password;
  Serial.println ( WiFi.softAPIP() );
}

//-----fonction de validation de la carte RFID
boolean compare(){
  String temp=idCarte();
  for(int i=0; i<sizeof(tabID); i++){
    if(tabID[i]==temp){
      return true;
    }
  }
  return false;
}

//-----détection de la présence d'un véhicule sur la place de parking/à proximité de la borne
boolean distanceUS(){ 
  digitalWrite(trig, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(trig, LOW); 
  lecture_echo = pulseIn(echo, HIGH); 
  cm = lecture_echo / 58;
  Serial.print("Distance cm : "); //faire une fonction d'affichage dans al page HTML
  Serial.println(cm); 
  
  if (cm<=distanceMin){
    return true;
  }
  
  return false;
}

//-----fonction retournant un String contenant la valeur HEX de la carte
String idCarte(){ 
  // if date is coming from software serial port ==> data is coming from SoftSerial shield
    if (SoftSerial.available())              
    {
        while(SoftSerial.available())               // reading data into char array
        {
            buffer[count] = SoftSerial.read();      // writing data into array
            if(count == 14)break;
            carte=carte + char(buffer[count]);
            count++;
        }
        
        Serial.write(buffer, count);     // if no data transmission ends, write buffer to hardware serial port
        Serial.print(carte);
        Serial.print("go");
        carteMem=carte;
        carte="";

        //test affichage
//        if(carte=="120096AADAF4"){
//          digitalWrite(led, HIGH);
//          delay(2000);
//          digitalWrite(led, LOW);
//          delay(2000);
//        }
        
        clearBufferArray();             // call clearBufferArray function to clear the stored data from the array
        count = 0;                      // set counter of while loop to zero
        
    }
    if (Serial.available())             // if data is available on hardware serial port ==> data is coming from PC or notebook
    SoftSerial.write(Serial.read());    // write it to the SoftSerial shield
    delay(1000);

  //Mise en forme du Sting à renvoyer
    carteMem.remove(0,1);
    carteMem.remove(carteMem.length()-1,1);

    return carteMem;
}

//-----function to clear buffer array
void clearBufferArray(){
    // clear all index of array with command NULL
    for (int i=0; i<count; i++)
    {
        buffer[i]=NULL;
    }                  
}

//-----fonction pour récupérer les infos d'un fichier txt dans un tableau (BDD)
void recupBDD_SD(){
  fichierSD = SD.open("BDD.txt", FILE_READ);           //Ouverture du fichier
   if(fichierSD){                                       //Test pour écriture
      String nouveauChiffre = "";                       //variable qui va contenir ce qu'on lit
        int x = 0;   
   
        while (fichierSD.available()){                  //tant que le fichier contient qqchose
        
            char fileChar = (char)fichierSD.read();
            if (fileChar == '\n'){                      //tant qu'on arrive pas en bout de ligne dans le fichier texte
            
                tabID[sizeof(tabID)]=nouveauChiffre; 
                nouveauChiffre = "";                    //onvite la varible  
            }
            else{
                if (fileChar >= ' ') {                  // >= ' ' pour supprimer tout ce qui est < que espace.
                  nouveauChiffre += fileChar;
                }        
            }   
        }
        fichierSD.close();
    }
}

void resetBDD(){    //réécrit les nouveaux ID dans le fichier de stockage
  SD.remove("BDD.txt");
  fichierSD = SD.open("BDD.txt", FILE_WRITE);  //test.txt is the file name to be created and FILE_WRITE is a command to create file.
  for (int i=0; i<sizeof(tabID)-1; i++){
    fichierSD.println(tabID[i]+"\n");
  }
  fichierSD.close();
}

/////////////////////////////
//-----Server Handeling---- PAS TOUCHER
void handleAff() {
   server.send(200, "text/plane", str); //Send ADC value only to client ajax request
}

void handleId() {
   String Id = server.arg("TEXT"); //Send ADC value only to client ajax request
   tabID[sizeof(tabID)-1]=Id;
   resetBDD();
}

void handleRoot() {
  fichierSD = SD.open("index.htm");        // open web page file
  if (fichierSD) {
      while(fichierSD.available()) {
          server.send(fichierSD.read()); // send web page to client
      }
      fichierSD.close();
  }
}
