#include <SoftwareSerial.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;
SoftwareSerial gsm(-1,2);

//within uclm
double uclmselat = 10.324460056559674;//d, 
double uclmselon = 123.95333240165124;

double uclmswlat= 10.325344281762012;//c, 
double uclmswlon= 123.95284449310348;

double uclmnelat =10.325409966963566;//b 
double uclmnelon = 123.9544597271905;

double uclmnwlat =10.325973343321055;//a,, 
double uclmnwlon = 123.95401547361807;

//canteenA
double canteenAselat= 10.324840036348292; 
double canteenAselon=  123.9534120486306;

double canteenAswlat= 10.324963748122045; 
double canteenAswlon= 123.95330938098174;

double canteenAnelat =10.324983322765325;
double canteenAnelon =  123.95355928518126;

double canteenAnwlat= 10.325129741058486;
double canteenAnwlon=  123.95345104641969; 

//canteenB
double canteenBselat =10.325584387381369;
double canteenBselon = 123.95390155224244;

double canteenBswlat =10.325703400958865;
double canteenBswlon =  123.95379649697385;

double canteenBnelat =10.32566425176071;
double canteenBnelon = 123.95399944465179;

double canteenBnwlat =10.32577935038939;
double canteenBnwlon = 123.95390393986216; 

//canteenC
double canteenCselat =10.325247480778208;
double canteenCselon = 123.95409615143923;

double canteenCswlat =10.325413723476167;
double canteenCswlon = 123.95395801767471;

double canteenCnelat = 10.325326643978686;
double canteenCnelon = 123.95425842508492;

double canteenCnwlat =10.325520593735595;
double canteenCnwlon = 123.9541632066647;

//crA
double crAselat =10.32567599652067;
double crAselon = 123.95402730021543;

double crAswlat =10.3257950100635;
double crAswlon = 123.95392065320033; 

double crAnelat =10.325771520552049;
double crAnelon =  123.95414270183622;

double crAnwlat =10.325894448975932; 
double crAnwlon = 123.9540519722861;

//classrooms
double roomselat =10.325265442893128; 
double roomselon =  123.95351392133034;

double roomswlat = 10.325473905932919; 
double roomswlon =  123.95337310536918;

double roomnelat =10.325548451229006;
double roomnelon =   123.95387601952804;

double roomnwlat =10.325728547308817; 
double roomnwlon =   123.953749285163;

//cbe tall bldg
double cbeselat =10.326643976089372; 
double cbeselon =123.95541021437658;

double cbeswlat =10.326814615052882;  
double cbeswlon = 123.95520207610987;

double cbenelat =10.32761242941482; 
double cbenelon =  123.95613760952033;

double cbenwlat = 10.327758297753105;
double cbenwlon =  123.95591286261212;

//annex
double anxselat = 10.324816972910437; 
double anxselon =123.9538396873969;

double anxswlat =10.32491803841909;  
double anxswlon =  123.9537342727157;

double anxnelat =10.32513404106398; 
double anxnelon =  123.95411833129936; 

double anxnwlat = 10.325223002960268;
double anxnwlon =  123.95401506075318; 

bool sentMessage = false, timeLimit = false;
bool buzzed = false, hasStarted = false;
bool canteen1=false, canteen2=false, canteen3=false, cr=false, cbe=false, outside=false, inside=false, annex=false, isBack = false;

unsigned long currmillis;

String loc = "location not fetched";
//end of definition

void setup() {
  digitalWrite(0, LOW);
  Serial.begin(9600);
  Serial.println("helo0");//init serial
  pinMode(0,OUTPUT);
  noTone(0);

  if(hasStarted==false){//send sms that device has started
    SendStarted();
  }
  
  delay(2000);
  Serial.println("hello");
}

void loop() {
  currmillis = millis();//set timer
  if(currmillis>=((15*60000)-20)){//comms with gps module after 15 mins (15*60000)-20
    timeLimit=true;
    CheckLatLon();
  }else{//check location of device always
    CheckLatLon();
    //Serial.println(currmillis);//print curr millis till 900000
  }
}

