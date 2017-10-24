#include <Wire.h>
#include <SerialCommand.h>
#include <ds3231.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// The SerialCommand object
SerialCommand sCmd;     

// RTC Global variable
#define BUFF_MAX 128
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 5000;
struct ts t;
char buff[BUFF_MAX];

void GET_time()
{
  DS3231_get(&t);
  // there is a compile time option in the library to include unixtime support
  #ifdef CONFIG_UNIXTIME
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d %ld", t.year,
           t.mon, t.mday, t.hour, t.min, t.sec, t.unixtime);
  #else
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d:w%d", t.year,
           t.mon, t.mday, t.hour, t.min, t.sec, t.wday);
  #endif
  Serial.println(buff);  
}

void SET_time()
{
  // SET TssmmhhWDDMMYYYY
  // Example : SET T014817405102017
  char *cmd;
 
  // Parser time parameter
  cmd = sCmd.next();

  // Set time to struct ts
  t.sec = inp2toi(cmd, 1);
  t.min = inp2toi(cmd, 3);
  t.hour = inp2toi(cmd, 5);
  t.wday = cmd[7] - 48;
  t.mday = inp2toi(cmd, 8);
  t.mon = inp2toi(cmd, 10);
  t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
  DS3231_set(t);
  Serial.println("OK");  
}

void ALARM1_GET()
{
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);
}

void ALARM2_GET()
{
  DS3231_get_a2(&buff[0], 59);
  Serial.println(buff);  
}

void Aging_GET()
{
  Serial.print("aging reg is ");
  Serial.println(DS3231_get_aging(), DEC);
}

void ALARM1_SET()
{
  // ALARM1SET TSSMMHHDD [flags[5]]
  // Example : A1SET T01000000 01111
  char *cmd,*cmd2;
  uint8_t i;
  uint8_t flags[5];
 
  // Parser time parameter
  cmd = sCmd.next();

  for (i = 0; i < 4; i++) {
    time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
  }
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  cmd2 = sCmd.next();
  for (i = 0; i < 5; i++) {
    flags[i] = (int)cmd2[i];
  }  
  
  DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);  
}

void ALARM2_SET()
{
  // ALARM2_SET TMMHHDD [flags[4]]
  // Example : ALARM2_SET T000000 01111
  char *cmd,*cmd2;
  uint8_t i;
  uint8_t flags[5];
 
  // Parser time parameter
  cmd = sCmd.next();

  DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
  for (i = 0; i < 3; i++) {
    time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
  }
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  cmd2 = sCmd.next();
  for (i = 0; i < 4; i++) {
    flags[i] = (int)cmd2[i];
  }  
  
  DS3231_set_a2(time[0], time[1], time[2], flags);
  DS3231_get_a2(&buff[0], 59);
  Serial.println(buff);  
}

void ALARM1_GET_STATUS()
{
  if (DS3231_triggered_a1())
    Serial.println("TRIGGED");
  else
    Serial.println("WAITING");
}

void ALARM2_GET_STATUS()
{
  if (DS3231_triggered_a2())
    Serial.println("TRIGGED");
  else
    Serial.println("WAITING");
}

void ALARM1_CLEAR()
{
  if (DS3231_triggered_a1()) {
    // INT has been pulled low
    Serial.println("CLEAR");
    // clear a1 alarm flag and let INT go into hi-z
    DS3231_clear_a1f();
  }
  else
  {
    Serial.println("OK");
  }
}

void ALARM2_CLEAR()
{
  if (DS3231_triggered_a2()) {
    // INT has been pulled low
    Serial.println("CLEAR");
    // clear a1 alarm flag and let INT go into hi-z
    DS3231_clear_a2f();
  }
  else
  {
    Serial.println("OK");
  }
}

void ALARM1_SET_NEXT_SECS()
{
  // A1NSEC [1~60]
  // Example : A1NSEC 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 60 ) )
  {
    Serial.println("ERROR");
    return;
  }  
  addTime = atoi(cmd);
   
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  if((((int)t.sec) + addTime) < 60)
  {
    time[0] = (int)t.sec + addTime;
    time[1] = (int)t.min;
  }
  else
  {
    time[0] = (int)t.sec + addTime - 60;
    time[1] = (int)t.min + 1;
  }
  time[2] = 0;
  time[3] = 0;
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 0, 1, 1, 1 };

  DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);  
}

void ALARM1_SET_NEXT_MINS()
{
  // A1NMIN [1~60]
  // Example : A1NMIN 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 60 ) )
  {
    Serial.println("ERROR");
    return;
  }  
  addTime = atoi(cmd);
   
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  time[0] = (int)t.sec;
  if((((int)t.min) + addTime) < 60)
  {
    time[1] = (int)t.min + addTime;
    time[2] = (int)t.hour;
  }
  else
  {
    time[1] = (int)t.min + addTime - 60;
    time[2] = (int)t.hour + 1;
  }
  time[3] = 0;
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 0, 0, 1, 1 };

  DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);    
}

