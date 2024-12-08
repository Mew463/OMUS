# `sudo pkill bluetoothd` if bluetooth not working on ming's mac m3 pro
import threading
from LaptopKeyboard import *
from bluetooth import *
import asyncio
import time

startTime = time.time()

def millis():
    return round((time.time()-startTime) * 1000)

omus = BLE_UART(peripheral_name='OMUS', address = '5B5FE843-FA33-2075-4100-949B5FF1ED5F')

keyboard_thread = threading.Thread(target=lambda: Listener(on_press=on_press, on_release=on_release).start())
keyboard_thread.daemon = True
keyboard_thread.start()  

robotcmd = ""
enabled = 0
drivestate = 1

async def bluetooth_receive_handler(BLE_DEVICE):
    global lastBeaconRead
    while True:
        await asyncio.sleep(0.1)
        if (BLE_DEVICE.isConnected):
            msg = await BLE_DEVICE.read()
            print(f"[{BLE_DEVICE._peripheral_name}] {msg}")

async def bluetooth_comm_handler(BLE_DEVICE):
    await BLE_DEVICE.connect()
    while True:
        await asyncio.sleep(0.05)
        if (BLE_DEVICE.isConnected):
            await BLE_DEVICE.write(robotcmd)
        else:  
            await BLE_DEVICE.connect()


async def cmd_handler():
    global robotcmd
    global irbeaconcmd
    global enabled
    global activeBeacon
    global drivestate
    waitForEnableReleased = 0
    while True:
        x,y,drivecmd,robottuning,flip,boost = (0,)*6  
        
        if get_key_state("Key.up"): 
            y = y + 1
        if get_key_state("Key.down"):
            y = y - 1
        if get_key_state("Key.left"):
            x = x - 1
        if get_key_state("Key.right"): 
            x = x + 1 
            
        if x == 0 and y == 0:
            drivecmd = 0
        elif x == 0 and y == 1:
            drivecmd = 1
        elif x == 1 and y == 1:
            drivecmd = 2
        elif x == 1 and y == 0:
            drivecmd = 3
        elif x == 1 and y == -1:
            drivecmd = 4
        elif x == 0 and y == -1:
            drivecmd = 5
        elif x == -1 and y == -1:
            drivecmd = 6
        elif x == -1 and y == 0:
            drivecmd = 7
        elif x == -1 and y == 1:
            drivecmd = 8
            
        if get_key_state('q'):
            robottuning = 1
        elif get_key_state('a'):
            robottuning = 2
        elif get_key_state('w'):
            robottuning = 3
        elif get_key_state('s'):
            robottuning = 4
        elif get_key_state('e'):
            robottuning = 5
        elif get_key_state('d'):
            robottuning = 6
            
        if get_key_state("Key.space"):     
            if get_key_state("Key.ctrl"):
                enabled = 1
                drivestate = 1 # Set default drive state
                waitForEnableReleased = 1
            else:
                if not waitForEnableReleased:
                    enabled = 0
        if not (get_key_state("Key.space")) and not (get_key_state("Key.ctrl")):
            waitForEnableReleased = 0
        if (enabled == 1):
            if (get_key_state('z')):
                drivestate = 1
            if (get_key_state('c')):
                drivestate = 2
        else:
            drivestate = 0
        if (get_key_state('x') or get_key_state('X')):
            flip = 1
        
        if (get_key_state("Key.shift")):
            boost = 1 

        robotcmd = f"{drivestate}{drivecmd}{robottuning}{boost}{flip}"
        # print(robotcmd)
        await asyncio.sleep(0.01)
        
async def main():
    await asyncio.gather(cmd_handler(), bluetooth_comm_handler(omus), bluetooth_receive_handler(omus))

asyncio.run(main())

