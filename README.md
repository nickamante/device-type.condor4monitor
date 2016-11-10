device-type.concord4monitor
==========================

SmartThings Device Type to provide interface to a Concord/Concord4 alarm system

All items are provided AS-IS

# Hardware
* GE Concord Home Security System
* GE Security/Interlogix SuperBus 2000 RS-232 Automation Module
	* Model Number: 60-783-02
	* [Installation Manual](http://static.interlogix.com/library/466-1876-C_superbus_2000_rs-232_automation_module_install_instr.pdf)
	* Automation Module Protocol
* [Arduino UNO Rev3](https://store-usa.arduino.cc/products/a000066)
* [Sparkfun RS232 Shield V2](https://www.sparkfun.com/products/13029) (requires soldering)
* [SmartThings Thingshield](http://docs.smartthings.com/en/latest/arduino/)
* RS232 Male-Male Null Modem Cable

# Setup

###Arduino
#### Hardware
* Plug the *SmartThings Thingshield* and the *RS232 Shield* into your Arduino
* If using the *Sparkfun RS232 Shield V2* with a Null Modem Cable, jumper D0 to RX (Receive) and D1 to TX (Transmit). 
	* If you are not using a null modem cable you can flip the jumpers, though you'll have to flip them back to upload sketches.


			With Null Modem		With Straight Through
			         D			           D
			  * * *  7			    * * *  7
			  * * *  7			    * * *  7
			  * * *  5			    * * *  5
			  * * *  4			    * * *  4
			  * * *  5			    * * *  3
			  * * *  2			    * * *  2
			  *[*-*] 1 (MTX)	   [*-*]*  1 (MTX)
			 [*-*]*  0 (MRX)	    *[*-*] 0 (MRX)
			J 1 2 3				  J 1 2 3

* Connect the RS232 Shield to SuperBus Automation Module using the null modem cable.

#### Software
* Download and install the [Arduino IDE](https://www.arduino.cc/en/Main/Software)
* Download and install the [SmartThings Arduino library](http://docs.smartthings.com/en/latest/arduino/)
* Connect the Arduino via USB and upload the sketch **ConcordController\ConcordController.ino**