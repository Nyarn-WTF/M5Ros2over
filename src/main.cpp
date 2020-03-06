#include <M5StickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ros2wr.hpp>

const char *ssid = "M5AP";
const char *password = "77777777";
WiFiUDP udp;
const char *hostname = "";

ROS2WR<geometry_msgs::Quaternion, geometry_msgs::Twist> *node;

void MainThread(void*);

void setup(){
  M5.begin();
  M5.update();
	Wire.begin(0,26,10000);
  WiFi.begin(ssid, password);
  while(WiFi.isConnected());
  MDNS.begin("M5Ros2over");
  IPAddress IPA = MDNS.queryHost(hostname);
  String sIPA = IPA.toString();
  ros2::init(&udp, sIPA.c_str(), (uint16_t) 2018);
  node = new ROS2WR<geometry_msgs::Quaternion, geometry_msgs::Twist>(100, "m5/quaternion", "cmd_vel");
  xTaskCreatePinnedToCore(MainThread, "MainThread", 1024*4, NULL, 2, NULL, 1);
}

void loop(){
	ros2::spin(node);
}

void MainThread(void *pvParameters){
  
  while(1){

  }
}

uint8_t I2CWrite1Byte( uint8_t Addr ,  uint8_t Data )
{
	Wire.beginTransmission(0x38);
	Wire.write(Addr);
	Wire.write(Data);
	return Wire.endTransmission();
}

uint8_t I2CWritebuff( uint8_t Addr,  uint8_t* Data, uint16_t Length )
{
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

uint8_t Setspeed( int16_t Vtx, int16_t Vty, int16_t Wt)
{
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
