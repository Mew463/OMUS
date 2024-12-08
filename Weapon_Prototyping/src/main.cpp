#include <Arduino.h>
#include "DShotESC.h"
#include "MyButton.h"
#include <BLE_Uart.h>
#include <pindefinitions.h>
#include <FastLED.h>
#include "Adafruit_VL53L0X.h"
#include <Wire.h>
#include <LEDHandler.h>
#include </Users/mingweiyeoh/Documents/GitHub/Oreo/Robot Code/lib/Battery_Monitor/Battery_Monitor.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int tofSensorValue = 0;
int rangeThreshold = 80;
bool tofSensorClear = false;

const int packSize = 5;
char laptop_packetBuffer[packSize] = {};
BLE_Uart computer = BLE_Uart(laptop_packetBuffer, packSize);

DShotESC l_mot;
DShotESC r_mot;
DShotESC weap_mot;

struct tank_drive_parameters {
  int drive = 250;
  int turn = 250;
  int boost = 200;
} tank_drive_parameters;

bool toggle = false;
int direction = 1;

int power = 300;
int despoolpower = 20;

enum WEAPON_STATES {
  IDLE,
  ACTIVATED,
  STOP,
  DESPOOL
};

enum ROBOT_STATES {
  NOT_ENABLED,
  TELEOP,
  TELEOP_AUTO
};

int WEAPON_STATE = 0;
unsigned long lastStateChange = millis();

int weapon_delays[] ={0, 500, 350, 1500};

void setup_motors() {
  l_mot.install(LEFT_MOTOR, RMT_CHANNEL_1);
  l_mot.init();
  l_mot.setReversed(false);
  l_mot.set3DMode(true);
  l_mot.throttleArm();

  r_mot.install(RIGHT_MOTOR, RMT_CHANNEL_2);
  r_mot.init();
  r_mot.setReversed(false);
  r_mot.set3DMode(true);
  r_mot.throttleArm();

  weap_mot.install(WEAPON_MOTOR, RMT_CHANNEL_3);
  weap_mot.init();
  weap_mot.setReversed(false);
  weap_mot.set3DMode(true);
  weap_mot.throttleArm();
}

void setup() {
  USBSerial.begin(115200);
  
  setup_motors();
  pinMode(MAG_SENSE, INPUT);

  computer.init_ble("OMUS");
  
	init_led();
  setLeds(ORANGE);

  Wire.begin(SDA_TOF, SCL_TOF);
  while (!lox.begin()) {
    USBSerial.println("Failed to boot VL53L0X");
    delay(100);
  }

  lox.startRangeContinuous();

}

