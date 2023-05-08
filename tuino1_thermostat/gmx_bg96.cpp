
#include "at_client.h"
#include "gmx_bg96.h"
#include "Regexp.h"
#include "Arduino.h"

#define DEBUG 1
//#define ECHO 1

String gmxSerial_String;
String dummy_Response;

// RegExp Engine
MatchState mst;
char buff[512]; 

void _log(String data )
{
  #ifdef DEBUG
    Serial.println(data);
  #endif
}

void _resetGMX()
{

  pinMode(GMX_RESET,OUTPUT);
  
  // Reset 
  digitalWrite(GMX_RESET,LOW);
  delay(200);
  digitalWrite(GMX_RESET,HIGH);
  delay(500);
  digitalWrite(GMX_RESET,LOW);
}

void _BG96PowerON()
{

  // 
  digitalWrite(GMX_GPIO4,LOW);
  delay(500);
  digitalWrite(GMX_GPIO4,HIGH);
  delay(500);
  digitalWrite(GMX_GPIO4,LOW);
}


void _GPIOInit()
{
    pinMode(GMX_GPIO1,OUTPUT);
    pinMode(GMX_GPIO2,OUTPUT);
    pinMode(GMX_GPIO3,OUTPUT);
    pinMode(GMX_GPIO4,OUTPUT);
    pinMode(GMX_GPIO5,OUTPUT);

    // RESET PIN SETUP
    pinMode(GMX_RESET,OUTPUT);
    digitalWrite(GMX_RESET,LOW);
    
    pinMode(GMX_GPIO4,LOW);
    
}

void _LedBootFlash()
{
    digitalWrite(GMX_GPIO1,LOW);
    digitalWrite(GMX_GPIO2,LOW);
    digitalWrite(GMX_GPIO3,LOW);
  
    digitalWrite(GMX_GPIO1,HIGH);
    delay(500);
    digitalWrite(GMX_GPIO1,LOW);
        
    digitalWrite(GMX_GPIO2,HIGH);
    delay(500);
    digitalWrite(GMX_GPIO2,LOW);
    
    digitalWrite(GMX_GPIO3,HIGH);
    delay(500);
    digitalWrite(GMX_GPIO3,LOW);
    
    digitalWrite(GMX_GPIO1,HIGH);
    delay(500);
    digitalWrite(GMX_GPIO1,LOW);
    
    digitalWrite(GMX_GPIO2,HIGH);
    delay(500);
    digitalWrite(GMX_GPIO2,LOW);
    
    digitalWrite(GMX_GPIO3,HIGH);
    delay(500); 
    digitalWrite(GMX_GPIO3,LOW);
  
    digitalWrite(GMX_GPIO1,LOW);
    digitalWrite(GMX_GPIO2,LOW);
    digitalWrite(GMX_GPIO3,LOW);

  
}

uint8_t gmxBG96_init() 
{
   
    _log("BG96 Init");

    _GPIOInit();
    _LedBootFlash();
    _BG96PowerON();
    _BG96PowerON();
    
    delay(500);
   
    // Init AT Interface
    at_serial_init(115200);
    _resetGMX();

    if ( at_read_for_response_single("RDY",BG96_BOOT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }

    _log("Boot OK - RDY!!!");

    at_send_command("ATE0");
    #ifdef ECHO
      at_send_command("ATE1"); // Echo the AT command on the serial
      _log("ECHO SET");
      if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
      {
      return GMX_BG96_KO;
      }    
    #endif

    at_send_command("AT+CMEE=2");
    if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    } 
     _log("GMX_BG96_OK");
     
     // Delay for SIM reading
     delay(1000);
     
     return GMX_BG96_OK;
}

uint8_t gmxBG96_IoT_Attach(char *apn, uint32_t band)
{ 
     at_send_command("AT+CIMI");
     if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }
    
     at_send_command("AT+COPS=?");
       if ( read_for_responses_dual("OK","ERROR",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      //_log("ERROR COPS");
      return GMX_BG96_KO;
    }

     at_send_command("AT+COPS=1,2,\"20820\",9");
     if ( read_for_responses_dual("OK","ERROR",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("ERROR COPS");
      //return GMX_BG96_KO;
    }

     at_send_command("AT+COPS?");
     if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      _log("ERROR COPS?");
      return GMX_BG96_KO;
    }

     at_send_command("AT+QNWINFO");

     if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      _log("ERROR QNWINFO");
      return GMX_BG96_KO;
    }

}

