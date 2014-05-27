// TI4 PRIZE POOL TICKER

// MAX7219 Variables ******************************************

// Registers
byte max7219_reg_noop        = 0x00;
byte max7219_reg_digit0      = 0x01;
byte max7219_reg_digit1      = 0x02;
byte max7219_reg_digit2      = 0x03;
byte max7219_reg_digit3      = 0x04;
byte max7219_reg_digit4      = 0x05;
byte max7219_reg_digit5      = 0x06;
byte max7219_reg_digit6      = 0x07;
byte max7219_reg_digit7      = 0x08;
byte max7219_reg_decodeMode  = 0x09;
byte max7219_reg_intensity   = 0x0a;
byte max7219_reg_scanLimit   = 0x0b;
byte max7219_reg_shutdown    = 0x0c;
byte max7219_reg_displayTest = 0x0f;

// SPI Controls
int dataIn = A5;
int load = A2;
int clk = A3;

// Number of MAX7219's
int maxInUse = 1; 

// ************************************************************
// Global Stuff ***********************************************

// Request Strings
char dotaURL[] = "www.dota2.com";
char getString[] = "GET /jsfeed/intlprizepool HTTP/1.1";
char hostString[] = "Host: www.dota2.com";

// Globals
unsigned long lastUpdateTime = 0;
unsigned long updateInterval = 300000; // Milliseconds
byte dispArray[8];
int loops = 0;
bool reqInProgress = false;

// TCP Client object
TCPClient client;

// ************************************************************
// SPI Functions **********************************************

// Send a byte via SPI
void putByte(byte data) {
  byte i = 8;
  byte mask;
  while(i > 0) {
    mask = 0x01 << (i - 1);      // get bitmask
    digitalWrite(clk, LOW);   // tick
    if (data & mask){            // choose bit
      digitalWrite(dataIn, HIGH);// send 1
    }else{
      digitalWrite(dataIn, LOW); // send 0
    }
    digitalWrite(clk, HIGH);   // tock
    --i;                         // move to lesser bit
  }
}

// ***********************************************************
// MAX7219 Functions *****************************************

// Push to a single MAX7219
void maxSingle( byte reg, byte col) {    

  digitalWrite(load, LOW);       // begin     
  putByte(reg);                  // specify register
  putByte(col); //((data & 0x01) * 256) + data >> 1); // put data   
  digitalWrite(load, LOW);       // and load da shit
  digitalWrite(load,HIGH); 
}



void setup() {
    
    // Enable Serial
    Serial1.begin(9600);
    delay(1000);
    
    Serial1.println("*************************************");
    Serial1.println("******* DOTA COUNTER STARTING *******");
    Serial1.println("*************************************");
    
    pinMode(dataIn, OUTPUT);
    pinMode(clk, OUTPUT);
    pinMode(load, OUTPUT);
    
    // Initialize the MAX7219
    maxSingle(max7219_reg_scanLimit, 0x07);     // All digits are on
    maxSingle(max7219_reg_decodeMode, 0xFF);    // Digit mode
    maxSingle(max7219_reg_shutdown, 0x01);      // Not in shutdown mode
    maxSingle(max7219_reg_displayTest, 0x00);   // No display test
    maxSingle(max7219_reg_intensity, 0x02);     // Max intensity
    
    dispArray[7] = 0x0F;
    dispArray[6] = 0x0A;
    dispArray[5] = 0x0A;
    dispArray[4] = 0x0A;
    dispArray[3] = 0x0A;
    dispArray[2] = 0x0A;
    dispArray[1] = 0x0A;
    dispArray[0] = 0x0A;
    
    writeSseg();

}

void loop() {
    
    if (reqInProgress) {
        
        if (client.available()) {
            
            if (client.available() < 20) {
                client.flush();
            } else {
                // Response processing
                int numChars = client.available();
                char resString[numChars+1];
                Serial1.print("Response length: ");
                Serial1.println(numChars);
                
                
                
                for (int i = 0; i < numChars; i++) {
                    resString[i] = client.read();
                    
                    /*
                    // Print to serial
                    if (resString[i] == '\n') {
                        Serial1.println();
                    } else {
                        Serial1.print(resString[i]);
                    }
                    */
                }
                Serial1.println();
                
                
                for (int x = 0; x < numChars; x++) {
                    char d = resString[numChars-x];
                    if (d == '}') {
                        // Update the display array
                        Serial1.print("Excess chars: ");
                        Serial1.println(x);
                        
                        for (int i = 0; i < 7; i++){
                            char c = resString[numChars-x-1-i];
                            dispArray[i] = (byte)(c - 48);
                            
                            Serial1.print(c);
                            Serial1.print(" = ");
                            Serial1.println((byte)(c - 48));

                        }
                        // Update the display
                        writeSseg();
                        break;
                    }
                }
            }
        } else if (!client.connected() || millis() > lastUpdateTime) {
             // Disconnect
            Serial1.println();
            Serial1.print("Closing connection...");
            Serial1.println(loops);
            client.stop();
            reqInProgress = false;
            loops++;
            
        }
        
    } else if (millis() > lastUpdateTime) {
        
        Serial1.print("Last time: ");
        Serial1.println(lastUpdateTime);
        Serial1.print("Now: ");
        Serial1.println(millis());
        
        lastUpdateTime += updateInterval;
        
        // Send a new request
        Serial1.print("connecting to ");
        Serial1.println(dotaURL);
        
        if (client.connect(dotaURL, 80)) {
            Serial1.println("connected, sending GET");
            client.println(getString);
            client.println(hostString);
            client.println();
            reqInProgress = true;
        } else {
            Serial1.println("connection failed");
        }
    }
    
}

void writeSseg() {
    dispArray[3] = dispArray[3] | 0x80;
    dispArray[6] = dispArray[6] | 0x80;
    for (int i = 0; i < 8; i++) {
        maxSingle(i+1, dispArray[i]);
    }
    
    Serial1.println("Updated SSEG Display");
}


