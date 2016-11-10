//*****************************************************************************
/// @file
/// @brief
///   Condor Monitor
///   Filters condor bus messages from the Condor 4 and sends a string representation
///   to ST.  Will also consume an ASCII Hex string to send on the condor bus.
///   This puts all the parsing and message format responsibilty on the ST Device Type.
//*****************************************************************************
//#include <SmartThings.h>

#define DEBUG // Remove this line to disable debugging

#ifdef DEBUG
  #include <SoftwareSerial.h>
  #define PIN_SOFTSERIAL_RX    6
  #define PIN_SOFTSERIAL_TX    7
  SoftwareSerial softSerial(PIN_SOFTSERIAL_TX, PIN_SOFTSERIAL_RX);

  #define panelSerial softSerial
  #define debugSerial Serial
#else
  #define panelSerial Serial
#endif

#define PIN_THING_RX    3
#define PIN_THING_TX    2 
#define DISPLAY_PARTITIION 1

// Define Global Variables
//SmartThingsCallout_t messageCallout;    // call out function forward decalaration
//SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout);  // constructor
 
unsigned short CondorData[64]; // Buffer Used to Store RX data from the Panel
char RawInputBuffer[128];
char CurrentSendCommand[128];

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  int fr = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  debugSerial.print("Free ram: ");
  debugSerial.println(fr);
  return fr;
}
 
void setup() {  
  // setup hardware pins 
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  #ifdef DEBUG
    debugSerial.begin(9600);
  #endif
  
  panelSerial.begin(9600);
  
  //Send Startup Message
  //smartthing.send( "Condor SmartThings Interface v1.0" );
    
  digitalWrite(LED_BUILTIN, HIGH);  // turn LED on  
  //smartthing.shieldSetLED(0, 0, 0);
}

//---------------------------------------------------------------------//

unsigned short RawIndex = 0;
unsigned short IncomingMessageSize = 0;
boolean isMessageIncoming = false;
int bSendDisplayData = 0;

void loop() {
  //millis();
  
  // run smartthing logic
  //smartthing.run();
 
  processIncomingMessages();
}

