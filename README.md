# A low battery consumption sensor to detect furnace activity and sending data through MQTT.

This library uses multiple ways to lower battery consumption. 
1. modified ESP-01 which uses ~10 microAmp during sleep, and wake up once a minute;
2. Two different wakeup modes. a)every minute, it wakes up with wifi radio off to probe, and write recordings to RTC memory which would survive the deep sleep; b) every hour, it wakes up with wifi radio on to send daa through MQTT;
3. IF wifi or mqtt is down, it would try to connect every small number times and go back to sleep without recording things.

I used the esp8266's builtin ADC to monitor the input voltage. 

The home server is implemented on a raspberry server, and with daemon written with PERL to listen and record mqtt message to a mysql database.


## Examples


## Limitations


## Compatible Hardware

ESP-01 was modified as described in https://tim.jagenberg.info/2015/01/18/low-power-esp8266/ . The sensor was buit according to https://makezine.com/projects/non-contact-voltage-detector/. I used a 18650 3.7V Li-ion Rechargeable Battery, and HT733, a low dropout voltage regulator.


## License

This code is released under the MIT License.
