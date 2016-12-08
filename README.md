# SLIC-GSM-rotary-phone
GSM conversion for rotary phone, using Arduino Pro Mini with Atmega328 (both 8Mhz and 16Mhz versions should work fine), a QCX601 SLIC and a SIM800 or SIM900 series breakout board with pins or jacks for mic and speaker.

Main functions:

- Control ringing using the SLIC
- Monitor the switch hook signal from SLIC to control states and count pulses for dialing
- Dial out using the GSM board
- Monitor the GSM for incoming calls
- Answer calls when receiver is picked up
- Short number list (0 to 9) for favorite numbers (hardcoded)

Known issues:

- Hard to get rid of GSM interference in audio if receiver audio is routed via the SLIC module. For clean audio, connect the receiver directly to MIC+ (tip of MIC jack connector), SPK+ (tip of SPK jack connector) and GND (pin connector, not from negative pins of jack connectors). If the handset has got separate wires for negative on mic and speaker, you can use the differential negative connections from the jacks. 

Connections to rotary phone:

- Hook up the ring/tip wires to the ring/tip pins of the SLIC module. It provides a local phone line connection.

- For best audio, connect the phone receiver wires directly to the GSM board.


For the SIM900 library I’ve been using the Itead one, but there are a bunch out there that will work.

https://github.com/itead/ITEADLIB_Arduino_SIMCom

Schematic for the connections available in the slic-gsm-rotary-schematic.png file. 

Make sure PIN is disabled on your SIM card, or stuff won’t work.