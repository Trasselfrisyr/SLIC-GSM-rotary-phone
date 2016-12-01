# SLIC-GSM-rotary-phone
GSM conversion for rotary phone, using Arduino, a QCX601 SLIC and a SIM800 or SIM900 breakout.

Main functions:

- Control ringing using the SLIC
- Monitor the switch hook signal from SLIC to control states and count pulses for dialing
- Dial out using the GSM board
- Monitor the GSM for incoming calls
- Short number list (0 to 9) for favorite numbers (hardcoded)

Known issues:

- The SIM900/800 has got differential connections for mic and speaker making it less good for connecting directly to the SLIC board or to receivers with common ground (three wires instead of four). I will try using an A6 board later on to see if that makes things easier, as they have a secondary headset connection with common ground.

- Hard to get rid of GSM interference in audio.

Connections to rotary phone:

- If you can get a good audio connection between GSM board and SLIC, you only need to hook up the ring/tip wires to the SLIC. It provides a phone line connection.

- If you are having troubles with audio, try connecting the phone receiver wires directly to the GSM board instead.


For the SIM900 library I’ve been using the Itead one, but there are a bunch out there that will work.

https://github.com/itead/ITEADLIB_Arduino_SIMCom

Basic schematic for the connections available in the slic-gsm-rotary.png file. For recommended decoupling caps and such, see datasheets.

Make sure PIN is disabled on your SIM card, or stuff won’t work.