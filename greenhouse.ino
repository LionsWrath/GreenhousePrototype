#include <DS3231.h>
#include <DHT.h>

#define NOTHING 0

#define DHTTYPE DHT11
#define DHTPIN 3

#define WATERPIN 4
#define LIGHTPIN 5
#define SOILPIN 0

#define MAXALARMS 100

#define ONLIGHT 4
#define OFFLIGHT 3
#define ONIRRIGATE 2
#define OFFIRRIGATE 1

DHT dht(DHTPIN, DHTTYPE);
DS3231 rtc(SDA, SCL);

//-------------------------------------Watering

void turnOnIrrigation() {
  Serial.println("IRRIGATING. . .");
  digitalWrite(WATERPIN, HIGH);
}

void turnOffIrrigation() {
  Serial.println("DONE\n");
  digitalWrite(WATERPIN, LOW);
}

//--------------------------------- Ilumination

void turnOnLight() {
  Serial.println("Turn ON LIGHT");
  digitalWrite(LIGHTPIN, HIGH);
}

void turnOffLight() {
  Serial.println("Turn OFF LIGHT");
  digitalWrite(LIGHTPIN, LOW);  
}

//-----------------------------------------RTC

struct Alarm {
  unsigned DOW;
  unsigned Hour, Minute, Second;
  unsigned action;

  Alarm() {}
  Alarm(unsigned d, unsigned h, unsigned m, unsigned s, unsigned a) {
    this->DOW = d;
    this->Hour = h;
    this->Minute = m;
    this->Second = s;
    this->action = a;
  }
  
  Alarm(unsigned h, unsigned m, unsigned s, unsigned a) {
    this->DOW = NOTHING;
    this->Hour = h;
    this->Minute = m;
    this->Second = s;
    this->action = a;
  }

  bool operator==(const Alarm &Other) const {
    if (this->DOW == Other.DOW)
      if (this->Hour == Other.Hour) 
        if (this->Minute == Other.Minute) 
          if (this->Second == Other.Second)
            return true;
    return false;
  }
  
  bool operator==(const Time &Other) const {
    if (this->DOW == Other.dow)
      if (this->Hour == Other.hour) 
        if (this->Minute == Other.min) 
          if (this->Second <= Other.sec)
            return true;
    return false;
  }
};

int numberOfAlarms;
Alarm alarms[MAXALARMS];

bool addAlarm(unsigned h, unsigned m, unsigned s, unsigned a) {
  Alarm alarm(h, m, s, a);

  if (numberOfAlarms < MAXALARMS) {
    alarms[numberOfAlarms] = alarm;
    numberOfAlarms++;
    return true;
  }
  return false;
}

void addLightAlarm(unsigned hi, unsigned mi, unsigned si, unsigned hf, unsigned mf, unsigned sf) {
  addAlarm(hi, mi, si, ONLIGHT);
  addAlarm(hf, mf, sf, OFFLIGHT);
}

void addIrrigateAlarm(unsigned hi, unsigned mi, unsigned si, unsigned hf, unsigned mf, unsigned sf) {
  addAlarm(hi, mi, si, ONIRRIGATE);
  addAlarm(hf, mf, sf, OFFIRRIGATE);
}

void removeAlarm(unsigned idx) {
  for (int i=0; i<numberOfAlarms-1; i++) {
    if (i >= idx) 
      alarms[i] = alarms[i+1];  
  }
  
  numberOfAlarms--;
}

bool removeAllAlarms() {
  numberOfAlarms = 0;
}

void activateAction(unsigned idx) {
  switch (alarms[idx].action) {
    case ONLIGHT:
      turnOnLight();
      break;
    case OFFLIGHT:
      turnOffLight();
      break;
    case ONIRRIGATE:
      turnOnIrrigation();
      break;
    case OFFIRRIGATE:
      turnOffIrrigation();
      break;  
  }
}

void verifyAlarm(unsigned idx) {
  Time actual = rtc.getTime();

  if (numberOfAlarms < idx) return;
  if (alarms[idx] == actual) {
    activateAction(idx);
    removeAlarm(idx);
  }
}

void verifyAllAlarms() {
  for (int i=0; i<numberOfAlarms; i++) {
    verifyAlarm(i);
  }  
}

//--------------------------------------------

void printStatus() {
  Serial.print("Number of Alarms: ");
  Serial.println(numberOfAlarms);
  Serial.println(rtc.getTimeStr());
}

void setup() {
  Serial.begin(9600);
  
  dht.begin();
  rtc.begin();

  pinMode(WATERPIN, OUTPUT);
  pinMode(LIGHTPIN, OUTPUT);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float s = analogRead(SOILPIN);

  Serial.print(" S ");
  Serial.print(int(s));
  Serial.print(" H ");
  Serial.print(int(h));
  Serial.print(" T ");
  Serial.println(int(t));
  
  verifyAllAlarms();

  unsigned n[3], o[3];
  while(Serial.available()) {
    char a = Serial.read();
    switch (a) {
      case 'l':
        n[0] = Serial.parseInt();
        n[1] = Serial.parseInt();
        n[2] = Serial.parseInt();
    
        o[0] = Serial.parseInt();
        o[1] = Serial.parseInt();
        o[2] = Serial.parseInt();
          
        addLightAlarm(n[0], n[1], n[2], o[0], o[1], o[2]);
        Serial.println("New Light Alarm!");
        break;
      case 'i':
        n[0] = Serial.parseInt();
        n[1] = Serial.parseInt();
        n[2] = Serial.parseInt();
    
        o[0] = Serial.parseInt();
        o[1] = Serial.parseInt();
        o[2] = Serial.parseInt();

        addIrrigateAlarm(n[0], n[1], n[2], o[0], o[1], o[2]);
        Serial.println("New Irrigation Alarm!");
        break;
      case 'r':
        removeAllAlarms();
        break;
      case 's':
        printStatus();
        break;
    }
  }

  delay(500);
}
