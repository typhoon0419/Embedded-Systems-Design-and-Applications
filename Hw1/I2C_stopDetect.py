import time
import board
import busio

# import digitalio # Used with SPI
import adafruit_lsm9ds0
from gpiozero import LED

# Setting
led = LED(18)
red = LED(15)
i2c = busio.I2C(board.SCL, board.SDA)
sensor = adafruit_lsm9ds0.LSM9DS0_I2C(i2c)

warningTimes = 0
overSpeedGap = 0

# Main loop will read the acceleration, magnetometer, gyroscope, Temperature
# values every second and print them out.
while True:
    # Read acceleration, magnetometer, gyroscope, temperature.
    accel_x, accel_y, accel_z = sensor.acceleration
    mag_x, mag_y, mag_z = sensor.magnetic
    gyro_x, gyro_y, gyro_z = sensor.gyro
    temp = sensor.temperature

    if abs(accel_z) >= 13. or abs(gyro_y) >= 50.:
        led.on()
        time.sleep(0.05)
        led.off()
        time.sleep(0.05)
        led.on()
        time.sleep(0.05)
        led.off()
        print("檢測到你的交通工具翻覆，是否需要幫助?")
    elif abs(accel_x) >= 5. or abs(accel_y) >= 5.:
        red.on()
        warningTimes += 1
        print("你的駕駛習慣不太好喔,急煞或急催油門會使乘客感到不舒適, 這是第 {0} 次, 超過 3 次會鎖定全部功能. "
              "\nAcceleration (m/s^2): ({1:0.3f},{2:0.3f},{3:0.3f})".format(
                  warningTimes, accel_x, accel_y, accel_z))
    else:
        overSpeedGap += 1
        if overSpeedGap > 5:
            warningTimes = 0
        red.off()
        if gyro_z > 100:
            print("車輛左轉彎")
        elif gyro_z < -100:
            print("車輛右轉彎")

    if warningTimes >= 3:
        print("You are out of permission to using the device.")
        break
    # Delay for a second.
    time.sleep(0.5)
