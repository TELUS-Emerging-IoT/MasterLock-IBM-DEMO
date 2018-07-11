/* BG96 NetworkSocketAPI Example Program
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "BG96Interface.h"
#include "TCPSocket.h"
#include "MQTTClient.h"
#include "MQTT_GSM.h"
#include <ctype.h>

#include "BG96.h"

//------------------------------------
// Hyperterminal default configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(SERIAL_TX, SERIAL_RX);
DigitalOut myled(LED1);
DigitalOut redLED(A3);
DigitalOut yellowLED(A4);
DigitalOut greenLED(A5);
DigitalOut lockPin1_1(A1);
DigitalOut lockPin1_2(A2);
DigitalIn motionSensor(A0);

#define MQTT_MAX_PACKET_SIZE 300
#define MQTT_MAX_PAYLOAD_SIZE 500

#define SUBSCRIBE

// Configuration values needed to connect to IBM IoT Cloud
#define BROKER_URL ".messaging.internetofthings.ibmcloud.com";
#define ORG "3ig3na"
#define ID "ConsiliumDemoUnit"
#define AUTH_TOKEN "koUdVL)xNMK0EQh_V5"
#define DEFAULT_TYPE_NAME "TELUS-IOT-Starter-Kit"
#define TOPIC  "iot-2/cmd/+/fmt/String"


// network credential
#define APN    "pp.telus.com" //"m2m-east.telus.iot"
#define PASSW  ""
#define USNAME ""

#define TYPE DEFAULT_TYPE_NAME       // Device Type defined above
#define MQTT_PORT 1883
#define MQTT_TLS_PORT 8883
#define IBM_IOT_PORT MQTT_PORT

char id[30] = ID;                 // mac without colons
char org[12] = ORG;
int connack_rc = 0; // MQTT connack return code
//const char* ip_addr = "11.12.13.14";
//char* host_addr = "11.12.13.14";
char sensor_id[50];
char type[36] = TYPE;
char auth_token[30] = AUTH_TOKEN; // Auth_token is only used in non-quickstart mode
bool netConnecting = false;
int connectTimeout = 1000;
bool mqttConnecting = false;
bool netConnected = false;
bool connected = false;
int retryAttempt = 0;
char subscription_url[MQTT_MAX_PAYLOAD_SIZE];
char subbed_msg[MQTT_MAX_PAYLOAD_SIZE];
#define FW_REV				"1.0a"

MQTT::Message message;
MQTTString TopicName={TOPIC};
MQTT::MessageData MsgData(TopicName, message);

void subscribe_cb(MQTT::MessageData & msgMQTT) {
    char msg[MQTT_MAX_PAYLOAD_SIZE];
    msg[0]='\0';
    strncat (msg, (char*)msgMQTT.message.payload, msgMQTT.message.payloadlen);
    sprintf (subbed_msg, "%s", msg);
}

int subscribe(MQTT::Client<MQTT_GSM, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTT_GSM* ipstack)
{
    char* subTopic = TOPIC;
    return client->subscribe(subTopic, MQTT::QOS1, subscribe_cb);
}

int connect(MQTT::Client<MQTT_GSM, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTT_GSM* ipstack)
{
    const char* iot_ibm = BROKER_URL;
    char hostname[strlen(org) + strlen(iot_ibm) + 1];
    sprintf(hostname, "%s%s", org, iot_ibm);

    // Construct clientId - d:org:type:id
    char clientId[strlen(org) + strlen(type) + strlen(id) + 5];
    sprintf(clientId, "d:%s:%s:%s", org, type, id);  //@@
    sprintf(subscription_url, "%s.%s/#/device/%s/sensor/", org, "internetofthings.ibmcloud.com",id);

    netConnecting = true;
    ipstack->open(&ipstack->getGSM());
    int rc = ipstack->connect(hostname, IBM_IOT_PORT, connectTimeout);
    if (rc != 0)
    {
        //WARN("IP Stack connect returned: %d\n", rc);
        return rc;
    }
    pc.printf ("--->TCP Connected\n\r");
    netConnected = true;
    netConnecting = false;

    // MQTT Connect
    mqttConnecting = true;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.struct_version=0;
    data.clientID.cstring = clientId;

 	data.username.cstring = "use-token-auth";
    data.password.cstring = auth_token;

    if ((rc = client->connect(data)) == 0)
    {
        connected = true;
        pc.printf ("--->MQTT Connected\n\r");
	#ifdef SUBSCRIBE
        if (!subscribe(client, ipstack)) printf ("--->>>MQTT subscribed to: %s\n\r",TOPIC);
	#endif
    }
    else {
        //WARN("MQTT connect returned %d\n", rc);
    }
    if (rc >= 0)
        connack_rc = rc;
    mqttConnecting = false;
    return rc;
}

int getConnTimeout(int attemptNumber)
{  // First 10 attempts try within 3 seconds, next 10 attempts retry after every 1 minute
   // after 20 attempts, retry every 10 minutes
    return (attemptNumber < 10) ? 3 : (attemptNumber < 20) ? 60 : 600;
}

void attemptConnect(MQTT::Client<MQTT_GSM, Countdown, MQTT_MAX_PACKET_SIZE>* client, MQTT_GSM* ipstack)
{
    connected = false;

    while (connect(client, ipstack) != MQTT_CONNECTION_ACCEPTED)
    {
        if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD) {
            printf ("File: %s, Line: %d Error: %d\n\r",__FILE__,__LINE__, connack_rc);
            return; // don't reattempt to connect if credentials are wrong
        }
        int timeout = getConnTimeout(++retryAttempt);
        //WARN("Retry attempt number %d waiting %d\n", retryAttempt, timeout);

        // if ipstack and client were on the heap we could deconstruct and goto a label where they are constructed
        //  or maybe just add the proper members to do this disconnect and call attemptConnect(...)
        // this works - reset the system when the retry count gets to a threshold
        if (retryAttempt == 5){
						pc.printf ("\n\n\rFAIL!! system reset!!\n\n\r");
            NVIC_SystemReset();
				}
        else
            wait(timeout);
    }
}

//HELPER FUNCTION OF LED SWITCHING
void writeLED(int number)
{
	if (number == 0)
	{
		redLED = 1;
		yellowLED = 0;
		greenLED = 0;
	}
	else if (number == 1)
	{
		redLED = 0;
		yellowLED = 1;
		greenLED = 0;
	}
	else if (number == 2)
	{
		redLED = 0;
		yellowLED = 0;
		greenLED = 1;
	}
}

//MAIN function
int loop_count = 0;
bool isAvailable = false;
bool isLocked = true;

int main()
{
    const char * apn = APN; // Network must be visible otherwise it can't connect
    const char * username = USNAME;
	const char * password = PASSW;
    BG96Interface bg96_if(D8, D2, false);

	//change serial baud to 9600
	pc.baud(9600);
	wait(0.1);

	pc.printf("\r\n*************************************************\r\n");
	wait( 0.1 );
	pc.printf("TELUS LTE-M IoT Starter Kit: Lock Demo");
    wait( 0.1 );
    pc.printf("MBED online version %s\r\n", FW_REV);
    wait( 0.1 );
	pc.printf("APN  = %s\r\n", APN);

   MQTT_GSM ipstack(bg96_if, apn, username, password);
   MQTT::Client<MQTT_GSM, Countdown, MQTT_MAX_PACKET_SIZE> client(ipstack);

   attemptConnect(&client, &ipstack);

   if (connack_rc == MQTT_NOT_AUTHORIZED || connack_rc == MQTT_BAD_USERNAME_OR_PASSWORD)
   {
      while (true) wait(1.0); // Permanent failures - don't retry
   }

	sprintf(sensor_id,"%s",bg96_if.get_mac_address());

	writeLED(0);

    while (true)
    {
		if (strcmp(subbed_msg, "unlock") == 0)
		{
			subbed_msg[0] = 0;
			isAvailable = true;
			pc.printf("available for unlocking \r\n");
			writeLED(1);
            wait(1);
		}
		else if (strcmp(subbed_msg, "lock") == 0)
		{
			subbed_msg[0] = 0;
			isAvailable = false;
			lockPin1_1 = 1;
            lockPin1_2 = 0;
            writeLED(0);
            wait(1.5);
		}
		else if (strcmp(subbed_msg, "unlocknow") == 0)
		{
			subbed_msg[0] = 0;
			lockPin1_1 = 0;
            lockPin1_2 = 1;
            writeLED(2);
            wait(1.5);
		}
		else if (strcmp(subbed_msg, "locknow") == 0)
		{
			subbed_msg[0] = 0;
			lockPin1_1 = 1;
            lockPin1_2 = 0;
            writeLED(0);
            pc.printf("locked \r\n");
            isLocked = 1;
            wait(1);
		}

		if (motionSensor.read() == 1 && isAvailable)
		{
			pc.printf("unlocking \r\n");
			pc.printf("c");
			wait(1);
			char c = pc.getc();
			if (c == 'o')
			{
				lockPin1_1 = 0;
            	lockPin1_2 = 1;
            	writeLED(2);
            	pc.printf("unlocked \r\n");
            	isLocked = 0;
            	wait(7.5);
            	lockPin1_1 = 1;
            	lockPin1_2 = 0;
            	wait(1.5);
            	writeLED(1);
            	pc.printf("locked \r\n");
            	isLocked = 1;
            }
		}

		lockPin1_1 = 0;
		lockPin1_2 = 0;
		client.yield(500);
    }
}
