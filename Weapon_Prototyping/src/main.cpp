#include <Arduino.h>
#include "DShotESC.h"
#include "MyButton.h"
#include <BLE_Uart.h>
#include <pindefinitions.h>
#include <FastLED.h>

MyButton but = MyButton(1);

const int packSize = 5;
char laptop_packetBuffer[packSize] = {};
BLE_Uart computer = BLE_Uart(laptop_packetBuffer, packSize);


DShotESC l_mot;
DShotESC r_mot;
// DShotESC weap_mot;

struct tank_drive_parameters {
  int drive = 500;
  int turn = 300;
  int boost = 200;
} tank_drive_parameters;

bool toggle = false;
int direction = 1;

int power = 300;
int despoolpower = 30;

void setup_motors() {
  l_mot.install(LEFT_MOTOR, RMT_CHANNEL_0);
  l_mot.init();
  l_mot.setReversed(false);
  l_mot.set3DMode(true);
  l_mot.throttleArm();

  r_mot.install(RIGHT_MOTOR, RMT_CHANNEL_1);
  r_mot.init();
  r_mot.setReversed(false);
  r_mot.set3DMode(true);
  r_mot.throttleArm();

  // weap_mot.install(WEAPON_MOTOR, RMT_CHANNEL_0);
  // weap_mot.init();
  // weap_mot.setReversed(false);
  // weap_mot.set3DMode(true);
  // weap_mot.throttleArm();
  
}

void setup() {
  Serial.begin(115200);
  
  setup_motors();
  
  computer.init_ble("OMUS");
	
  pinMode(MAG_SENSE, INPUT_PULLUP);
}

void loop() {
  if (computer.isConnected()) {
    // weap_mot.sendThrottle3D(0);
    switch(laptop_packetBuffer[0]) {
      case '0': // Disabled case
        l_mot.sendThrottle3D(0);
        r_mot.sendThrottle3D(0);
      break;
      case '1': // Enabled case
        int lmotorpwr = 0;
        int rmotorpwr = 0;
        int boostVal = 0;
        if (laptop_packetBuffer[3] == '1')
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

      l_mot.sendThrottle3D(-lmotorpwr);
      r_mot.sendThrottle3D(rmotorpwr);

      EVERY_N_MILLIS(100) {
        if (laptop_packetBuffer[2] != '0') { // Increasing power
          if (laptop_packetBuffer[2] == '1')
            tank_drive_parameters.drive+=50;
          if (laptop_packetBuffer[2] == '2')
            tank_drive_parameters.drive-=50;
          if (laptop_packetBuffer[2] == '3')
            tank_drive_parameters.turn+=50;
          if (laptop_packetBuffer[2] == '4')
            tank_drive_parameters.turn-=50;
            
          computer.send(String(tank_drive_parameters.drive) + " " +  String(tank_drive_parameters.turn));
        }
      }
      


      // if (laptop_packetBuffer[4] == '1') {

      //   for (int i = 0; i < 50; i++) {
      //     delay(10);
      //     weap_mot.sendThrottle3D(power * direction);
      //   }

      //   for (int i = 0; i < 100; i++) {
      //     delay(10);
      //     weap_mot.sendThrottle3D(0);
      //   }

      //   for (int i = 0; i < 70; i++) {
      //     delay(10);
      //     weap_mot.sendThrottle3D(despoolpower * direction * -1);
      //   }
        
      
      // }

      break;
    }

  }
  delay(5);
}
