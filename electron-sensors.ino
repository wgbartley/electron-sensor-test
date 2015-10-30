#include "application.h"
#include "elapsedMillis.h"
#include "PietteTech_DHT.h"
#include "thermistor-library.h"
#include "PietteTech_DHT.h"

#define SLEEP_DURATION 9*60 // Sleep for 9 minutes

PMIC PMIC;
FuelGauge fuel;

double fuelSOC = 0;
double fuelVcell = 0;

elapsedMillis dht_timer;
elapsedMillis pub_timer;
elapsedMillis fuel_timer;
elapsedMillis therm_timer;
elapsedMillis ldr_timer;

uint16_t dht_interval = 1000*2;
uint32_t pub_interval = 1000*60*5;
uint16_t fuel_interval = 1000*5;
uint16_t therm_interval = 100*5;
uint16_t ldr_interval = 1000*5;

String device_name = "E3A";

void dht_wrapper();
PietteTech_DHT DHT(D2, DHT22, dht_wrapper);
bool dhtStarted = false;
String dhtError = "";
uint32_t dhtTimestamp = 0;
double dhtFahrenheit = 0;
double dhtHumidity = 0;
double dhtDewPoint = 0;
double thermistorF = 0;
uint8_t ldrValue = 0;

Thermistor Thermistor(A0, 10000);


void setup() {
    // https://gist.github.com/brandoaire/68d639903d4984c3bd7c
    PMIC.begin();
    PMIC.disableWatchdog();
    PMIC.setInputCurrentLimit(500);

    Serial.begin(115200);
    Thermistor.begin();

    pinMode(A1, INPUT);

	dht_timer = 0;
	pub_timer = pub_interval-15000;
	fuel_timer = fuel_interval;
	therm_timer = therm_interval;
	ldr_timer = ldr_interval;
}


void loop() {
    PMIC.setInputCurrentLimit(500);

	doDHT22();
	doFuel();
	doThermistor();
	doPhotoresistor();
	doPub();
}


void doDHT22() {
    if(dht_timer>dht_interval) {
        if(!dhtStarted) {
            DHT.acquire();
            dhtStarted = true;
        }

        if(!DHT.acquiring()) {
            int dhtResult = DHT.getStatus();

            if(dhtResult==DHTLIB_OK) {
                dhtTimestamp = Time.now();

                dhtHumidity = DHT.getHumidity();
                dhtFahrenheit = DHT.getFahrenheit();
                dhtDewPoint = DHT.getDewPoint();
            }

            dhtStarted = false;
        }

        dht_timer = 0;
    }
}


void doFuel() {
    if(fuel_timer>fuel_interval) {
        fuelSOC = fuel.getSoC();
        fuelVcell = fuel.getVCell();

        fuel_timer = 0;
    }
}


void doThermistor() {
    if(therm_timer>therm_interval) {
        thermistorF = Thermistor.getTempF(false);
        therm_timer = 0;
    }
}


void doPhotoresistor() {
    if(ldr_timer>ldr_interval) {
        uint16_t val = analogRead(A1);
        ldrValue = map(val, 0, 4095, 0, 100);

        ldr_timer = 0;
    }
}


void doPub() {
	if(pub_timer>pub_interval) {
	    RGB.control(true);
	    RGB.color(255, 0, 0);

	    String pub = device_name+";h:"+String((float)dhtHumidity, 2)+"|g,f:"+String((float)dhtFahrenheit, 2)+"|g";
    	pub += ",f2:"+String((float)thermistorF, 2)+"|g";
    	pub += ",l:"+String(ldrValue)+"|g";
    	pub += ",soc:"+String(fuelSOC, 2)+"|g,vc:"+String(fuelVcell, 2)+"|g";

        // Skip this publish if the DHT22 reading is exactly 0
	    if((uint8_t)dhtHumidity==0) {
	        pub_timer = 45000;
	        Serial.print("-  ");
	    } else {
    	    if(Particle.publish("statsd", pub, 60, PRIVATE))
    	        Serial.print("+  ");
    	    else
    	        Serial.print("-- ");
	    }

    	Serial.println(pub);

    	RGB.control(false);

    	//delay(10);
    	//Particle.process();

    	//pub = device_name+";soc:"+String(fuelSOC, 2)+"|g,vc:"+String(fuelVcell, 2)+"|g";
    	//Particle.publish("statsd", pub, 60);

    	pub_timer = 0;
    	
    	
    	System.sleep(SLEEP_MODE_DEEP, SLEEP_DURATION); // Sleep!
	}
}


void dht_wrapper() {
    DHT.isrCallback();
}