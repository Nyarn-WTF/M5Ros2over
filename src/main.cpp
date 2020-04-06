#include <M5StickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ros2arduino.h>
#include <ros2wr.hpp>
#include <ESPmDNS.h>

const char *ssid = "hoge";
const char *password = "hoge";

WiFiUDP udp;
const char *host = "hoge";

ROS2WR<std_msgs::String, geometry_msgs::Twist> *node;

void MainThread(void*);

void setup(){
	M5.begin();
	M5.Lcd.setTextSize(3);
	M5.Lcd.setRotation(3);
	Wire.begin(0,26,10000);
	pinMode(10, OUTPUT);
	
	WiFi.begin(ssid, password);
	while(WiFi.status() != WL_CONNECTED);
	while(!ros2::init(&udp, "192.168.0.200", (uint16_t) 50000)){
		delay(1);
	}
	Serial.println("ros start");

	node = new ROS2WR<std_msgs::String, geometry_msgs::Twist>(100, "m5/pub", "cmd_vel");
	
	xTaskCreatePinnedToCore(MainThread, "MainThread", 1024*16, NULL, 1, NULL, 1);
}

void loop(){
	Serial.println("Spin");
	ros2::spin(node);
}

void BrinkLED();
void Drive(geometry_msgs::Twist*);

void MainThread(void *pvParameters){
	geometry_msgs::Twist smsg;
	std_msgs::String pmsg;
	uint64_t n = 0;
	while(1){
		do{
			BrinkLED();
			Serial.printf("%lld\n", n);
			n++;
			delay(1);
		}while(!node->getSubscribeMsg(&smsg));
		Serial.println("ok");
		Drive(&smsg);
		sprintf(pmsg.data, "%lld\n", n);
		node->setPublishMsg(&pmsg);
	}
}

void BrinkLED(){
	static bool stat = true;
	digitalWrite(10, (int)stat);
	stat = stat ? false : true;
}

uint8_t I2CWrite1Byte( uint8_t Addr ,  uint8_t Data ){
	Wire.beginTransmission(0x38);
	Wire.write(Addr);
	Wire.write(Data);
	return Wire.endTransmission();
}

uint8_t I2CWritebuff( uint8_t Addr,  uint8_t* Data, uint16_t Length ){
	Wire.beginTransmission(0x38);
	Wire.write(Addr);
	for (int i = 0; i < Length; i++)
	{
		Wire.write(Data[i]);
	}
	return Wire.endTransmission();
}


int16_t speed_buff[4] = {0};
int8_t  speed_sendbuff[4] = {0};
uint32_t count = 0;
uint8_t IIC_ReState = I2C_ERROR_NO_BEGIN;

uint8_t Setspeed( int16_t Vtx, int16_t Vty, int16_t Wt){
	Wt = ( Wt > 100 )  ? 100 :  Wt;
	Wt = ( Wt < -100 ) ? -100 : Wt;

	Vtx = ( Vtx > 100 )  ? 100 :  Vtx;
	Vtx = ( Vtx < -100 ) ? -100 : Vtx;
	Vty = ( Vty > 100 )  ? 100 :  Vty;
	Vty = ( Vty < -100 ) ? -100 : Vty;

	Vtx = ( Wt != 0 ) ? Vtx *  (100- abs(Wt)) / 100 : Vtx;
	Vty = ( Wt != 0 ) ? Vty *  (100- abs(Wt)) / 100 : Vty;

	speed_buff[0] = Vty - Vtx - Wt ;
	speed_buff[1] = Vty + Vtx + Wt ;
	speed_buff[3] = Vty - Vtx + Wt ;
	speed_buff[2] = Vty + Vtx - Wt ;

	for (int i = 0; i < 4; i++)
	{
		speed_buff[i] = ( speed_buff[i] > 100 )  ? 100 :  speed_buff[i];
		speed_buff[i] = ( speed_buff[i] < -100 ) ? -100 : speed_buff[i];
		speed_sendbuff[i] = speed_buff[i];
	}
	return I2CWritebuff(0x00,(uint8_t*)speed_sendbuff,4);
}

void Drive(geometry_msgs::Twist *msg){
	Setspeed(msg->linear.x*100.0, msg->linear.y*100.0, msg->angular.z*100.0);
}