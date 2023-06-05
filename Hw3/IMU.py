import time, os
import adafruit_lsm9ds0
import board
import busio
from gpiozero import LED

i2c = busio.I2C(board.SCL, board.SDA)
sensor = adafruit_lsm9ds0.LSM9DS0_I2C(i2c)

warningTimes = 0
overSpeedGap = 0

mode = ["off", "Red on", "Yellow on", "flash"]

currPath = os.getcwd()
print(currPath)
path = "/sys/ebb/ledGOGO/mode"
ctrl = open(path, "w")
ctrl.write(mode[0])

# Main loop will read the acceleration, magnetometer, gyroscope, Temperature
# values every second and print them out.
while True:
    # Read acceleration, magnetometer, gyroscope, temperature.
    accel_x, accel_y, accel_z = sensor.acceleration
    mag_x, mag_y, mag_z = sensor.magnetic
    gyro_x, gyro_y, gyro_z = sensor.gyro
    temp = sensor.temperature

    if abs(accel_z) >= 13. or abs(gyro_y) >= 50.:
        ctrl.write(mode[2])
        time.sleep(0.05)
        ctrl.write(mode[0])
        time.sleep(0.05)
        ctrl.write(mode[2])
        time.sleep(0.05)
        ctrl.write(mode[0])
        print("Are you turned over? Did you need help?")
    elif abs(accel_x) >= 5. or abs(accel_y) >= 5.:
        ctrl.write(mode[1])
        warningTimes += 1
        print("You have bad habit when driving, It's {0} times, System will stop when 3 times is detected. "
              "\nAcceleration (m/s^2): ({1:0.3f},{2:0.3f},{3:0.3f})".format(
                  warningTimes, accel_x, accel_y, accel_z))
    else:
        overSpeedGap += 1
        if overSpeedGap > 5:
            warningTimes = 0
        ctrl.write(mode[0])
        if gyro_z > 100:
            print("get left")
        elif gyro_z < -100:
            print("get right")

    if warningTimes >= 3:
        print("You are out of permission to using the device.")
        break
    # Delay for a second.
    time.sleep(0.5)