void buzzer(){// play buzzer
  int i =0;
  delay(1000);
  while(i<120){
    tone(0,1000);
    delay(100);
    tone(0,500);
    delay(100);
    i++;
  }
  noTone(0);
  buzzed = true;
}

void CheckLatLon(){//get and decode gps data
  while (Serial.available() > 0) {
    double latitude, longitude;
    gps.encode(Serial.read());
    if (gps.location.isUpdated()) {
      Serial.print("Latitude: ");
      latitude = gps.location.lat();
      Serial.print(latitude, 6);
      Serial.print(", Longitude: ");
      longitude = gps.location.lng();
      Serial.println(longitude, 6);
      ConfirmLoc(latitude, longitude);
    }
  }
}

void ConfirmLoc(double latitude, double longitude){//check fetched location if it is inside any of the areas set


  if(IsInsideArea(latitude, longitude, canteenAnwlat, canteenAnwlon, canteenAnelat, canteenAnelon, canteenAswlat, canteenAswlon, canteenAselat, canteenAselon)){
    loc = "Is in the Canteen 1";
    if(canteen1==false){
      isBack=false;
      canteen1=true;
      canteen2=false;
      canteen3=false;
      outside=false;
      inside=false;
      cr=false;
      annex=false;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, canteenBnwlat, canteenBnwlon, canteenBnelat, canteenBnelon, canteenBswlat, canteenBswlon, canteenBselat, canteenBselon)){
    loc = "Is in the Canteen 2";
    if(canteen2==false){
      canteen2=true;
      canteen1=false;
      canteen3=false;
      isBack=false;
      outside=false;
      inside=false;
      annex=false;
      cr=false;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, canteenCnwlat, canteenCnwlon, canteenCnelat, canteenCnelon, canteenCswlat, canteenCswlon, canteenCselat, canteenCselon)){
    loc = "Is in the Canteen 3";
    if(canteen3==false){
      canteen2=false;
      canteen1=false;
      canteen3=true;
      annex=false;
      isBack=false;
      cr=false;
      outside=false;
      inside=false;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, crAnwlat, crAnwlon, crAnelat, crAnelon, crAswlat, crAswlon, crAselat, crAselon)){
    loc = "Is in the CR";
    if(cr==false){
      canteen2=false;
      canteen1=false;
      canteen3=false;
      isBack=false;
      annex=false;
      outside=false;
      inside=false;
      cr=true;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, cbenwlat, cbenwlon, cbenelat, cbenelon, cbeswlat, cbeswlon, cbeselat, cbeselon)){
    loc = "Is in the CBE building (Warning: Tall Building Height)";
    if(cr==false){
      canteen2=false;
      canteen1=false;
      annex=false;
      canteen3=false;
      cr=true;
      outside=false;
      inside=false;
      isBack=false;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, anxnwlat, anxnwlon, anxnelat, anxnelon, anxswlat, anxswlon, anxselat, anxselon)){
    loc = "Is in the Basic Education Bldg.";
    if(cr==false){
      canteen2=false;
      canteen1=false;
      canteen3=false;
      annex=true;
      isBack=false;
      cr=false;
      outside=false;
      inside=false;
      cbe=false;
      SendMessage();
    }
  }else if(IsInsideArea(latitude, longitude, roomnwlat, roomnwlon, roomnelat, roomnelon, roomswlat, roomswlon, roomselat, roomselon)){
    //back in classroom, reset timer and location
    //if(canteen1==true||canteen2==true||canteen3==true||cr==true||cbe==true||outside==true){
    loc = "Is near classroom";
    isBack=true;
    canteen2=false;
    canteen1=false;
    canteen3=false;
    annex=false;
    inside=false;
    outside=false;
    cr=false;
    cbe=false;
    buzzed=false;
    timeLimit=false;
    currmillis = 0;
    if(isBack=false){
      SendMessage();
    }
    //}
    Serial.println(currmillis);
  }else if(!IsInsideArea(latitude, longitude, uclmnwlat, uclmnwlon, uclmnelat, uclmnelon, uclmswlat, uclmswlon, uclmselat, uclmselon)){
    loc = "Is outside of UCLM";
    if(outside==false){
      canteen2=false;
      canteen1=false;
      canteen3=false;
      cr=false;
      annex=false;
      cbe=false;
      inside=false;
      isBack=false;
      outside=true;
      SendMessage();  
    }
  }else if(IsInsideArea(latitude, longitude, uclmnwlat, uclmnwlon, uclmnelat, uclmnelon, uclmswlat, uclmswlon, uclmselat, uclmselon)){
    loc = "Is within UCLM";
    if(inside==false&&canteen1==false&&canteen2==false&&canteen3==false&&cr==false&&cbe==false&&annex==false&&outside==false){
      canteen2=false;
      canteen1=false;
      canteen3=false;
      cr=false;
      isBack=false;
      outside=false;
      cbe=false;
      annex=false;
      inside=true;
      SendMessage();
    }
  }

  if(timeLimit==true){//if timer ran out, send message
    SendMessage();
  }
}