void loop() {

  EVERY_N_MILLIS(1000) {
    USBSerial.print("."); // This keeps the serial alive??
  }

  if (computer.isConnected()) {
    int robotmode = int(laptop_packetBuffer[0] - '0');

    if (robotmode == NOT_ENABLED) { // Disabled case
      setLeds(GREEN);
      l_mot.sendThrottle3D(0);
      r_mot.sendThrottle3D(0);
      weap_mot.sendThrottle3D(0);
      
      EVERY_N_MILLIS(1000) {
        computer.send("SOC: " + String(get3sSOC(BAT_PIN)) + " %");
      }

    } else { // Teleop mode
      int lmotorpwr = 0;
      int rmotorpwr = 0;
      int boostVal = 0;
      if (laptop_packetBuffer[3] == '1') // Boost button activates this
        boostVal = tank_drive_parameters.boost;

      switch (laptop_packetBuffer[1]) { // Check the drive cmd
      case '0':
        lmotorpwr = 0;
        rmotorpwr = 0;
        break;
      case '1':
        lmotorpwr = tank_drive_parameters.drive + boostVal;
        rmotorpwr = tank_drive_parameters.drive + boostVal;
        break;
      case '2':
        lmotorpwr = tank_drive_parameters.drive + tank_drive_parameters.turn + boostVal;
        rmotorpwr = tank_drive_parameters.drive - tank_drive_parameters.turn + boostVal; 
        break;
      case '3':
        lmotorpwr = tank_drive_parameters.turn + boostVal;
        rmotorpwr = -tank_drive_parameters.turn - boostVal; 
        break;
      case '4':
        lmotorpwr = -tank_drive_parameters.drive - tank_drive_parameters.turn - boostVal;
        rmotorpwr = -tank_drive_parameters.drive + tank_drive_parameters.turn - boostVal;
        break;
      case '5':
        lmotorpwr = -tank_drive_parameters.drive - boostVal;
        rmotorpwr = -tank_drive_parameters.drive - boostVal;
        break;
      case '6':
        lmotorpwr = -tank_drive_parameters.drive + tank_drive_parameters.turn - boostVal;
        rmotorpwr = -tank_drive_parameters.drive - tank_drive_parameters.turn - boostVal; 
        break;
      case '7':
        lmotorpwr = -tank_drive_parameters.turn - boostVal;
        rmotorpwr = tank_drive_parameters.turn + boostVal;  
        break;
      case '8':
        lmotorpwr = tank_drive_parameters.drive - tank_drive_parameters.turn + boostVal;
        rmotorpwr = tank_drive_parameters.drive + tank_drive_parameters.turn + boostVal; 
      break;
      }

      EVERY_N_MILLIS(10) {
        l_mot.sendThrottle3D(-lmotorpwr);
        r_mot.sendThrottle3D(rmotorpwr);
      }

      EVERY_N_MILLIS(100) { // CONFIGURING MOTOR POWERS
        if (laptop_packetBuffer[2] != '0') { // Check for if we want to change a setting
          if (laptop_packetBuffer[2] == '1')
            tank_drive_parameters.drive+=50;
          if (laptop_packetBuffer[2] == '2')
            tank_drive_parameters.drive-=50;
          if (laptop_packetBuffer[2] == '3')
            tank_drive_parameters.turn+=50;
          if (laptop_packetBuffer[2] == '4')
            tank_drive_parameters.turn-=50;
          if (laptop_packetBuffer[2] == '5')
            power+=50;
          if (laptop_packetBuffer[2] == '6')
            power-=50;
            
          computer.send(String(tank_drive_parameters.drive) + " " +  String(tank_drive_parameters.turn) + " " +  String(power));
        }
      }
      
      if (robotmode == TELEOP) {
        setLeds(BLUE);
      } else if (robotmode == TELEOP_AUTO) {
        VL53L0X_RangingMeasurementData_t measure;
        tofSensorValue = lox.rangingTest(&measure, false);
        if (measure.RangeStatus != 2) { // 2 Means a ranging error
          tofSensorValue = measure.RangeMilliMeter;
          if (WEAPON_STATE == IDLE && tofSensorValue < rangeThreshold && tofSensorClear == true) {
            WEAPON_STATE = ACTIVATED;
            tofSensorClear = false;
            lastStateChange = millis();
          }
          toggleLeds(BLACK, BLUE, constrain(tofSensorValue * 2, 50, 500));
        } else {
          setLeds(PURPLE);
          tofSensorValue = 999;
        }    
        if (tofSensorValue > rangeThreshold + 30 && digitalRead(MAG_SENSE) == 0) // Double check arm has returned to normal position
          tofSensorClear = true;
      }

      if (laptop_packetBuffer[4] == '1') { // Manual pressing of the fire button!
        WEAPON_STATE = ACTIVATED;
        lastStateChange = millis();
      }

      if (millis() - lastStateChange > weapon_delays[WEAPON_STATE] && WEAPON_STATE != IDLE) {
        WEAPON_STATE = WEAPON_STATE + 1;
        lastStateChange = millis();
        if (WEAPON_STATE > 3) // To not access outside of the ENUMs 
          WEAPON_STATE = IDLE;
      } 

      EVERY_N_MILLIS(10) {
        switch(WEAPON_STATE) {
          case IDLE:
            weap_mot.sendThrottle3D(0);
          break; 
          case ACTIVATED:
            weap_mot.sendThrottle3D(power);
          break;
          case STOP:
            weap_mot.sendThrottle3D(0);
          break;
          case DESPOOL:
            weap_mot.sendThrottle3D(-despoolpower);
            if (digitalRead(MAG_SENSE) == 0) {// Mag encoder gets activated
              delay(100);
              WEAPON_STATE = IDLE;
            }
          break;
        }

      }
    }
  } else { // Computer Disconnected Case
    setLeds(RED);
  }
}
