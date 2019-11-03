#include <ESP8266WiFi.h>
#include <PubSubClient.h>

ADC_MODE(ADC_VCC);
#define DATAPIN 3


/*** SLEEPTIME is the interval between two data collections, 1 second = 1,000,000***/
/*** 50ms per reading ***/
/*** 5000ms per sending cycle 60000000***/ 
#define SLEEPTIME 60000000

/*** CollectSize is the number of data points to collect before sending to cloud 60 ***/ 
#define CollectSize 120

/*** Each data point is a bit, ByteSize is the number of bytes needed***/
const int ByteSize = int((CollectSize + 7) / 8);
/*** HEXSTR is the HEX char representation of the value bytes, need two char for each byte, plus one extra for NULL ending***/
char HEXSTR[ByteSize * 2 + 1];

typedef struct {
	int CollectPointer;
	byte values[ByteSize];
} rtcStoredInfo;

rtcStoredInfo rtcValues;


const byte rtcStartAddress = 64;





/************ WIFI and MQTT Information (CHANGE THESE FOR YOUR SETUP) ******************/
const char* ssid = "xxxxxx"; //type your WIFI information inside the quotes
const char* password = "xxxxxx";
const char* mqtt_clientid = "furnace";
const char* mqtt_server = "192.168.0.101";
const char* mqtt_username = "sensor_ge";
const char* mqtt_password = "***REMOVED***";
const int mqtt_port = 1883;
const char* topic = "sensor/furnace";
const char* topic2 = "sensor/Vfurnace";
const char* topic4 = "sensor/status";

WiFiClient espClient;
PubSubClient client(espClient);

extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}


void setup() {

	//mark starting cycle time, cycle time will be subtracted from next sleep time
	unsigned long time_spent = micros();

	delay(5);
	//Serial.begin(115200);
	Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
	pinMode(DATAPIN, INPUT_PULLUP);
	Serial.println();
	
	rst_info* ri = system_get_rst_info();
	if (ri == NULL) return ;
	switch (ri->reason) {
	case 5:
		system_rtc_mem_read(rtcStartAddress, &rtcValues, sizeof(rtcValues));
		break;
	case 6:	
  case 0:	
		//WiFi.disconnect();
    send_status();
		WiFi.mode(WIFI_OFF);
		reset_all();	
		system_rtc_mem_write(rtcStartAddress, &rtcValues, sizeof(rtcValues));
		break;
	}

	Serial.println("next pointer ");
	Serial.println(rtcValues.CollectPointer);


	probe();	
	if (rtcValues.CollectPointer >= CollectSize)
	{
		Serial.print("send data ");
		send_data();
		delay(100);
		reset_all();
	}
	system_rtc_mem_write(rtcStartAddress, &rtcValues, sizeof(rtcValues));


	time_spent = (unsigned long)(micros() - time_spent);
	Serial.print("Time spent: ");
	Serial.println(time_spent);
	//do not adjust if time_spent is too long
	if ((time_spent<0) ||  (time_spent>= SLEEPTIME))
	{
		time_spent = 0;
	}	


	if (rtcValues.CollectPointer == CollectSize -1)
	{
		//Serial.println("enable wifi in next call");
		ESP.deepSleep((unsigned long)SLEEPTIME - time_spent, RF_CAL);
	}
	else
	{
		//Serial.println("disable wifi in next call");
		ESP.deepSleep((unsigned long)SLEEPTIME - time_spent, RF_DISABLED);
	}	
}


void loop() {}





void probe ()
{
	//Read data
	int transitions = 0;
	int lastreading = digitalRead(DATAPIN);
	delay(2);
	for (int i = 0; i < 17; i++)
	{
		int thisreading = digitalRead(DATAPIN);
		if (thisreading != lastreading)
		{
			transitions++;
			lastreading = thisreading;
		}
		delay(2);
	}

	if (transitions >= 4)
	{
		int byteindex = rtcValues.CollectPointer / 8;
		byte bitindex = rtcValues.CollectPointer % 8;
		bitSet(rtcValues.values[byteindex], bitindex);
	}

	rtcValues.CollectPointer++;
}


void send_data()
{
	// We start by connecting to a WiFi network
	WiFi.mode(WIFI_STA);
	delay(20);

	//Serial.print("Connecting to ");
	//Serial.print(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);


	int t = 0;
	while (WiFi.status() != WL_CONNECTED) {
		t++;
		//Serial.print(".");
		if (t>15)
		{
			//Serial.println("Wifi failed");
			return;
		}
		delay(1000);
	}
	//Serial.println("Wifi in");
	client.setServer(mqtt_server, mqtt_port);
	t = 0;
	while (!client.connected()) {
		client.connect(mqtt_username, mqtt_username, mqtt_password);
		t++;
		if (t>10)
		{
			return;
		}
		delay(250);
	}
	
	int ptr = 0;
	for (int i = 0; i < ByteSize; i++)
	{
		ptr += snprintf(HEXSTR + ptr, sizeof(HEXSTR) - ptr, "%02x", rtcValues.values[i]);

	}
	Serial.println(HEXSTR);
	client.publish(topic, HEXSTR, 0);

	int vcc = ESP.getVcc();
	char buffer[4];
	itoa(vcc, buffer, 10);
	client.publish(topic2, buffer, 1);
  delay(100);
	//Serial.println(vcc);
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
}

void send_status()
{
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  delay(20);
  WiFi.begin(ssid, password);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED) {
    t++;
    Serial.print(".");
    if (t > 15)
    {
      Serial.println("Wifi failed");
      return;
    }
    delay(1000);
  }


  
  client.setServer(mqtt_server, mqtt_port);
  t = 0;
  while (!client.connected()) {
    client.connect(mqtt_username, mqtt_username, mqtt_password);
    t++;
    if (t > 10)
    {
      return;
    }
    delay(250);
  }


  client.publish(topic4, mqtt_clientid, 0);

  delay(200); 
  //Serial.println(vcc);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100); 
}


void reset_all()
{
	rtcValues.CollectPointer = 0;
	for (int i = 0; i < ByteSize; i++)
	{
		rtcValues.values[i] = 0;
	}
	for (int i = 0; i < ByteSize * 2; i++)
	{
		HEXSTR[i] = '0';
	}
	HEXSTR[ByteSize * 2] = NULL;
}