void processIncomingMessages( void ) {
  if ( panelSerial.available() ) {
    char incomingByte = panelSerial.read();

    if ( incomingByte == 0xA ) {
      // A single Line Feed (0Ah) is used to signal the start of a message. The line feed character should
      // reset the Host Processor's message parsing pointer as it always indicates the start of a new message.

      #ifdef DEBUG
        debugSerial.println("Rx: 0Ah");
        debugSerial.println("---------- Start of Message ----------");
      #endif

      // Reset our buffer, expected size, and status LEDs
      isMessageIncoming = true;
      RawIndex = 0;
      IncomingMessageSize = 0;      
      //smartthing.shieldSetLED(0, 0, 1); // Blue to indicate an an incoming message
    } else if ( isMessageIncoming == true ) {
      // Data
      // An 8-bit binary value is sent as two upper-case ASCII digits (‘0’...’9’, ‘A’...’F’).
      // Example: To expand the 8 bit binary value 3Ch into its ASCII representation:
      //  Send ASCII '3' (33h)
      //  Send ASCII 'C' (43h)
      // NOTE: Control characters (ACK, NAK, and LF) are sent as a single byte, and not converted into ASCII pairs.
      //  The use of control characters and ASCII data permits software flow control. Only sixteen ASCII 
      //  characters '0'...'9', 'A'...'F' are used to transfer "data." If binary data were transmitted, the use of
      //  hardware flow control, timing specifications, or a byte-stuffing scheme would be required. The control
      //  characters and ASCII data protocol was chosen because its advantage in simplicity outweighs the
      //  slight loss of efficiency.

      #ifdef DEBUG
        debugSerial.print(incomingByte, HEX);
        debugSerial.print(" ");
      #endif
     
      // Store the current data byte
      RawInputBuffer[RawIndex] = incomingByte;
      RawIndex++;

      if (RawIndex % 2 == 0) {
        // upon receiving the second nybble in the byte
        
        char dataByte[3];
        dataByte[0] = RawInputBuffer[RawIndex-2];        
        dataByte[1] = RawInputBuffer[RawIndex-1];
        dataByte[2] = '\0'; // null terminator
                
        CondorData[RawIndex/2-1] = strtol( dataByte, NULL, 16 );

        #ifdef DEBUG        
//          debugSerial.print("High nybble: ");
//          debugSerial.print(dataByte[0], HEX);  
//          debugSerial.println('h');      
//          debugSerial.print("Low nybble: ");
//          debugSerial.print(dataByte[1], HEX);
//          debugSerial.println('h');
            debugSerial.print("--- Data byte: ");
            debugSerial.print(dataByte[0]);
            debugSerial.print(dataByte[1]);
            debugSerial.println('h');
        #endif
      }

                 
      if ( RawIndex == 2 ) {
        // The first two bytes of data are the Last Index. The Last Index indicates the remaining number of bytes (ASCII pairs) to follow in the message.
        // For example, in a three byte message, the last index should be 02h, indicating two bytes (a total of four) ASCII characters will follow.
        
        long lastIndex = CondorData[0];              
        IncomingMessageSize = (lastIndex * 2) + 2; // the total number of ASCII characters to expect in the message (two per byte), including the two for the last index byte

        #ifdef DEBUG  
          debugSerial.print("          --- Last Index (remaining ASCII Pairs): ");
          debugSerial.println(lastIndex);
  
          debugSerial.print("          --- Total Message Size: ");
          debugSerial.println(IncomingMessageSize);
        #endif
      } else if ( RawIndex >= IncomingMessageSize && IncomingMessageSize > 1 ) {
        #ifdef DEBUG
          debugSerial.println("---------- Message Complete ----------"); 
        #endif
        
        // We've received the expected number of bytes, process the message
               
        //smartthing.shieldSetLED(1, 0, 1);  // Purple to indicate processing a received message
        
        RawInputBuffer[RawIndex] = '\0'; // Null terminate our string
        
        if( isValidChecksum() ) {
          // ACK (06h) (Acknowledge)
          // Sent autonomously by the Automation Device to indicate that the message was well formed (enough
          // bytes were received, and the checksum was correct).
          // Sent autonomously by the Automation Module to indicate that the message was well formed (enough
          // bytes were received, and the checksum was correct) and that the message has been successfully
          // transmitted to the panel. An ACK sent by the Automation Module does not mean the Panel
          // understood/acted on the message.

          panelSerial.write( 0x6 );

          #ifdef DEBUG
            debugSerial.println("Tx: 06h (ACK)\n\n");
            //PrintCondorPacket();
          #endif
          
          ProcessCondorPacket();
          
          //smartthing.shieldSetLED(0, 1, 0); // Green to indicate a valid message 
        } else { 
          // NAK (15h) (Negative Acknowledge)
          // Sent autonomously by the Automation Module or Automation Device whenever the message was not
          // well formed (i.e., too many bytes, or invalid checksum). The message originator should re-send its message.
                    
          panelSerial.write( 0x15 );

          #ifdef DEBUG
            debugSerial.println("Tx: 15h (NAK)\n\n");
            //PrintCondorPacket();
          #endif
                    
          //smartthing.shieldSetLED(1, 0, 0); // Red to indicate an invalid message      
        }
        
        // Reset in preperation for the next message       
        isMessageIncoming = false;
        RawIndex = 0;
        IncomingMessageSize = 0;
      }
    }
  }
}

bool isValidChecksum( void ) {
  // Checksum
  // A checksum is appended to each message. It is the sum of the binary interpretation of all the
  // preceding bytes in the message (control characters and ignored characters excluded), taken modulus
  // 256. The checksum is computed on the 8-bit binary representation of the ASCII pair, rather than the
  // values of the individual ASCII characters. An example checksum calculation, with an overflow, is
  // shown below:
  
  // Example
  // Calculate the checksum for the message: 03h (Last Index), 7Ah, character, and 9Bh. The Last Index
  // is three because the message will contain three bytes: data byte: 7Ah, data byte: 9Bh, and the
  // checksum byte. The checksum calculation is a byte wide, sum of the binary data (excluding the
  // control characters).

  unsigned int lastIndex = CondorData[0];
  unsigned int checksum = 0;   
    
  for (int i = 0; i < lastIndex; i++ ) {
    checksum = (checksum + CondorData[i]) % 256;
  }
        
  checksum = checksum % 256;
        
  return (checksum == (CondorData[lastIndex] & 0xFF));
}