void gmxBG96_print_config()
{
  at_send_command("AT+CGDCONT?");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);
  at_send_command("AT+CGREG?");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);
}

void _strip_all_newlines(char *data )
{
  
  String _temp= String(data);

  _temp.replace("\n","");
  _temp.replace("\r","");
  
  _temp.toCharArray(data,_temp.length()+1);

}

byte _parse_Response(String& response) {
  
  char cmd[16];
  
  gmxSerial_String="";
  
  // Read Serial
  while (Serial1.available()>0) 
  {
      while (Serial1.available()>0) 
      {
        char c = Serial1.read(); 
        Serial.print(c);
        gmxSerial_String += c;
      }
      
      delay(10);
  }

  gmxSerial_String.toCharArray(cmd,gmxSerial_String.length());

  // Parse Response
  mst.Target(cmd);
  char result = mst.Match ("(.*)\r\nOK", 0);
  
  if ( result == REGEXP_MATCHED )
  { 
    mst.GetCapture (buff, 0);
    response = String(buff);
   
    // remove second \r\n => Not very elegant to optimize
    response.toCharArray(cmd,response.length());
    response = String(cmd);
    
    return (GMX_BG96_OK);
  }

}

byte gmxBG96_getIMEI(String& IMEI)
{
  
  at_send_command("AT+GSN");    
  return( _parse_Response(IMEI) );
}


uint8_t gmxBG96_getIMSI(char *data, uint8_t max_size)
{
  
  at_send_command("AT+CIMI");

  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
    
  return GMX_BG96_OK;
}

uint8_t gmxBG96_setGSM(char *apn)
{
  char cgdcont[128];

  at_send_command("AT+QCFG=\"nwscanseq\",03,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    
  
  sprintf(cgdcont,"AT+CGDCONT=1,\"IP\",\"%s\"",apn);

  at_send_command(cgdcont);
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    


  sprintf(cgdcont,"AT+QICSGP=1,1,\"%s\",\"\",\"\",1",apn);
  at_send_command(cgdcont);
  
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;   
    
  return GMX_BG96_OK;
}


uint8_t gmxBG96_setNBIoT_A(char *apn, uint32_t band)
{ 
     at_send_command("AT+CIMI");
     if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }

    at_send_command("AT+QCFG=\"iotopmode\",1,1");
     if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }

     at_send_command("AT+CGDCONT=1,\"IP\",\"ltebox\"");
     if ( at_read_for_response_single("OK",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }

     at_send_command("AT+CGDCONT?");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      return GMX_BG96_KO;
    }


     at_send_command("AT+CFUN=1");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("sw off radio not posible ??");
      return GMX_BG96_KO;
    }

     at_send_command("AT+QCFG=\"band\",0,0,8000000,1");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("BAND NOT OK ??");
      return GMX_BG96_KO;
    }

     at_send_command("AT+CFUN=0");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("sw on radio not posible ??");
      return GMX_BG96_KO;
    }

     at_send_command("AT+QCFG=\"band\"");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("BAND NOT OK");
      return GMX_BG96_KO;
    }
    
      at_send_command("AT+QCFG=\"nwscanseq\",03,1");
      
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("Network Operator NOT SET");
      return GMX_BG96_KO;
    }
      
     //at_send_command("AT+COPS=1,1,\"20892\",9");
     at_send_command("AT+COPS=?");
     //at_send_command("AT+COPS?");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("Network Operator NOT SET");
      return GMX_BG96_KO;
    }
/*
         at_send_command("AT+COPS=?");
     if ( at_read_for_response_single("OK",BG96_COPS_TIMEOUT, 1) != 0 )
    {
      _log("HERE ?");
      return GMX_BG96_KO;
    }
*/
    

  return GMX_BG96_OK;

  


  /*at_send_command("AT+CFUN=0");
  at_send_command("AT+QCFG=0");
  at_send_command("AT+QCFG=\"band\", 0, 0, 8000000, 1");
  at_send_command("AT+CFUN=1");
  at_send_command("AT+CGDCONT=1,\"IP\",\"ltebox\",,0,0");
  at_send_command("AT+COPS=?");
  */
  
}

