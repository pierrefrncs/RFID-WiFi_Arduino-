/* 
author: Pierre Francois 

This program was developped and supposed to work on the augmented 
Arduino 2.6 IDE by DUINO EDU, on a ESP8266 D1 board
 
*/
#include <SoftwareSerial.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>



#define distanceMin 100 //detecting distance 

const int PIN_LED_R =8;  //Red LED pin 
const int PIN_LED_V =9;  //Green LED pin
const int PIN_TX= 6;     //RFID Pin
const int PIN_RX= 7;     //  ----
const int PIN_ECHO= 4;   //Ultra sound Pin
const int PIN_TRIG= 3;   //    ----

//----- variables for the RFID reader -----
SoftwareSerial SoftSerial(PIN_TX, PIN_RX);
unsigned char buffer[64];         // buffer array for data receive over serial port
int count = 0;                    // counter for buffer array
String carte;
String carteMem;

//----- variables for the US module -----
int trig = PIN_TRIG; 
int echo = PIN_ECHO; 
long lecture_echo; 
long cm;

//-----variables for the WiFi server-----
String MY_IP;
String MY_SSID;
String MY_PWD;
ESP8266WebServer server(80); // creates web server  


//-----variables for the LED pins-----
const int rouge = PIN_LED_R;
const int vert = PIN_LED_V;

//-----variables for the SD card reading and writting -----
String tabID[100]; //array containing IDs of the RFID tags (max 100 tags stored)
File fichierSD;
String str="";

void setup()
{
  //Set Up LED
    pinMode(rouge, OUTPUT);
    pinMode(vert, OUTPUT);
    digitalWrite(rouge, LOW);
    digitalWrite(vert, LOW);
  
  //Set Up SD card
    if(!SD.begin(4)) {  //4= command Pin  
        Serial.println(F("Initialisation impossible !"));
        return;
    }
    Serial.println(F("Initialisation OK"));  
/* If you want to store the HTML code of the web page on the SD card and not directly in this code
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
*/    
    if (!SD.exists("BDD.txt")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find data base file
    }
    Serial.println("SUCCESS - Found BDD.txt file.");
    recupBDD_SD();
    
 //SetUp Wifi
    connect_AP("Jarvis","12341234"); // put your (SSID, password)
    
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());  //IP address assigned to your ESP
    server.on ("/", handleRoot );
    server.on("/readWeb", handleAff);
    server.on("/setTXT", handleId);
    server.begin();
    
    Serial.println ( "HTTP server started" );
    
    SoftSerial.begin(9600);     // the SoftSerial baud rate
    Serial.begin(9600);         // the Serial port of Arduino baud rate.
}
 
void loop(){
    server.handleClient();
}

//-----function to display value of the différents components 
void affichage(){
  str="";       
  if(!distanceUS()){      //no vehicules detected
        str="Pas de véhicule sur la place"; 
        
    }

  else{                   //vehicule detected 
    str+="vehicule detected on the parking spot  \nwaitting for identification "; 
   
    digitalWrite(rouge, HIGH);
    if(compare()){ //displaying user infos on the web page
        str="Autorisation granted \nUser ID : " +carte;
        digitalWrite(rouge, LOW);
        digitalWrite(vert, HIGH);
        while(distanceUS()){//pause the program while the car is on the spot
        }
    }
    else{
        str="Autorisation refusée, carte non reconnue \nID de la : " +carte; //displaying an error message on the web page                    
    }
  } 
}

//-----connecting to the network
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

//-----validation of the ID from the RFID tag 
boolean compare(){
  String temp=idCarte();
  for(int i=0; i<sizeof(tabID); i++){
    if(tabID[i]==temp){
      return true;
    }
  }
  return false;
}

//-----detecting the presence of a vehicule on the parking spot 
boolean distanceUS(){ 
  digitalWrite(trig, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(trig, LOW); 
  lecture_echo = pulseIn(echo, HIGH); 
  cm = lecture_echo / 58;
  Serial.print("Distance cm : "); 
  Serial.println(cm); 
  
  if (cm<=distanceMin){
    return true;
  }
  
  return false;
}

//-----getting the HEX value stored on the RFID card 
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

        //test display 
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

  //formatting the obtained string to fit our needs
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

//-----get the txt information from our database (SD card here) to get the already existing users IDs 
void recupBDD_SD(){
  fichierSD = SD.open("BDD.txt", FILE_READ);            //Open the file
   if(fichierSD){                                       //test for writting
      String nouveauChiffre = "";                       //readed content 
        int x = 0;   
   
        while (fichierSD.available()){                  //while the file contains something 
        
            char fileChar = (char)fichierSD.read();
            if (fileChar == '\n'){                      //while the line is not empty
            
                tabID[sizeof(tabID)]=nouveauChiffre; 
                nouveauChiffre = "";                    //emptying our variable
            }
            else{
                if (fileChar >= ' ') {                  // >= ' ' to suppress everything < the space
                  nouveauChiffre += fileChar;
                }        
            }   
        }
        fichierSD.close();
    }
}

void resetBDD(){    //write the new tags on the txt file in our SD card 
  SD.remove("BDD.txt");
  fichierSD = SD.open("BDD.txt", FILE_WRITE);  //test.txt is the file name to be created and FILE_WRITE is a command to create file.
  for (int i=0; i<sizeof(tabID)-1; i++){
    fichierSD.println(tabID[i]+"\n");
  }
  fichierSD.close();
}

/////////////////////////////
//-----Server Handeling----
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