void ProcessCondorPacket( void ) {
  switch( CondorData[1] ) {
    case 0x02: // Automation Event Lost
      /* This command is sent if the panel’s automation buffer has overflowed resulting in the loss of system
          events. This command should result in a Dynamic Data Refresh and Full Equipment List request from
          the Automation Device. */
    case 0x20: // Clear Automation Dynamic Image
      /* This command is sent on panel power up initialization and when a communication failure restoral with
          the Automation Module occurs. The Concord will also send this command when user or installer
          programming mode is exited. */      
      // Need to perform system resync
      messageCallout( "Refresh" );
      break;
    case 0x21: // Zone Status
      /* This command is sent whenever there is a change in zone state (e.g. trip, restore, alarm, cancel,
          trouble, restoral, bypass, unbypass). Also, if the Automation Module requests a Dynamic Data
          Refresh Request this command will be sent for each zone that is not normal (i.e. any zone that is open
          (non restored), in alarm, troubled or bypassed). The remote device should assume that all zones are
          normal unless told otherwise. */
      //smartthing.send( RawInputBuffer );
      break;
    case 0x22:      
      switch( CondorData[2] ) {
        case 0x01: // Arming Level
          /* This command is sent whenever there is a change in the arming level. Also, if the Automation Module
              requests a Dynamic Data Refresh Request this command will be sent for each partition that is
              enabled. */
        case 0x02: // Alarm Trouble Code
          /* This command is sent to identify alarm and trouble conditions as well as several other system events.
              Events are specified by three numbers; General Type, Specific Type, and Event Specific Data. The
              lists below show all the events, categorized by General Type. */
          //smartthing.send( RawInputBuffer );
          break;        
        case 0x09: // Touchpad Display
          /* This command sends the touchpad display text tokens to the Automation Module. */
          if( bSendDisplayData > 0 && (CondorData[3] == DISPLAY_PARTITIION) )
          {
            bSendDisplayData = bSendDisplayData - 1;
            if( bSendDisplayData <= 0 )
            {
              // Stop the message updates
              messageCallout( "Dstop" );
            }
            //smartthing.send( RawInputBuffer );
          }                
          break;
        case 0x03: // Entry/Exit Delay
        case 0x04: // Siren Setup
        case 0x05: // Siren Synchronize
        case 0x06: // Siren Go
        case 0x0B: // Siren Stop
        case 0x0C: // Feature State
        case 0x0D: // Temperature
        case 0x0E: // Time and Date
        default:
          break;
      };
    case 0x23:      
      switch( CondorData[2] )
      {
        case 0x01: // Lights State Command
        case 0x02: // User Lights Command
        case 0x03: // Keyfob Command
          /* This command is sent whenever the panel receives a keypress from a keyfob. */
          break;
      };
  };
}

//---------------------------------------------------------------------//

void PrintCondorPacket( void )
{
  // CondorData[0] is the Last Index data byte 
  for( int i = 0; i <= CondorData[0]; i++ ) {
    debugSerial.print(CondorData[i], HEX);
  }
  
  debugSerial.println("");
}

//---------------------------------------------------------------------//

void messageCallout(String message)
{
  // if message contents equals to 'on' then call on() function
  // else if message contents equals to 'off' then call off() function
  if ( message.substring(0,2) == "TX" && message.length() >= 9 )
  {
    //Send the start character 0xA
    panelSerial.write( 0xA );
    for ( int i = 3; i < message.length(); i++ ) {      
      CurrentSendCommand[ i-3 ] = message[i];
      panelSerial.write( CurrentSendCommand[ i - 3 ] );
    }   
  }
  else if ( message.equals("Refresh") ) {
    sendDynamicDataRefreshRequest();
  }
  else if (message.equals("Disarm"))
  {
    //Disarm the system
    //TODO::encode the disarm command
    //smartthing.send("Disarmed");  
  }
  else if( message.equals("Dupdate") )
  {
    bSendDisplayData = 50;
    //smartthing.send("Dupdate");
  }
  else if( message.equals("Dstop") )
  {
    bSendDisplayData = 0;
    //smartthing.send("Dstop");
  }  
}

//---------------------------------------------------------------------//
// AUTOMATION TO PANEL COMMANDS
//---------------------------------------------------------------------//
void sendFullEquipmentListRequest( void ) {
    // Newline to start transmission
    panelSerial.write( 0x0A );
    // 0x02
    panelSerial.write( 0x30 );
    panelSerial.write( 0x32 );
    // 0x02
    panelSerial.write( 0x30 );
    panelSerial.write( 0x32 );
    // 0x04
    panelSerial.write( 0x30 );
    panelSerial.write( 0x34 );
}

void sendDynamicDataRefreshRequest( void ) {
    // Newline to start transmission
    panelSerial.write( 0x0A );
    // 0x02
    panelSerial.write( 0x30 );
    panelSerial.write( 0x32 );
    // 0x20
    panelSerial.write( 0x32 );
    panelSerial.write( 0x30 );
    // 0x22
    panelSerial.write( 0x32 );
    panelSerial.write( 0x32 );
}

//void sendKeypress (unsigned short partionNumber, unsigned short[] keyPresses) {
//
//}