uint8_t gmxBG96_setNBIoT(char *apn, uint32_t band)
{
  char temp_string[128];
  char reply[2048];
  
  at_send_command("AT+CPAS");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QCFG=\"nwscanseq\",03,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    

  at_send_command("AT+QCFG=\"nwscanmode\",3,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    

  at_send_command("AT+QCFG=\"iotopmode\",1,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    

/*
  at_send_command("AT+QCFG=\"roamservice\",2,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    
*/

  at_send_command("AT+QCFG=\"servicedomain\",1,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    

/*
  at_send_command("AT+QCFG=\"band\"");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
*/


  //at_send_command("AT+QCFG=\"nb1/bandprior\",1C");
  //if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
  //  return GMX_BG96_KO;

    
  at_send_command("AT+QCFG=\"band\", 0, 0, 80, 1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+COPS?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QCFG=?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

/*
  sprintf(temp_string,"AT+QCFG=\"band\",0x00000000,0x400A0E189F,0xA0E189F,1");
 
  Serial.print( "BAND SELECT: " );
  Serial.println( temp_string );

  at_send_command(temp_string);
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT) != 0 )
    return  GMX_BG96_KO;    
*/
/*
  sprintf(temp_string,"AT+CGREG=1");
  Serial.println( temp_string );
  at_send_command(temp_string);
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT) != 0 )
    return  GMX_BG96_KO;   

  sprintf(temp_string,"AT+CGDCONT=1,\"IP\",\"ltebox\",,0,0");
  Serial.println( temp_string );
  at_send_command(temp_string);  
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT) != 0 )
    return  GMX_BG96_KO;   
*/
/*
  sprintf(temp_string,"AT+CGACT=1,1");
  Serial.println( temp_string );
  at_send_command(temp_string);  
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT) != 0 )
    return  GMX_BG96_KO;   
*/

/*
  at_send_command("AT+CEREG?");
  if ( at_copy_serial_to_buffer(reply, 'O', 512,  BG96_DEFAULT_TIMEOUT) != AT_COMMAND_SUCCESS )
    return GMX_BG96_KO;

  Serial.print(reply);
*/  
/*
  at_send_command("AT+CPIN?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+CREG?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QCCID");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QSIMSTAT?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QSIMSTAT=1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QSIMSTAT?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+QNWINFO");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("ATV1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+COPS=?");
  if ( read_for_responses_dual("OK","ERROR",180000, 1) != 0 )
    return GMX_BG96_KO;
*/

  /* Disable the use of eDRX */
  at_send_command("AT+CEDRXS=0");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

#if 0
  /* Enter PSM after T3324 expires */
  at_send_command("AT+QCFG=\"psm/enter\",0");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
#endif

  /* PSM mode settings T3324=20s T3412_extended=120s */
  at_send_command("AT+CPSMS=1,,,\"10000100\",\"00001010\"");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

#if 0
  /* Enter PSM after T3324 expires */
  at_send_command("AT+QPSMCFG=60,5");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
#endif

/*
  at_send_command("AT+CGDCONT=1,\"IP\",\"ltebox\"");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
*/
  at_send_command("AT+QICSGP=1,1,\"ltebox\",\"\",\"\",1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

/*
  at_send_command("AT+COPS=1,1,\"208 92\",9");
  if ( read_for_responses_dual("OK","ERROR",180000, 1) != 0 )
    return GMX_BG96_KO;
*/
/*
  at_send_command("AT+CREG=2");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
*/
/*
  at_send_command("AT+CREG?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
*/
/*
  at_send_command("AT+COPN");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;
*/

  at_send_command("AT+CREG?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+CGREG?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  at_send_command("AT+CGATT?");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return GMX_BG96_KO;

  return GMX_BG96_OK;
}

uint8_t gmxBG96_BellLabs_Sequence()
{
}

uint8_t gmxBG96_setCatM1(char *apn)
{
  char cgdcont[128];

  at_send_command("AT+QCFG=\"nwscanseq\",01,1");
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    
  
  sprintf(cgdcont,"AT+CGDCONT=1,\"IP\",\"%s\"",apn);

  at_send_command(cgdcont);
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    


  sprintf(cgdcont,"AT+QICSGP=1,1,\"%s\",\"\",\"\",1",apn);
  at_send_command(cgdcont);
  
  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;   
    
  return GMX_BG96_OK;
}



uint8_t gmxBG96_isNetworkAttached(int *attach_status)
{
  char data[16];
  int network_status;

/*  
  at_send_command("AT+CREG?");
*/

  at_send_command("AT+QNWINFO");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  at_send_command("AT+CGPADDR");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  at_send_command("AT+CGATT?");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  at_send_command("AT+CGATT=1");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  memset(data,0,sizeof(data));
   
  if ( at_copy_serial_to_buffer(data, ':', 16,  BG96_DEFAULT_TIMEOUT) != AT_COMMAND_SUCCESS )
    return GMX_BG96_KO;

  memset(data,0,sizeof(data));
   
  if ( at_copy_serial_to_buffer(data, '\r', 16,  BG96_DEFAULT_TIMEOUT) != AT_COMMAND_SUCCESS )
    return GMX_BG96_KO;

  *attach_status = atoi(data);
  Serial.println( data );

/*
  at_send_command("AT+QNWINFO");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  at_send_command("AT+CGPADDR");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);

  at_send_command("AT+CGATT?");
  read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1);
*/
  return GMX_BG96_OK;
}


uint8_t gmxBG96_signalQuality(int *rssi, int *ber )
{
  char data[16];
  char *pch;
  
  at_send_command("AT+CSQ");

  memset(data,0,sizeof(data));
  if ( at_copy_serial_to_buffer(data, ':', 16,  BG96_DEFAULT_TIMEOUT) != AT_COMMAND_SUCCESS )
    return GMX_BG96_KO;

  memset(data,0,sizeof(data));
  if ( at_copy_serial_to_buffer(data, '\r', 16,  BG96_DEFAULT_TIMEOUT) != AT_COMMAND_SUCCESS )
    return GMX_BG96_KO;
    
  _strip_all_newlines( data );

  // we have 2 items
  pch = strtok (data,",");
  
  if (pch!=NULL)
  {
    *rssi = atoi( pch );
    pch = strtok (NULL,",");

    if ( pch != NULL )
    {
      *ber = atoi( pch );

      return GMX_BG96_OK;
    }
  }

   return  GMX_BG96_KO; 
}



uint8_t gmxBG96_SocketOpen(void)
{

  at_send_command("AT+QIOPEN=1,0,\"UDP\",\"10.11.2.1\",9200,0,1\r");

  if ( at_read_for_response_single("+QIOPEN: 0,0",10000, 1) != 0 )
        return  GMX_BG96_KO;   
 
  return GMX_BG96_OK;
}


uint8_t gmxBG96_SocketClose(void)
{

  at_send_command("AT+QICLOSE=0");

  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO;    
 
 
  return GMX_BG96_OK;
}


uint8_t gmxBG96_TXData(char *data, int data_len)
{
/*
  at_send_command("AT+QIOPEN=1,0,\"UDP\",\"5.79.89.3\",9200,0,1\r");

  if ( at_read_for_response_single("+QIOPEN: 0,0",10000) != 0 )
        return  GMX_BG96_KO;   

Serial.println("Socket Opened!");
*/



#if 0
   at_send_command("AT+QISEND=0");   

   if ( at_read_for_response_single(">",BG96_DEFAULT_TIMEOUT, 1) != 0 )
        return  GMX_BG96_KO;   
        
  Serial.println("Sending Data to Socket");

  for (int i=0; i<data_len; i++) 
  {
    at_send_char(data[i]);
    delay(1);
  }

  // Send CTRL-Z
  at_send_char(0x1A);

  if ( read_for_responses_dual("SEND OK","SEND FAIL",BG96_DEFAULT_TIMEOUT, 1) != 0 )
    return  GMX_BG96_KO; 

#endif
  // TRY pinging
  at_send_command("AT+QPING=1,\"10.11.2.1\"");
  delay(1);
  if ( read_for_responses_dual("OK","ERROR",4000, 1) != 0 )
    return GMX_BG96_KO;
  
/*
  at_send_command("AT+QICLOSE=0");

  if ( read_for_responses_dual("OK","ERROR",BG96_DEFAULT_TIMEOUT) != 0 )
    return  GMX_BG96_KO;    
 
 */
  return GMX_BG96_OK;
}