bool IsInsideArea(double lat, double lon, 
double vertex1Lat, double vertex1Lon, 
double vertex2Lat, double vertex2Lon,
double vertex3Lat, double vertex3Lon,
double vertex4Lat, double vertex4Lon
) {//if device is inside the area checker
    Serial.println("isInsideFunc");
    Serial.println(lat, 6);
    Serial.println(lon, 6);
    int intersections = 0;
    if ((lat >= vertex1Lat && lat <= vertex2Lat) || (lat >= vertex2Lat && lat <= vertex1Lat)) {
        double slope1 = (vertex2Lon - vertex1Lon) / (vertex2Lat - vertex1Lat);
        double intercept1 = vertex1Lon - slope1 * vertex1Lat;
        double intersection1 = slope1 * lat + intercept1;
        if (lon <= intersection1) {
            intersections++;
        }
    }
    if ((lat >= vertex3Lat && lat <= vertex4Lat) || (lat >= vertex4Lat && lat <= vertex3Lat)) {
        double slope2 = (vertex4Lon - vertex3Lon) / (vertex4Lat - vertex3Lat);
        double intercept2 = vertex3Lon - slope2 * vertex3Lat;
        double intersection2 = slope2 * lat + intercept2;
        if (lon >= intersection2) {
            intersections++;
        }
    }
    
    return (intersections % 2 != 0);
}

void SendStarted(){//send sms that device has started
  delay(3000);
  gsm.begin(9600);//start gsm module
  delay(3000);
  Serial.println("GSM BEGIN");
  hasStarted = true;
  delay(1000);

  gsm.println("AT+CMGF=1\r");    
  delay(500);  

  gsm.println("AT+CMGS=\"+639055574398\"\r"); //639055574398
  delay(500);

  gsm.println("HALLPASS01: 15 mins. countdown has started!");
  delay(500);

  gsm.write((char)26);
  delay(1000);

  delay(2000);
  gsm.end();
}

void SendMessage(){//send sms update of device's location
  Serial.println(loc);
  delay(3000);
  gsm.begin(9600);//start gsm module
  delay(3000);
  Serial.println("GSM BEGIN");
  delay(1000);

  gsm.println("AT+CMGF=1\r");    
  delay(500);  

  gsm.println("AT+CMGS=\"+639055574398\"\r"); //639055574398
  delay(500);

  if(timeLimit==true){
    gsm.println("HALLPASS01: Time limit of 15 mins. has been reached!");
    delay(500);
  }

  gsm.println();
  gsm.println("Student's current/last fetched location:");
  delay(500);

  gsm.println(loc);
  delay(500);

  gsm.write((char)26);
  delay(1000);

  delay(2000);
  gsm.end();
  if(timeLimit==true&&buzzed == false){
   buzzer();
  }
  
}