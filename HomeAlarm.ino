#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

#define RST_PIN 9             
#define SS_PIN  10     
#define LED_PIN 6
#define PIR_PIN 7

void updateSerial();
void send_sms(char);
void gsm_signal_test();

SoftwareSerial mySerial(3, 2);      /* SIM800L Tx & Rx is connected to Arduino #3 & #2 */
MFRC522 mfrc522(SS_PIN, RST_PIN);   /* Create MFRC522 instance */

char id_flag=0;
char sms_select=0;
unsigned int timer_counter=0;
unsigned int timer_cycle=0;
char error_flag=0;
char error_counter=0;
char pirStat=0;
char pir_flag=0;
int serial_get=0;

void setup() 
{
    Serial.begin(9600);
    mySerial.begin(9600);                  /* Begin serial communication with Arduino and SIM800L */
    while (!Serial);                       

    /*/////////////////SPI-RFID/////////////////*/
    SPI.begin();                           /* Init SPI bus */
    mfrc522.PCD_Init();                    /* Init MFRC522 card */

    /*/////////////////PIR/////////////////*/
    pinMode(PIR_PIN,INPUT); 
    digitalWrite(PIR_PIN,LOW);

    /*/////////////////LED/////////////////*/
    pinMode(LED_PIN,OUTPUT);

    /*/////////////////GSM/////////////////*/
    Serial.println("GSM Initializing...");
    delay(1000);

    //mySerial.println("AT");                 	    /* Once the handshake test is successful, it will back to OK */
    //updateSerial();
    //mySerial.println("AT+CSQ");             	    /* Signal quality test, value range is 0-31 , 31 is the best */
    //updateSerial();
    /* mySerial.println("AT+CCID"); */            /* Read SIM information to confirm whether the SIM is plugged */
    /* updateSerial(); */
    //mySerial.println("AT+CREG?");           	    /* Check whether it has registered in the network */
    //updateSerial();
    /* mySerial.println("AT+COPS?");  */       	  /* Network check */
    /* updateSerial(); */
    /* mySerial.println("AT+COPS=?"); */       	  /* AGet operators */
    /* updateSerial(); */
    /* mySerial.println("AT+CMGF=1"); */          /* Configuring TEXT mode */
    /* updateSerial(); */
    /* mySerial.println("AT+CNMI=1,2,0,0,0"); */  /* Decides how newly arrived SMS messages should be handled */
    /* updateSerial(); */

    /*/////////////////TIMER0-CTC/////////////////*/
    cli();
    TCCR0A |= (1<<WGM01);                 /* CTC */
    TCCR0B |= (1<<CS00 | 1<<CS01);        /* CLK/ */
    OCR0A = 0xF9;                         /* 1ms */
    TIMSK0 |= (1<<OCIE0A);
    sei();
    
    /*/////////////////PRE-OPERATIONS/////////////////*/
    id_flag=1;
}