void ALARM1_SET_NEXT_HOURS()
{
  // A1NHOUR [1~24]
  // Example : A1NHOUR 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 60 ) )
  {
    Serial.println("ERROR");
    return;
  }  
  addTime = atoi(cmd);
   
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  time[0] = (int)t.sec;
  time[1] = (int)t.min;
  if((((int)t.hour) + addTime) < 24)
  {
    time[2] = (int)t.hour + addTime;
  }
  else
  {
    time[2] = (int)t.hour + addTime - 24;
  }
  time[3] = 0;
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 0, 0, 1, 1 };

  DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);      
}

void ALARM1_SET_NEXT_DAYS()
{
  // A1NDAY [1~7]
  // Example : A1NDAY 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 60 ) )
  {
    Serial.println("ERROR");
    return;
  }  
  addTime = atoi(cmd);
   
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  time[0] = (int)t.sec;
  time[1] = (int)t.min;
  time[2] = (int)t.hour;
  if((((int)t.wday) + addTime) < 7)
  {
    time[3] = (int)t.wday + addTime;
  }
  else
  {
    time[3] = (int)t.wday + addTime - 7;
  }
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 0, 0, 0, 1 };

  DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
  DS3231_get_a1(&buff[0], 59);
  Serial.println(buff);     
}

void ALARM2_SET_NEXT_MINS()
{
  // A2NMIN [1~60]
  // Example : A2NMIN 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 60 ) )
  {
    Serial.println("ERROR");
    return;
  }  
  addTime = atoi(cmd);
  
  
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  if(((int)t.sec) > 30)
    addTime = addTime+1;
  if((((int)t.min) + addTime) < 59)
  {
    time[0] = (int)t.min + addTime;
  }
  else
  {
    time[0] = (int)t.min + addTime - 60;
  }
  time[1] = 0;
  time[2] = 0;
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 1, 1, 1 };

  DS3231_set_a2(time[0], time[1], time[2], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
  DS3231_get_a2(&buff[0], 59);
  Serial.println(buff);  
}

void ALARM2_SET_NEXT_HOURS()
{
  // A2NHOUR [1~24]
  // Example : A2NHOUR 1
  char *cmd;
  int addTime;

  // Parser time parameter
  cmd = sCmd.next();
  if ( (cmd == NULL) || (atoi(cmd) < 0) || (atoi(cmd) > 24 ) )
  {
    Serial.println("ERROR");
    return;
  }
  addTime = atoi(cmd);
  
  // Get Now time
  DS3231_get(&t);

  // Set wakeup time after setting mins
  time[0] = (int)t.min;
  if((((int)t.hour) + addTime) < 24)
  {
    time[1] = (int)t.hour + addTime;
  }
  else
  {
    time[1] = (int)t.hour + addTime - 24;
  }
  time[2] = 0;
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable) 
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  uint8_t flags[5] = { 0, 0, 1, 1 };

  DS3231_set_a2(time[0], time[1], time[2], flags);
  DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
  DS3231_get_a2(&buff[0], 59);
  Serial.println(buff);  
}

void GET_temp()
{
  Serial.print("TEMP:");
  Serial.println(DS3231_get_treg(), DEC);  
}

/* Function        :
 * Description     :
 * Input Parameter :
 * Return Value    :
 */
void backgroundTask() {
  
}

/* Function        :
 * Description     :
 * Input Parameter :
 * Return Value    :
 */
void setup()
{
    // Initializes the Serial Port 
    Serial.begin(9600);

    // Initial RTC
    Wire.begin();
    DS3231_init(DS3231_INTCN);
    memset(recv, 0, BUFF_MAX);
    //DS3231_clear_a1f();
    //DS3231_clear_a2f();

    // Setup callbacks for SerialCommand commands
    sCmd.addCommand("GET",     GET_time);
    sCmd.addCommand("SET",     SET_time);
    sCmd.addCommand("A1GET",   ALARM1_GET);
    sCmd.addCommand("A2GET",   ALARM2_GET);
    sCmd.addCommand("AGING",   Aging_GET);
    sCmd.addCommand("A1SET",   ALARM1_SET);
    sCmd.addCommand("A2SET",   ALARM2_SET);
    sCmd.addCommand("A1ST",    ALARM1_GET_STATUS);
    sCmd.addCommand("A2ST",    ALARM2_GET_STATUS);
    sCmd.addCommand("A1CLR",   ALARM1_CLEAR);
    sCmd.addCommand("A2CLR",   ALARM2_CLEAR);
    sCmd.addCommand("A1NSEC",  ALARM1_SET_NEXT_SECS);
    sCmd.addCommand("A1NMIN",  ALARM1_SET_NEXT_MINS);
    sCmd.addCommand("A1NHOUR", ALARM1_SET_NEXT_HOURS);
    sCmd.addCommand("A1NDAY",  ALARM1_SET_NEXT_DAYS);
    sCmd.addCommand("A2NMIN",  ALARM2_SET_NEXT_MINS);
    sCmd.addCommand("A2NHOUR", ALARM2_SET_NEXT_HOURS);
    sCmd.addCommand("TEMP",    GET_temp);

}

/* Function        :
 * Description     :
 * Input Parameter :
 * Return Value    :
 */
void loop()
{
  // Process serial commands
  sCmd.readSerial();
  // Run the background Task 
  backgroundTask();
}


