#include "Arduino.h"
#include "ser_comm.h"

u8 xcn;
u32 t1;
bool DataHere;
u8 serlen;

//----------------------------------------------------------
void ReadSerial(u8 *buffptr, void (*fun()))
{
    DataHere=false;
    if(Serial.available()>0)
    {
       t1=millis();
       buffptr[xcn]=Serial.read();
       xcn++; 
    }

    if(((millis()-t1)>TOUT) && (xcn!=0)) DataHere=true;

    if(DataHere==true)
    {
      serlen=xcn;
      xcn=0;
      DataHere=false;

      fun();
    }
}
//----------------------------------------------------------