void loop() 
{
    if ( mfrc522.PICC_IsNewCardPresent())
    {
		  if ( mfrc522.PICC_ReadCardSerial())
		  {
			  Serial.println();
			  Serial.print("Tag UID:");
        
			  String access_uid="";
        
			  for(byte i = 0; i < mfrc522.uid.size; i++)
			  {
			    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
			    Serial.print(mfrc522.uid.uidByte[i], HEX);

			    access_uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
			    access_uid.concat(String(mfrc522.uid.uidByte[i], HEX));
			  }
        
			  Serial.println();
			  access_uid.toUpperCase();
			  if( (access_uid.substring(1)=="66 0D 6C F1") || (access_uid.substring(1)=="43 8E 1A 34") ||
				    (access_uid.substring(1)=="CC EF DE 22") || (access_uid.substring(1)=="C7 35 8C C8") ||
				    (access_uid.substring(1)=="87 AB 88 C9") ) 
			  {	
			
				  if(error_counter>=10)
				  {
					  sms_select=0;
					  send_sms(sms_select);
					  error_counter=0;
					  id_flag=2;				/* cant turn on */
			
					  Serial.println("ISMERETLEN HIBA HALMOZODAS!");
				  }
		  
				  switch(id_flag)
				  {
					  case 1:   digitalWrite(LED_PIN, HIGH); delay(10000); digitalWrite(LED_PIN, LOW); id_flag=2; error_flag=0; break;
					  case 2:   timer_cycle=0; timer_counter=0; pir_flag=0; id_flag=1; error_flag=0; sms_select=0; 			        break;
					  default:  timer_cycle=0; timer_counter=0; pir_flag=0; id_flag=1; error_flag=1; sms_select=0;
				  }
		  
				  if(error_flag!=0) { error_counter++; error_flag=0; Serial.println("ISMERETLEN HIBA!"); }
				  else Serial.println("Access Complete");
		  
				  if(id_flag==2) Serial.println("Alarm sys: ON");
				  if(id_flag==1) Serial.println("Alarm sys: OFF");
			  }
			  else if( (access_uid.substring(1)=="59 CF 94 B2") && (id_flag==1) )   /* turn off */
			  {
				  sms_select=3;
				  send_sms(sms_select);
			
				  Serial.println("TEST");
			  }
			  else
			  {
				  sms_select=2;
				  send_sms(sms_select);

				  timer_cycle=0;
				  timer_counter=0;
				  id_flag=1;
				  pir_flag=0;
				  sms_select=0;
        
				  Serial.println("Access Denied");
			  }
        
			  mfrc522.PICC_HaltA();
		  }
	  }
  
	  if( (id_flag>1) && (pir_flag==0) )
	  {
		  pirStat = digitalRead(PIR_PIN);
    
		  if(pirStat == HIGH) { pir_flag=1; timer_cycle=500; timer_counter=0; }
	  }
  
	  if(pir_flag>0)
	  {
		  if(timer_counter>=10000)
		  {
			  sms_select=1;
			  send_sms(sms_select);

			  timer_cycle=0;                   /* reset */
			  timer_counter=0;                
			  id_flag=1;                          
			  pir_flag=0;
			  sms_select=0;
      
			  Serial.println("SMS send");
		  }
	  }

	  if( (pir_flag>0) && (timer_cycle>=500) ) digitalWrite(LED_PIN, HIGH);
	  else digitalWrite(LED_PIN, LOW);                                        /* def. turn off LED */
  
	  if(timer_cycle>=1000) timer_cycle=0;

	  if(timer_counter>=50000) timer_counter=0;
	
	  /* GSM SIGNAL TEST */
	  if (Serial.available() > 0)
	  {
		  serial_get = Serial.read();

		  Serial.print("I received: ");
		  Serial.println(serial_get, DEC);

		  if(serial_get=='1') gsm_signal_test();
	  }

	  /* SMS OLVASÁS */
	  /* updateSerial(); */						/* ha SMS-t akarok olvasni de akkor a sorosporti olvasás nem megy */
}

ISR(TIMER0_COMPA_vect)                                  
{
	timer_cycle++;
	timer_counter++;
}

void updateSerial()
{
	delay(600);                                      
	while (Serial.available()) 
	{
		mySerial.write(Serial.read());                  /* Forward what Serial received to Software Serial Port */
	}
	while(mySerial.available()) 
	{
		Serial.write(mySerial.read());                  /* Forward what Software Serial received to Serial Port */
	}
}

void send_sms(char function)
{
	mySerial.println("AT+CMGF=1");                    /* Configuring TEXT mode */
	updateSerial();
	mySerial.println("AT+CMGS=\"+36xxxxxxxxx\"");
	updateSerial();
  
	switch(function)
	{
		case 1:  mySerial.print("Riasztas! Mozgas a hazban!"); 	break;
		case 2:  mySerial.print("Riasztas! Wrong ID!");         break;
		case 3:  mySerial.print("TESZT SMS"); 			            break;
		default: mySerial.print("HIBAS MUKODES");
	}
  
	updateSerial();
	mySerial.write(26);

	delay(10000);
	mySerial.println("AT+CMGF=1");
	updateSerial();
	mySerial.println("AT+CMGS=\"+36xxxxxxxxx\"");
	updateSerial();
  
	switch(function)
	{
		case 1:  mySerial.print("Riasztas! Mozgas a hazban!"); 	break;
		case 2:  mySerial.print("Riasztas! Wrong ID!");         break;
		case 3:  mySerial.print("TESZT SMS"); 			            break;
		default: mySerial.print("HIBAS MUKODES");
	}
  
	updateSerial();
	mySerial.write(26);
}

void gsm_signal_test()
{
	mySerial.println("AT+CSQ");             /* Signal quality test, value range is 0-31 */
	updateSerial();
}
