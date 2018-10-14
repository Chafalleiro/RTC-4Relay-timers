//RTC-4Relay timers v.01. Alfonso Abelenda Escudero 2018
//******************************************************************************
// Pins
	// ZS-042: SDA-A4(PC5),SCL-A5(PC4),SQW-pin 2 (PD2)
	// This sketch demonstrates how to use the ZS-042 RTC
	// and a 4 way relay module to swtih the relays in a timely programmed way
	// using the serial port to set the alarm and timer parameters.

	// There are ten alarms, two form the RTC (RTCAlarm) than can be scheduled 
	// in a concrete day or dayly manner. Eight are Arduino programmed alarms
	// that occurs in a 24 hour cycle. Each of the latter are associated
	// with a timer that can be one time triggered or repeated in cycles.

//TODO: routine to change EEPROM cell when they reach their writes lifespan, that can be around seven years.
//TODO use AT24C32 EEPROM to get advantage of its 1M cicles of rewriting. 32Kb 1M rewrites. Seven decades of use per register writing one every minute.
//TODO more actions doable with timers.
//TODO: LCD display routines.
//TODO: Make something useful with the RTC alarms.
//TODO: portable control GUI in Win, Mac and Linux.
//TODO: Full mweekday actions.

//******************************************************************************

#include <EEPROM.h>
#include <TimeLib.h>			// https://github.com/PaulStoffregen/Time
#include <DS3232RTC.h>			// https://github.com/JChristensen/DS3232RTC
#include <Streaming.h>			// http://arduiniana.org/libraries/streaming/
#include <elapsedMillis.h>		// https://github.com/pfeerick/elapsedMillis/wiki

//******************************************************************************
//mweekday(t);       // day of the week for the given time t
struct StructAlarms { //10 bytes. Can be 7 bytes.
	int myStatus;
	time_t myTime;
	int myModifier;
	int myAction;
};
int timeonRepeats[8];
//These are the default values of the alarms
StructAlarms myAlarms[8] ={{0, 1514764802,0 ,0 },{0, 1514764803,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764805,0 ,0 },{0, 1514764808,0 ,0 },{0, 1514764809,0 ,0 },{0, 1514764807,0 ,0 },{0, 1514764806,0 ,0 }}; //80 bytes, start 0
StructAlarms myTimersOn[8] ={{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 }}; //80 bytes, start 80
StructAlarms myTimersOff[8] ={{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 },{0, 10,0 ,0 }}; //80 bytes, , start 160, 240 total

StructAlarms myTempStruct[8] ={{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764800,0 ,0 },{0, 1514764900,0 ,0 }};

struct StructTimers //6 bytes. Can be 5 bytes.
{
	int mySw;
	time_t myCountdown; 
};
StructTimers myTimersOnCtd[8] ={{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}; //48 bytes, start 240,
StructTimers myTimersOffCtd[8] ={{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}; //48 bytes, start 288, 96 bytes total, EEPROM data size 336 bytes + 2 control bytes

byte weekDayOff[8] = {0,0,0,0,0,0,0,0}; //Starts at 338

struct Holyday { //4 bytes
	byte hDay;
	byte hMonth;
	byte hYear; //BEWARE Two digits year
	boolean hCyclic;
};
Holyday holydays[30] ={{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};//120 bytes, starts at 346
byte mVerbosity = 0;
byte mSleeptime = 0;
	
elapsedMillis timeElapsed; //declare global if you don't want it reset every time loop runs
elapsedMillis ledTimeElapsed;
elapsedMillis guiTimeElapsed;
elapsedMillis minuteTimeElapsed;
elapsedMillis timeOnElapsed[8];
elapsedMillis timeOffElapsed[8];

byte guimode = 0;

unsigned int ledInterval = 1000;
unsigned int guiInterval = 750;
unsigned int minuteInterval = 60000;

byte RelayControl[8] = {7,6,5,4,7,6,5,4};

boolean sleepSW = 0;

byte ledStatus = 13;
boolean ledState = LOW;

//******************************************************************************
//******************************************************************************
//******************************************************************************
void setup()
 {
char myLabel[] = "AA";
byte i = 0;
int eeAddress = 0;
boolean outgage = 0;
time_t t;

pinMode(ledStatus, OUTPUT);

for (i=0;i< 4; i++)
	{
	pinMode(RelayControl[i], OUTPUT);
	}
Serial.begin(115200);

//Serial << F("Arduino Time is ");
t = now();
//printDateTime(now());
//Serial << F(" t=") << t << F("\n");
if (t==0)
	{
	setSyncProvider(RTC.get);	 // We get the time of the RTC and initialize the time functions. Every 5 min the time is adjusted.
//	RTC.set(1531161062);	//Set testing time 09 Jul 2018 18:31:02
//	setTime(1531161062);
//	RTC.set(1531133383);	//Set testing time 09 Jul 2018 10:49:43
//	setTime(1531133383);	//Set testing time 09 Jul 2018 10:49:43
//	setTime(1531223383);	//1531223383 datetime= 10 Jul 2018 11:49:43	
//	RTC.set(1531223383);
	outgage = 1;
	}
	else
	{
	setSyncProvider(RTC.get);
	}

if(timeStatus() != timeSet)
	Serial.println(F("Unable to sync with the RTC"));
else
	{
	Serial.println(F("RTC has set the system time"));
	Serial << F("Time is ");
	printDateTime(now());
	Serial << "\n";
	}

/*	Setup our work variables and defaults
	Arduino alarms and timers. the ZS-042 RTC only includes two inbuilt alarms, so we use our own set of them for
	daily cyclic uses. We set them in the EEPROM to restore properly after a blackout.
	RTC alarms can be set to long term operations as you can set a date for them and are stored in the RTC.
	EEPROM can be rewritten around 100k times.
	http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_Summary.pdf
*/	
//	EEPROM.put(336, "AA");// Uncomment to reset EEPROM data to defaults


EEPROM.get(336, myLabel[0]);
EEPROM.get(337, myLabel[1]);
	
if (String(myLabel) == "Ch")	//	Byte 241 of the EEPROM must be "C", 242 "h"
	{
	Serial << eeAddress << F(" ") << String(myLabel) << F(" EPPROM already initialized\n");
	Serial << F("Alarms info is loaded\n");
	//We read the alarms data and feed our work variables with the last saved data.
	GetAlarms(1);
	GetTimersOn(1);
	GetTimersOff();
	GetTimersWeekdayOffs();
	GetHolydays();
	EEPROM.get(471, mVerbosity);
	Serial << F("Verbosity is  ") << mVerbosity << F("\n");
	EEPROM.get(472, mSleeptime);
	Serial << F("Sleeptime is  ") << mSleeptime << F("\n");
	}
else	//If not we initialize the memory with default values
	{ //Total data is 356 butes
	Initialize();
	}

for (i=0;i<8;i++)//Display the status of alarms and timers
	{
		printDateTime(myTimersOnCtd[i].myCountdown);
		Serial << F(" Timer On countdown ") << i << F(" Time. Active=") << myTimersOnCtd[i].mySw << F("\n");
	}
for (i=0;i<8;i++)
	{
		printDateTime(myTimersOffCtd[i].myCountdown);
		Serial << F(" Timer Off countdown ") << i << F(" Time. Active=") << myTimersOffCtd[i].mySw << F("\n");
	}
	for (i=0;i< 4; i++)//Setup the relays
	{
		digitalWrite(RelayControl[i],HIGH);
		delay(250);
	}
//Recover alarms
if(outgage == 1)
	{
	Serial << F("Circuit has resseted, trying to resume interrupted operations...\n");
	RecoverActions();
	}
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
void loop() {
time_t t;

//******************************************************************************
//If we are in active time check if any alarm is raised.
if(sleepSW == 0)
	{
	CheckTimers();
	if (ledTimeElapsed > ledInterval) 
		{				
		ledState = !ledState;         // toggle the state from HIGH to LOW to HIGH to LOW ... 
		digitalWrite(ledStatus, ledState);
		ledTimeElapsed = 0;              // reset the counter to 0 so the counting starts over...
		}
	}
else
{
	CheckWakeUp();	
	if (guiTimeElapsed > guiInterval) // Every minute we save the current time We can do this for 7 years.	
	{
	Serial  << F("Sleeping");
	}
}
WaitForCommands();
if (guimode == 1 && guiTimeElapsed > guiInterval) // Every minute we save the current time We can do this for 7 years.
	{
	guiTimeElapsed = 0;
	Serial  << F("GUIRTC ") << year(now()) << F(";") << ((month(now())<10) ? "0" : "") << month(now()) << F(";") << ((day(now())<10) ? "0" : "") << day(now()) << F(";") << ((hour(now())<10) ? "0" : "") << hour(now()) << F(";") << ((minute(now())<10) ? "0" : "") << minute(now()) << F(";")  << ((second(now())<10) ? "0" : "") << second(now()) << F("\n");
	}

if (minuteTimeElapsed > minuteInterval) // Every minute we save the current time We can do this for 7 years.
	{
	t = now();
	EEPROM.put(467, t);
	printDateTime(t);
	minuteTimeElapsed = 0;
	Serial << F(" Time Saved\n");
	}
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

void printDateTime(time_t t)
{
		Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t)) << " ";
		Serial << monthShortStr(month(t)) << " " << _DEC(year(t)) << ' ';
		Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
		Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
		Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}

void printTime(time_t t)
{
		Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
		Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
		Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}
//******************************************************************************
void CheckTimers()
{
time_t t;
byte i = 0;
int eeAddress = 0;
unsigned int interval = 1000;	// 1 second elapsed time

for (i=0;i<8;i++)
	{
	if (myAlarms[i].myStatus == 1 && myAlarms[i].myAction < 60  && sleepSW == 0)
		{
		t = now();
		t = myAlarms[i].myTime - t;
		CheckDayOff();
		if (t >= 0 && t < 10)	// 10 secs of margin to an alarm to raise if there was a little outgage.
			{
			Serial << F("t=") << t << F("...\n");
			Serial << F("Alarm ") << i << F(" raised...\n");
			myTimersOn[myAlarms[i].myAction].myStatus = myAlarms[i].myStatus; //Activate Timer
			// We add a day to the Alarm activating date, since they are scheluled in a daily based and we don't need it until tomorrow.
			// Also we won't the same timer activated again. 84600s = 24h
			myAlarms[i].myTime = myAlarms[i].myTime + 86400;
			timeonRepeats[myAlarms[i].myAction] = myTimersOn[myAlarms[i].myAction].myModifier;
			eeAddress = i * 10;//Calculate the EEPROM addres of the alarm
			EEPROM.put(eeAddress, myAlarms[i]);	//Put the updated alarm info
			}
		}
	else
		{
		t = now();
		t = myAlarms[i].myTime - t;
		if (t >= 0 && t < 10)	// 10 secs of margin to an alarm to raise if there was a little outgage.
			{	
			if (myAlarms[i].myStatus == 1 && myAlarms[i].myAction == 88)
				{
				myAlarms[i].myTime = myAlarms[i].myTime + 86400;
				int eeAddress = i * 10;//Calculate the EEPROM addres of the alarm
				EEPROM.put(eeAddress, myAlarms[i]);	//Put the updated alarm info
				Serial << F("\n");
				Serial << F("Alarm ") << i << F(" raised...\n");
				Serial << F("t=") << t << F("...\n");
				SleepTimeNow();
				Serial << F("\n");
				}
			else
				{CheckWakeUp();}
			}
		}
	}	
	//Check if any timer On is active
for (i=0;i<8;i++)
	{
	if (myTimersOn[i].myStatus == 1  && sleepSW == 0)
		{
		digitalWrite(RelayControl[myTimersOn[i].myAction],LOW);// NO1 and COM1 Connected (LED on) 
		if (timeOnElapsed[i] > interval)
			{	
			myTimersOnCtd[i].myCountdown++;
			if (mVerbosity == 1)
				{
				Serial << F("Timer On ") << i << F(" on Relay ") << myTimersOn[i].myAction << F(":") << RelayControl[myTimersOn[i].myAction] << F(" ");
				printTime(myTimersOnCtd[i].myCountdown);
				Serial << F(" is active, checking its countdown...\n");
				}
			timeOnElapsed[i] = 0;	// reset the counter to 0 so the counting starts over...
			if (myTimersOnCtd[i].myCountdown >= myTimersOn[i].myTime)
				{
				Serial << F(" Reached timer ") << i << F(" ON countdown on Relay ") << myTimersOn[i].myAction << F(" ");
				printTime(myTimersOnCtd[i].myCountdown);
				Serial << F(" ...\n");
				myTimersOn[i].myStatus = 0;
				myTimersOff[i].myStatus = 1;
				digitalWrite(RelayControl[myTimersOn[i].myAction],HIGH);// NO1 and COM1 Connected (LED off)
				myTimersOffCtd[i].myCountdown = 0;//Put the test value in counter 4
				}
			}
		}
	}
//Check if any timer Off is active	
for (i=0;i<8;i++)
	{
	if (myTimersOff[i].myStatus == 1  && sleepSW == 0)
		{
		digitalWrite(RelayControl[myTimersOff[i].myAction],HIGH);// NO1 and COM1 Connected (LED on) 
		if (timeOffElapsed[i] > interval)
			{
			myTimersOffCtd[i].myCountdown++;
			if (mVerbosity == 1)
				{
				Serial << F("Timer Off ") << i << F(" on Relay ") << myTimersOff[i].myAction << F(":") << RelayControl[myTimersOff[i].myAction] << F(" ");
				printTime(myTimersOffCtd[i].myCountdown);
				Serial << F(" is active, checking its countdown...\n");
				}
			timeOffElapsed[i] = 0;	// reset the counter to 0 so the counting starts over...
			if (myTimersOffCtd[i].myCountdown >= myTimersOff[i].myTime)
				{
				Serial << F(" Reached timer ") << i << F(" OFF countdown on Relay ") << myTimersOff[i].myAction << F(" ");
				printTime(myTimersOffCtd[i].myCountdown);
				Serial << F(" ...\n");
				myTimersOff[i].myStatus = 0;
				if (myTimersOn[i].myModifier == 1) //If Timer is repeatable
					{
					myTimersOn[i].myStatus = 1;
					myTimersOnCtd[i].myCountdown = 0;//Put the test value in counter 4
					}
				else
					{
					if (myTimersOn[i].myModifier > 1 && timeonRepeats[i] > 1) //If repeats are limited
						{
						timeonRepeats[i]--;
						Serial << F(" Reached timer ") << i << F(" OFF countdown on Relay ") << myTimersOff[i].myAction << F(" ");
						printTime(myTimersOffCtd[i].myCountdown);
						Serial << F(" It has ")<< timeonRepeats[i] << F(" repeats left ...\n");
						myTimersOn[i].myStatus = 1;
						myTimersOnCtd[i].myCountdown = 0;//Put the test value in counter 4
						}
					}
				}
			}	
		}
	}	
}
//******************************************************************************
boolean CheckHolydays()
{
boolean holyday = false;
time_t t,ht;
byte i = 0;
tmElements_t tm;
int tYear;
int eeAddress = 346;
//char* mweekday = {" MtWTFsS"};
//char* mweekday = {"DLMXJVS "};
String weekly = "DLMXJVS ";

t = now();
tm.Hour = int(hour(t)); tm.Minute = int(minute(t)); tm.Second = int(second(t));
for (i=0;i<30;i++) //Check for holyday.
	{
	EEPROM.get(eeAddress, holydays[i]);
	eeAddress += 4;
	tYear = (int)holydays[i].hYear + 2000;
	tm.Day = (int)holydays[i].hDay;  tm.Month = (int)holydays[i].hMonth; tm.Year = tYear - 1970;
	ht = makeTime(tm);
//	Serial << F(" ")<< i << F(" holyday...\n");
	if (holydays[i].hCyclic < 2 && ht == t)
		{
		if (sleepSW == 0)
			{
			printDateTime(ht);
			Serial << F("\n");
			Serial << F("Today is an scheduled holyday, going to sleep...\n");
			Serial << F("\n");
			}
		holyday = true;
		if (holydays[i].hCyclic == 0) //One timer
			{
				holydays[i].hCyclic = 2;
				EEPROM.put(eeAddress, holydays[i]);	
			}
		}
	}
eeAddress = 0;
tm.Month = int(month(t));  tm.Day = int(day(t)); tm.Year = int(year(t)) - 1970;
ht = makeTime(tm);
for (i=0;i<8;i++) //Check for weekly dayOff.
	{
	//boolean weekDO = (weekDayOff[i] & (128 >> (weekday(t) - 1)))?true:false; //Has this alarm a dayOff?
	if ((weekDayOff[i] & (128 >> (weekday(t) - 1)))?true:false)
		{
		if (month(t) == month(myAlarms[i].myTime) && day(t) == day(myAlarms[i].myTime) && year(t) == year(myAlarms[i].myTime))
			{
			tm.Hour = int(hour(myAlarms[i].myTime)); tm.Minute = int(minute(myAlarms[i].myTime)); tm.Second = int(second(myAlarms[i].myTime));
			myAlarms[i].myTime = makeTime(tm) + 86400;
			Serial <<F("Alarm ")<< i << F(" has Day off today, scheduling it to tomorrow\n");
			eeAddress = i * 10;//Calculate the EEPROM addres of the alarm
			EEPROM.put(eeAddress, myAlarms[i]);	//Put the updated alarm info
			}
		}
	}
return holyday;
}

void CheckDayOff()
{
int eeAddress = 0;
tmElements_t tm;
time_t t, ht;
byte i;
t = now();
tm.Month = int(month(t));  tm.Day = int(day(t)); tm.Year = int(year(t)) - 1970;
tm.Hour = int(hour(t)); tm.Minute = int(minute(t)); tm.Second = int(second(t));
ht = makeTime(tm);
for (i=0;i<8;i++) //Check for weekly dayOff.
	{
	if ((weekDayOff[i] & (128 >> (weekday(now()) - 1)))?true:false)//Has this alarm a dayOff?
		{
		if (month(now()) == month(myAlarms[i].myTime) && day(now()) == day(myAlarms[i].myTime) && year(now()) == year(myAlarms[i].myTime))
			{
			tm.Hour = int(hour(myAlarms[i].myTime)); tm.Minute = int(minute(myAlarms[i].myTime)); tm.Second = int(second(myAlarms[i].myTime));
			myAlarms[i].myTime = makeTime(tm) + 86400;
			Serial <<F("Alarm ")<< i << F(" has Day off today, scheduling it to tomorrow\n");
			eeAddress = i * 10;//Calculate the EEPROM addres of the alarm
			EEPROM.put(eeAddress, myAlarms[i]);	//Put the updated alarm info
			}
		}
	}
}
//******************************************************************************
void CheckWakeUp()
{
time_t t;
byte i = 0;
//Check if it's a Holyday
boolean mHolyday = CheckHolydays();
if (mHolyday == false)
	{
// "actions" can be a number representing a TimerOn to activate, 88 to Sleep or 99 to Awake the board, 77 for a day off, 66 for 48h off.
	for (i=0;i<8;i++)
		{
		if (myAlarms[i].myAction == 99 && myAlarms[i].myStatus == 1)
			{
			t = now();
			t = t - myAlarms[i].myTime;
			if (t >= 0 && t < 30)	// 30 secs of margin
				{
				Serial << F("-----------=========== Awaking ===========-----------\n");
				Serial << F("Alarm ") << i << F(" raised...\n");
				Serial << F("Waking up...\n");
				Serial << F("Time is ");
				t = now();
				printDateTime(now());
				sleepSW = 0;
				mSleeptime = 0;
				EEPROM.put(472, mSleeptime);
				// We add a day to the Alarm activating date, since they are scheluled in a daily based and we don't need it until tomorrow.
				// Also we won't the same timer activated again. 84600s = 24h
				myAlarms[i].myTime = myAlarms[i].myTime + 86400;
				int eeAddress = i * 10;//Calculate the EEPROM addres of the alarm
				EEPROM.put(eeAddress, myAlarms[i]);	//Put the updated alarm info
				Serial << F("\n*******************************************\n");
				}
			}
		}
	}
else
	{
	sleepSW = 1;
	}
}
//******************************************************************************
/* List of commands:
 Set the date and time by entering the following on the Arduino
 serial monitor if the RTC has an wrong date and time:
 Where "number" is an integer between 0 and 7
 "year" is a string of four numbers
 "month" is a number between 1 and 12
 "day" is a number between 1 and 31
 "hour" is a number between 0 and 23
 "minute" is a number between 0 and 59
 "second" is a number between 0 and 59
 "randomeventswitch" is a boolean, can be true, false, 0 or 1
 "repeatable" is an int
 "relay" a number representing one of the four relays controlled by the relay board. Between 0 and 3.
 "actions" can be a number representing a TimerOn to activate, 88 to Sleep or 99 to Awake the board.
******************************************************************************
 Use SetTime to set the RTC time and date

 Use SetAlarm to set an alarm that will repeat itself daily.
 Usage; "SetAlarm 0 16:00:00 0 n" with n between 0-7 will start TimerOn n every day at 4PM in the first alarm slot.
 Usage; "SetAlarm 2 18:00:00 0 88" will put the circuit in "Sleep" mode at 6PM, all the timers will be ignored and only Alarms ontaining "WakeUp" command will be considered, relays active will be deactivated.
 Usage; "SetAlarm 3 08:00:00 0 99" will put the circuit in "WakeUp" or working mode at 8AM, all alarms and timers will be considered.

 Use SetOnTimer to set on a Relay and tell the Arduino how much time it will be on
 Use SetOffTimer to set off a Relay and tell the Arduino how much time it will be off
 SetOnTimer and SetOffTimer are better used in pairs so you can contro better start and stop times of the relays
 Usage; "SetOnTimer 1 00:45:00 1 1"	"SetOffTimer 1 00:10:00 1 1" Will turn on Relay 1 45 minutes, and turn it off 10 until you change the commands or Sleep is issued.

 SetTime year/month/day hour:minute:second
 SetAlarm number hour:minute:second randomeventswitch actions
 SetOnTimer number hours:minutes:seconds repeatable relay
 SetOffTimer number hours:minutes:seconds repeatable relay
 
 SetAlarm number hour:minute:second randomeventswitch actions
 SetOnTimer number hours:minutes:seconds repeatable relay
 SetOffTimer number hours:minutes:seconds repeatable relay
 SetRTCAlarm number year/month/day hour:minute:second
 SetRTCAlarm number year/month/day hour:minute:second
 
Use ActTimer n {0-1}, ActTimerOff n  {0-1}, ActAlarm n  {0-1}, ActRelay n  {0-1} to activate or deactivate timers, relays and alarms.
*/
//******************************************************************************
void WaitForCommands()
{
byte i = 0;
byte j = 0;
int eeAddress = 0;
time_t t;
tmElements_t tm;
boolean foundCommand = 0;
//Check the serial port for input values
// Calculate based on max input size expected for one command
#define INPUT_SIZE 62
char input[INPUT_SIZE + 1];
char* command;	
int argument[19];
//char* mweekday = {" MtWTFsS"};
char* mweekday = {"DLMXJVS "};
//str = Serial.readStringUntil('\n');
// Add the final 0 to end the C string
byte size = Serial.readBytes(input, INPUT_SIZE);
input[size] = 0;
String myCommand = "";
//String weekly = "MtWTFsS"";
String weekly = "DLMXJVS ";

// Get next command from Serial (add 1 for final 0)
// Read each command pair 
command = strtok (input," ;:/");
myCommand = command;
myCommand.trim();

//GUIMODE
if (myCommand.lastIndexOf("GUIMODE") >= 0) // GUIMODE [0]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	if (argument[0] == 1)
		{
		Serial << F("GUI mode active \n");
		guimode = 1;
		}
	else
		{
		Serial << F("GUI mode inactive\n");
		guimode = 0;
		}	
	foundCommand = 1;	
	}
//PrtTime
if (myCommand.lastIndexOf("PrtTime") >= 0) // SetOnTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
	{
	Serial << F("Time is ");
	printDateTime(now());
	Serial << F("\n");
	foundCommand = 1;
	}
//SetTime 2018/05/28 00:00:01
if (myCommand.lastIndexOf("SetTime") >= 0) // SetTime 2018/05/28 00:00:01
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	
	tm.Day = argument[2]; tm.Month = argument[1]; tm.Year = argument[0] - 1970; //Timer date is 01/01/1970
	tm.Hour = argument[3]; tm.Minute = argument[4]; tm.Second = argument[5] + 1;
	t = makeTime(tm);
	RTC.set(t);        // use the time_t value to ensure correct mweekday is set
	setTime(t);
	Serial << F("RTC Time Set to int= ") << t <<  F(" datetime= ");
	printDateTime(t);
	Serial << F("\n");
	foundCommand = 1;	
	}
//SetOnTimer 1 00:01:02 1 2
if (myCommand.lastIndexOf("SetOnTimer") >= 0) // SetOnTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myTimersOn[argument[0]].myModifier = argument[4];
	myTimersOn[argument[0]].myAction = argument[5];
	myTimersOn[argument[0]].myStatus = 0;

	tm.Day = 1; tm.Month = 1; tm.Year = 0; //Timer date is 01/01/1970
	tm.Hour = argument[1]; tm.Minute = argument[2]; tm.Second = argument[3];
	myTimersOn[argument[0]].myTime = makeTime(tm);

	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 80;	//Add displacement
	EEPROM.put(eeAddress, myTimersOn[argument[0]]);	//Put the updated alarm info
	Serial << F("\n Timer On ") << argument[0] << F(" Set to ");
	printTime(myTimersOn[argument[0]].myTime);
	Serial << F(" Active=") << myTimersOn[argument[0]].myStatus << F(" Relay=") << myTimersOn[argument[0]].myAction << F(" Repeatable=") << myTimersOn[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetTimersOn(0);
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetOffTimer 1 00:00:33 1 2
if (myCommand.lastIndexOf("SetOffTimer") >= 0) // SetOfTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myTimersOff[argument[0]].myModifier = argument[4];
	myTimersOff[argument[0]].myAction = argument[5];
	myTimersOff[argument[0]].myStatus = 0;

	tm.Day = 1; tm.Month = 1; tm.Year = 0; //Timer date is 01/01/1970
	tm.Hour = argument[1]; tm.Minute = argument[2]; tm.Second = argument[3];
	myTimersOff[argument[0]].myTime = makeTime(tm);

	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 160;	//Add displacement
	EEPROM.put(eeAddress, myTimersOff[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << F("\n Timer Off ") << argument[0] << F(" Set to ");
	printTime(myTimersOff[argument[0]].myTime);
	Serial << F(" Active=") << myTimersOff[argument[0]].myStatus << F(" Relay=") << myTimersOff[argument[0]].myAction << F(" Repeatable=") << myTimersOff[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetTimersOff();
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetAlarm 7 00:20:23 0 1
if (myCommand.lastIndexOf("SetAlarm") >= 0) // SetAlarm[0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-Random [5]-Timer
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myAlarms[argument[0]].myModifier = argument[4];
	myAlarms[argument[0]].myAction = argument[5];
	myAlarms[argument[0]].myStatus = 0;

	t = now();
	tm.Month = int(month(t));  tm.Day = int(day(t)); tm.Year = int(year(t)) - 1970;
	tm.Hour = argument[1]; tm.Minute = argument[2]; tm.Second = argument[3];
	myAlarms[argument[0]].myTime = makeTime(tm);

	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 0;	//Add displacement
	EEPROM.put(eeAddress, myAlarms[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << F("\n Alarm ") << argument[0] << F(" Set to ");
	printTime(myAlarms[argument[0]].myTime);
	Serial << F(" Active=") << myAlarms[argument[0]].myStatus << F(" On Timer=") << myAlarms[argument[0]].myAction << F(" Random=") << myAlarms[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetAlarms(1);
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetAlDate 7 2018/06/09
if (myCommand.lastIndexOf("SetAlDate") >= 0) // SetAlDate [0]-number [1]-Year/[2]-Month/[3]-Day
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}

	t = now();
	tm.Year = argument[1] - 1970; tm.Month = argument[2];  tm.Day = argument[3];
	tm.Hour = hour(myAlarms[argument[0]].myTime); tm.Minute = minute(myAlarms[argument[0]].myTime); tm.Second = second(myAlarms[argument[0]].myTime);
	myAlarms[argument[0]].myTime = makeTime(tm);

	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 0;	//Add displacement
	EEPROM.put(eeAddress, myAlarms[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << F("\n Alarm date ") << argument[0] << F(" Set to ");
	printDateTime(myAlarms[argument[0]].myTime);
	Serial << F(" Active=") << myAlarms[argument[0]].myStatus << F(" On Timer=") << myAlarms[argument[0]].myAction << F(" Random=") << myAlarms[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetAlarms(1);
	Serial << F("\n");	
	foundCommand = 1;
	}
//ActTimer 1 1
if (myCommand.lastIndexOf("ActTimer") >= 0) // ActTimer 1 1 [0]-number [1]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myTimersOn[argument[0]].myStatus = argument[1];
	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 80;	//Add displacement
	EEPROM.put(eeAddress, myTimersOn[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << "\n Timer " << argument[0] << " Set to ";
	printTime(myTimersOn[argument[0]].myTime);
	Serial << " Active=" << myTimersOn[argument[0]].myStatus << " On Timer=" << myTimersOn[argument[0]].myAction << " Random=" << myTimersOn[argument[0]].myModifier << " Saved on EEPROM address " << eeAddress << "\n";
	Serial << F("\n");	
	foundCommand = 1;
	}
//ActTimerOff 1 0
if (myCommand.lastIndexOf("ActTimerOff") >= 0) // ActTimerOff 1 1 [0]-number [1]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myTimersOff[argument[0]].myStatus = argument[1];
	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 160;	//Add displacement
	EEPROM.put(eeAddress, myTimersOff[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << F("\n Timer Off ") << argument[0] << F(" Set to ");
	printTime(myTimersOff[argument[0]].myTime);
	Serial << F(" Active=") << myTimersOff[argument[0]].myStatus << F(" On Timer=") << myTimersOff[argument[0]].myAction << F(" Random=") << myTimersOff[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetTimersOff();
	Serial << F("\n");	
	foundCommand = 1;
	}	
//ActAlarm 7 1
if (myCommand.lastIndexOf("ActAlarm") >= 0) // ActAlarm 1 1 [0]-number [1]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	myAlarms[argument[0]].myStatus = argument[1];
	eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
	eeAddress = eeAddress + 0;	//Add displacement
	EEPROM.put(eeAddress, myAlarms[argument[0]]);	//Put the updated alarm info
	delay(10);
	Serial << F("\n Alarm ") << argument[0] << F(" Set to ");
	printTime(myAlarms[argument[0]].myTime);
	Serial << F(" Active=") << myAlarms[argument[0]].myStatus << F(" On Timer=") << myAlarms[argument[0]].myAction << F(" Random=") << myAlarms[argument[0]].myModifier << F(" Saved on EEPROM address ") << eeAddress << F("\n");
	GetAlarms(1);
	Serial << F("\n");	
	foundCommand = 1;
	}
//ActRelay 1 1 
if (myCommand.lastIndexOf("ActRelay") >= 0) // SetOfTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	digitalWrite(RelayControl[argument[0]],argument[1]);// NO1 and COM1 Connected (LED on) 
	Serial << F("\n RelayControl[") << i << F("] Pin ") << RelayControl[argument[0]] << F("  Changed to ") << argument[1] << F("\n");
	Serial << F("\n");	
	foundCommand = 1;	
	}
//DisplayAlarms 0
if (myCommand.lastIndexOf("DisplayAlarms") >= 0) // DisplayAlarms [0]-alarm number
	{
	Serial << F("\n");
	command = strtok (NULL, " :/");
	if (isDigit(command[0]))
		{
		argument[0] = atoi(command);
		printDateTime(myAlarms[argument[0]].myTime);
		Serial << F(" is Alarm ") <<  argument[0] << F(" Time. Active=") << myAlarms[argument[0]].myStatus << F(" Timer=") << myAlarms[argument[0]].myAction << F(" Random=") << myAlarms[argument[0]].myModifier << F("\n");
		printTime(myTimersOn[myAlarms[argument[0]].myAction].myTime);
		Serial << F(" is Timer On ") <<  myAlarms[argument[0]].myAction << F(" Time. Active=") << myTimersOn[myAlarms[argument[0]].myAction].myStatus << F(" Relay=") << myTimersOn[myAlarms[argument[0]].myAction].myAction << F(" Repeatable=") << myTimersOn[myAlarms[argument[0]].myAction].myModifier << F("\n");
		printTime(myTimersOff[myAlarms[argument[0]].myAction].myTime);
		Serial << F(" is Timer Off ") <<  myAlarms[argument[0]].myAction << F(" Time. Active=") << myTimersOff[myAlarms[argument[0]].myAction].myStatus << F(" Relay=") << myTimersOff[myAlarms[argument[0]].myAction].myAction << F(" Repeatable=") << myTimersOff[myAlarms[argument[0]].myAction].myModifier << F("\n");
		GetTimersWeekdayOff(argument[0]);
		}
	else
		{ //Else we show all alarms.
		GetAlarms(0);
		Serial << F("           =-=-=-=-=-=-=-=-=               \n");
		GetTimersOn(0);
		Serial << F("           =-=-=-=-=-=-=-=-=               \n");
		GetTimersOff();
		Serial << F("           =-=-=-=-=-=-=-=-=               \n");
		GetTimersWeekdayOffs();
		}
	Serial << F("\n");
	if (guimode == 1)
		{
		for (i=0;i< 8; i++)
			{
			//We show the actual status of the alarms variables.
			Serial  << F("GUIALM ") << i << F("-"); //Index
			Serial << year(myAlarms[i].myTime) << F(";"); //alarm date
			Serial << ((month(myAlarms[i].myTime)<10) ? "0" : "") << month(myAlarms[i].myTime) << F(";");
			Serial << ((day(myAlarms[i].myTime)<10) ? "0" : "") << day(myAlarms[i].myTime) << F(";");
			Serial << ((hour(myAlarms[i].myTime)<10) ? "0" : "") << hour(myAlarms[i].myTime) << F(";");
			Serial << ((minute(myAlarms[i].myTime)<10) ? "0" : "") << minute(myAlarms[i].myTime) << F(";");
			Serial << ((second(myAlarms[i].myTime)<10) ? "0" : "") << second(myAlarms[i].myTime) << F("-");
			Serial << myAlarms[i].myStatus << F(";") << myAlarms[i].myAction << F(";") << myAlarms[i].myModifier << F("-"); //Alarm data, active, timer, random

			Serial << year(myTimersOn[myAlarms[i].myAction].myTime) << F(";"); //Timer ON date
			Serial << ((month(myTimersOn[myAlarms[i].myAction].myTime)<10) ? "0" : "") << month(myTimersOn[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((day(myTimersOn[myAlarms[i].myAction].myTime)<10) ? "0" : "") << day(myTimersOn[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((hour(myTimersOn[myAlarms[i].myAction].myTime)<10) ? "0" : "") << hour(myTimersOn[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((minute(myTimersOn[myAlarms[i].myAction].myTime)<10) ? "0" : "") << minute(myTimersOn[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((second(myTimersOn[myAlarms[i].myAction].myTime)<10) ? "0" : "") << second(myTimersOn[myAlarms[i].myAction].myTime) << F("-");
if(myAlarms[i].myAction < 88)
{
Serial << myTimersOn[myAlarms[i].myAction].myStatus << F(";") << myTimersOn[myAlarms[i].myAction].myAction << F(";") << myTimersOn[myAlarms[i].myAction].myModifier << F("-"); //Timer data, active, relay, repeats
}
else{Serial << F("0;0;0-");}
			Serial << year(myTimersOff[myAlarms[i].myAction].myTime) << F(";"); //Timer OFF date
			Serial << ((month(myTimersOff[myAlarms[i].myAction].myTime)<10) ? "0" : "") << month(myTimersOff[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((day(myTimersOff[myAlarms[i].myAction].myTime)<10) ? "0" : "") << day(myTimersOff[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((hour(myTimersOff[myAlarms[i].myAction].myTime)<10) ? "0" : "") << hour(myTimersOff[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((minute(myTimersOff[myAlarms[i].myAction].myTime)<10) ? "0" : "") << minute(myTimersOff[myAlarms[i].myAction].myTime) << F(";");
			Serial << ((second(myTimersOff[myAlarms[i].myAction].myTime)<10) ? "0" : "") << second(myTimersOff[myAlarms[i].myAction].myTime) << F("-");
if(myAlarms[i].myAction < 88)
{
Serial << myTimersOff[myAlarms[i].myAction].myStatus << F(";") << myTimersOff[myAlarms[i].myAction].myAction << F(";") << myTimersOff[myAlarms[i].myAction].myModifier << F("-"); //Timer data, active, relay, repeats
}
else{Serial << F("0;0;0-");}
			
			Serial << weekDayOff[i] << F("-\n");
			}
		}
	foundCommand = 1;
	}
//ResetAllTheAlarmsNow
if (myCommand.lastIndexOf("ResetAllTheAlarmsNow") >= 0) // ResetAllTheAlarmsNow
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	Serial << F("\n");	
	Serial << F("Initializing All alarms\n");
	Initialize();
	Serial << F("Done initializing\n");	
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetWeekdayOff 0 L 1
if (myCommand.lastIndexOf("SetWeekdayOff") >= 0) // SetWeekdayOff [0]-alarm [1]-Weekoff day (L,M,X,J,V,S,D, or M,t,W,T,F,s,S)  [2]-working day 0,1
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			if (isAlpha(command[0]))
				{
				myCommand = command;
				}
			else 
				{
				argument[i] = atoi(command);
				}
			i++;
		}
	if(argument[2] == 1)
		{
		weekDayOff[argument[0]] = weekDayOff[argument[0]] | (128 >> weekly.lastIndexOf(myCommand));
		}
	else
		{
		byte temp = (254 << (7 - weekly.lastIndexOf(myCommand))) | (254 >> (-(7 - weekly.lastIndexOf(myCommand)) & 7));
		weekDayOff[argument[0]] = weekDayOff[argument[0]] & temp;
		}
	int eeAddress = argument[0] + 338;
	EEPROM.put(eeAddress, weekDayOff[argument[0]]);
	Serial << F("\n");	
	Serial << F("Weekly Dayoffs for alarm") << argument[0] << F(" are set to ");
	j = 0;
	for (byte mask = 128; mask; mask >>= 1)
		{
		Serial.print(mask&weekDayOff[argument[0]]?mweekday[j]:'_');
		j++;
		}
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetHolyday 0 01/01/18 1
if (myCommand.lastIndexOf("SetHolyday") >= 0) // SetHolyday [0]-holyday index [0]-day [1]-month [2]-year [3]-yearly (0 do once - 1 yearly - 2 deactivated)
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	holydays[argument[0]].hDay = (byte)argument[1];
	holydays[argument[0]].hMonth = (byte)argument[2];
	holydays[argument[0]].hYear = (byte)argument[3];
	holydays[argument[0]].hCyclic = (byte)argument[4];
	eeAddress = 346;
	EEPROM.put(eeAddress, holydays[argument[0]]);
	Serial << F(" Holyday ") << argument[0] << F(" is now ") << holydays[argument[0]].hDay << F("/") << holydays[argument[0]].hMonth << F("/")  << holydays[argument[0]].hYear << F(" yearly repeatable ") << holydays[argument[0]].hCyclic << F("\n");
	foundCommand = 1;
	}
//DisplayHolydays
if (myCommand.lastIndexOf("DisplayHolydays") >= 0) // DisplayHolydays
	{
	Serial << F("\n");	
	GetHolydays();
	Serial << F("\n");	
	foundCommand = 1;
	}
//Verbosity 1
if (myCommand.lastIndexOf("Verbosity") >= 0) // Verbosity 1 [0]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	mVerbosity = (byte)argument[0];
	EEPROM.put(471, mVerbosity);
	delay(10);
	Serial << F("\n");	
	Serial << F(" Verbosity is set to ") << argument[0] << F("\n");
	Serial << F("\n");	
	foundCommand = 1;
	}
//Sleep 1
if (myCommand.lastIndexOf("Sleep") >= 0) // Sleep 1 [0]-boolean
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	mSleeptime = (byte)argument[0];
	EEPROM.put(472, mSleeptime);
	delay(10);
	Serial << F("\n");	
	Serial << F(" Sleep is set to ") << argument[0] << F("\n");
	if(mSleeptime == 1){Serial << F("Feeling Sleepy, zzz.... \n");SleepTimeNow();}
	else{Serial << F("Waking up... \n");sleepSW = 0;CheckWakeUp();}
	Serial << F("\n");	
	foundCommand = 1;
	}
//SetData n aaaa;mm;dd;hh;mm;ss;hh;mm;ss;hh;mm;ss;n;n;nn;n;aaaa;mm;dd;hh;mm;ss;hh;mm;ss;hh;mm;ss;n;n;nn;nnn
//SetData 7 2018;12;31;00;00;17;00;00;17;00;00;17;1;7;7;6;208
if (myCommand.lastIndexOf("SetData") >= 0) // GUI command, see RTC4RlaysFB "void RTC4RlaysFBDialog::btnSetAlarms( wxCommandEvent& event )" function
	{
	Serial << F("\n");	
	while (command != NULL)
		{
			command = strtok (NULL, " ;");
			argument[i] = atoi(command);
			i++;
		}
		tm.Year = argument[1] - 1970; tm.Month = argument[2];  tm.Day = argument[3];
		tm.Hour = argument[4]; tm.Minute = argument[5]; tm.Second = argument[6];
		myAlarms[argument[0]].myTime = makeTime(tm);
		myAlarms[argument[0]].myStatus = argument[13];
		myAlarms[argument[0]].myAction = argument[14];
		eeAddress = argument[0] * 10;//Calculate the EEPROM addres of the alarm
		EEPROM.put(eeAddress, myAlarms[argument[0]]);	//Put the updated alarm info
		delay(10);	
		Serial << F("Timer ") << argument[14] << F(" wrote\n");
		if(argument[14] < 88)
			{
			tm.Day = 1; tm.Month = 1; tm.Year = 0; //Timer date is 01/01/1970
			tm.Hour = argument[7]; tm.Minute = argument[8]; tm.Second = argument[9];
			myTimersOn[myAlarms[argument[0]].myAction].myTime = makeTime(tm);
			tm.Hour = argument[10]; tm.Minute = argument[11]; tm.Second = argument[12];
			myTimersOff[argument[14]].myTime = makeTime(tm);
			myTimersOn[argument[14]].myModifier = argument[15];
			myTimersOn[argument[14]].myAction = argument[16];
			myTimersOff[argument[14]].myModifier = 0;
			myTimersOff[argument[14]].myAction = argument[16];
			Serial << F("has action ") << myTimersOn[argument[14]].myAction << F(" and repeats ") << myTimersOn[argument[14]].myModifier;
			eeAddress = argument[14] * 10;//Calculate the EEPROM addres of the timer on info
			eeAddress = eeAddress + 80;	//Add start of the block
			EEPROM.put(eeAddress, myTimersOn[myAlarms[argument[0]].myAction]);	//Put the updated timer on info
			delay(10);			

			eeAddress = argument[14] * 10;//Calculate the EEPROM addres of the timer off info
			eeAddress = eeAddress + 160;	//Add displacement
			EEPROM.put(eeAddress, myTimersOff[myAlarms[argument[0]].myAction]);	//Put the updated timer off info
			delay(10);
			}
		Serial << F(" wrote\n");		
		weekDayOff[argument[0]] = argument[17];
		eeAddress = argument[0] + 338;
		EEPROM.put(eeAddress, weekDayOff[argument[0]]);
		delay(10);
		Serial << F("Data for alarm ") << argument[0] << F(" wrote\n");
	Serial << F("\n");	
		foundCommand = 1;
	}

//DisplayGUIInfos
if (myCommand.lastIndexOf("DisplayGUIInfos") >= 0) // DisplayGUIInfos 0  - DisplayGUIInfos [0] - int
	{
	while (command != NULL)
		{
			command = strtok (NULL, " :/");
			argument[i] = atoi(command);
			i++;
		}
	switch (argument[0])
		{
		case 0: //Return RTC time and date
			tm.Month = int(month(now()));  tm.Day = int(day(now())); tm.Year = int(year(now())) - 1970;
			tm.Hour = int(hour(now())); tm.Minute = int(minute(now())); tm.Second = int(second(now()));
			
			Serial  << "GUIRTC;" << year(now()) << F(";") << ((month(now())<10) ? "0" : "") << month(now()) << F(";") << ((day(now())<10) ? "0" : "") << day(now()) << F(";") << ((hour(now())<10) ? "0" : "") << hour(now()) << F(";") << ((minute(now())<10) ? "0" : "") << minute(now()) << F(";")  << ((second(now())<10) ? "0" : "") << second(now()) << F("\n");
			break;
		}
	foundCommand = 1;
	}
	
//No Command
if (foundCommand == 0 && command)
	{
	Serial << F("\n");	
	Serial << F("Warning ") << command << F(" is not a valid command\n");
	Serial << F("\n");	
	}
}
//******************************************************************************
void GetAlarms(byte showInfo)
{
int eeAddress = 0;
int i;
for (i=0;i< 8; i++)
	{
	if (showInfo == 1) //We populate the memory of the alarms.
		{
		EEPROM.get(eeAddress, myAlarms[i]);
		eeAddress += 10;
		delay(10);
		}
	 //We show the actual status of the alarms variables.
	printDateTime(myAlarms[i].myTime);
	Serial << F(" Alarm ") << i << F(" Time. Active=") << myAlarms[i].myStatus << F(" Timer=") << myAlarms[i].myAction << F(" Random=") << myAlarms[i].myModifier << F("\n");
	}
}

void GetTimersOn(byte showInfo)
{
int eeAddress = 80;
int i;
for (i=0;i< 8; i++)
	{
	if (showInfo == 1) //We populate the memory of the alarms.
		{
		EEPROM.get(eeAddress, myTimersOn[i]);
		eeAddress += 10;
		delay(10);
		}
	printDateTime(myTimersOn[i].myTime);
	Serial << F(" Timer On ") << i << F(" Time. Active=") << myTimersOn[i].myStatus << F(" Relay=") << myTimersOn[i].myAction << F(" Repeatable=") << myTimersOn[i].myModifier << F("\n");
	}
}

void GetTimersOff()
{
int eeAddress = 160;
byte i;
for (i=0;i< 8; i++)
	{
	EEPROM.get(eeAddress, myTimersOff[i]);
	eeAddress += 10;
	delay(10);
	printDateTime(myTimersOff[i].myTime);
	Serial << F(" Timer Off ") << i << F(" Time. Active=") << myTimersOff[i].myStatus << F(" Relay=") << myTimersOff[i].myAction << F(" Repeatable=") << myTimersOff[i].myModifier << F("\n");
	}

}
void GetTimersWeekdayOffs()
{
byte i;
int eeAddress = 338;
for (i=0;i< 8; i++)
	{
	EEPROM.get(eeAddress, weekDayOff[i]);
	eeAddress += 1;
	GetTimersWeekdayOff(i);
	}
}
void GetTimersWeekdayOff(byte alarm)
{
//char* mweekday = {" MtWTFsS"};
char* mweekday = {"DLMXJVS "};	
	Serial << F("Weekly DayOffs of Alarm ") << alarm << F(" are ");
	byte i = 0;
	for (byte mask = 0x80; mask; mask >>= 1)
		{
		Serial.print(mask&weekDayOff[alarm]?mweekday[i]:'_');
		i++;
		}
	Serial << F("\n");	
}
void GetHolydays()
{
byte i;
int eeAddress = 346;
for (i=0;i< 30; i++)
	{

	EEPROM.get(eeAddress, holydays[i]);
	eeAddress += 4;
	Serial << F(" Holyday ") << i << F(" is ") << holydays[i].hYear << F("/") << holydays[i].hMonth << F("/") << holydays[i].hDay << F(" yearly repeatable ") << holydays[i].hCyclic << F(" (yy/mm/dd)\n");
	}
}
//******************************************************************************
void RecoverActions()
{
time_t t, lastTimeRecorded, alarmValues[8];
tmElements_t tm, tm_alarm;
int i=0;
int j=0;
int myIndexes[8] = {0,1,2,3,4,5,6,7};
int eeAddress = 0;
EEPROM.get(472, mSleeptime);
	t = now();	//Prepare a date to inject temporary structure
	tm.Month = int(month(t));  tm.Day = int(day(t)); tm.Year = int(year(t)) - 1970;
	t = makeTime(tm);
//	int eeAddress = 0;
	EEPROM.get(467, lastTimeRecorded);
	Serial << F("Last time recorded ");
	printDateTime(lastTimeRecorded);
	Serial << F("\n");
	for(i=0;i<8;i++)								//Prepare the arrays to be sorted and deactivate timers
		{											//We work with a temporary struct cause alarm date maybe wrong.
		myTempStruct[i].myTime = myAlarms[i].myTime;
		tm.Hour = int(hour(myTempStruct[i].myTime)); tm.Minute = int(minute(myTempStruct[i].myTime)); tm.Second = int(second(myTempStruct[i].myTime));	
		myTempStruct[i].myTime = makeTime(tm);
		alarmValues[i] = myTempStruct[i].myTime;
		myTimersOn[i].myStatus = 0;
		myTimersOff[i].myStatus = 0;
		}
	sortDates(alarmValues, 8); //Sort the alarms

	for(i=0;i < 8; i++) //Sort the indexes array
		{
		for(j=0;j < 8;j++)
			{
			if (alarmValues[i] == myTempStruct[j].myTime)
				{
				myIndexes[i] = j;
				}
			}
		}
	for(i=0;i < 8; i++)
		{	
		if (int(day(myAlarms[i].myTime)) < int(day(now())))
			{
			if (myTempStruct[myIndexes[i]].myTime < now())
				{
				tm_alarm.Day = int(day(now())) + 1;
				}
			else
				{
				tm_alarm.Day = int(day(now()));
				}				
			tm_alarm.Month = int(month(now())); tm_alarm.Year = int(year(now())) - 1970;								
			tm_alarm.Hour = int(hour(myAlarms[i].myTime)); tm_alarm.Minute = int(minute(myAlarms[i].myTime)); tm_alarm.Second = int(second(myAlarms[i].myTime));	
			t = makeTime(tm_alarm);
			myAlarms[i].myTime = t;
			EEPROM.put(eeAddress, myAlarms[i]);
			eeAddress += 10;
			delay(10);
			}
		}
	i=7;
	while (i>=0) //Fire the alarms since last wake up time if their timer is repeatable.
		{
		if (myTempStruct[myIndexes[i]].myTime < now())
			{
			if(myAlarms[myIndexes[i]].myAction == 88 || myAlarms[myIndexes[i]].myAction == 99)
				{
				CheckWakeUp();
				Serial << myAlarms[myIndexes[i]].myAction << F(" Finished restoring\n");
				i= -1;
				}
			else
				{
				Serial << F("Found Alarm: ") << myIndexes[i] <<  F(" Active: ") << myAlarms[myIndexes[i]].myStatus << F(" ");
				printDateTime(myAlarms[myIndexes[i]].myTime);
				if (myAlarms[myIndexes[i]].myStatus == 1 && myTimersOn[myAlarms[myIndexes[i]].myAction].myModifier >= 1)
					{
					if ((weekDayOff[i] & (128 >> (weekday(now()) - 1)))?true:false)//Has this alarm a dayOff?
						{
						Serial << F(" But has a weekly DayOff today\n");
						}
					else
						{
						myTimersOn[myAlarms[myIndexes[i]].myAction].myStatus = 1;
						timeonRepeats[myAlarms[myIndexes[i]].myAction] = myTimersOn[myAlarms[myIndexes[i]].myAction].myModifier;
						Serial << F(" Launching its timer\n");
						}
					}
				else
					{
					Serial << F(" but is inactive or its timer is not repeatable\n");
					}
				i--;
				}
			}
		else
			{
			i--;
			}
		}
if (mSleeptime == 0)
	{
	sleepSW = 0;
	}
else
	{
	sleepSW = 1;
	Serial << F("Sleeptime is  ") << mSleeptime << F(" nothing to do, sleeping...\n");
	}
}

void sortDates(time_t a[], int size) { //Bubble sort algo http://hwhacks.com/2016/05/03/bubble-sorting-with-an-arduinoc-application/
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    time_t t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                }
        }
    }
}
//******************************************************************************
void Initialize()
{
int eeAddress = 0;
int i = 0;

for (i=0;i< 8; i++)
	{

	myAlarms[i].myStatus = 0;
	myAlarms[i].myTime = 1514764800 + i;
	myAlarms[i].myModifier = 0;
	myAlarms[i].myAction = 0;
	EEPROM.put(eeAddress, myAlarms[i]);
	eeAddress+= 10;
	}
for (i=0;i< 8; i++)
	{
	myTimersOn[i].myStatus = 0;
	myTimersOn[i].myTime = 10;
	myTimersOn[i].myModifier = 0;
	myTimersOn[i].myAction = 0;
	EEPROM.put(eeAddress, myTimersOn[i]);
	eeAddress += 10;
	}
for (i=0;i< 8; i++)
	{
	myTimersOff[i].myStatus = 0;
	myTimersOff[i].myTime = 10;
	myTimersOff[i].myModifier = 0;
	myTimersOff[i].myAction = 0;
	EEPROM.put(eeAddress, myTimersOff[i]);
	eeAddress += 10;
	}
	for (i=0;i< 8; i++)
	{
	myTimersOnCtd[i].mySw = 0;
	myTimersOnCtd[i].myCountdown = 0;
	EEPROM.put(eeAddress, myTimersOnCtd[i]);
	eeAddress += 6;
	}
	for (i=0;i< 8; i++)
	{
	myTimersOffCtd[i].mySw = 0;
	myTimersOffCtd[i].myCountdown = 0;
	EEPROM.put(eeAddress, myTimersOnCtd[i]);
	eeAddress += 6;
	}
	EEPROM.put(eeAddress, "Ch");
	eeAddress += 2;

	for (i=0;i< 8; i++)
	{
	weekDayOff[i] = 0;
	EEPROM.put(eeAddress, weekDayOff[i]);
	eeAddress += 1;
	}

	for (i=0;i< 30; i++)
	{
	holydays[i].hDay = i;
	holydays[i].hMonth = 0;
	holydays[i].hYear = 0;
	holydays[i].hCyclic = 0;
	EEPROM.put(eeAddress, holydays[i]);
	eeAddress += 4;
	}

	EEPROM.put(471, 1);
	EEPROM.put(472, 0);
	Serial << F(" EEPROM has been initialized\n");
	GetAlarms(1);
	GetTimersOn(1);
	GetTimersOff();
	GetTimersWeekdayOffs();
	GetHolydays();
	EEPROM.get(471, mVerbosity);
	Serial << F("Verbosity is  ") << mVerbosity << F("\n");
	EEPROM.get(472, mSleeptime);
	Serial << F("Sleeptime is  ") << mSleeptime << F("\n");	
}

void SleepTimeNow()
{
byte j=0;
mSleeptime = 1;
Serial << F("Going to sleep...\n");
EEPROM.put(472, mSleeptime);
Serial << F("Setting Sleeptime bit...\n");			
Serial << F("Setting timers off...\n");
for (j = 0; j < 8; j++)
	{
	myTimersOn[j].myStatus = 0;
	myTimersOff[j].myStatus = 0;
	}
Serial << F("Switching relays off...\n");
for (j = 0; j < 4; j++)
	{
	digitalWrite(RelayControl[j],HIGH);
	}
Serial << F("Turning status led off...\n");
digitalWrite(ledStatus, LOW);
Serial << F("Really going to sleep, now ...\n");
sleepSW = 1;
Serial << F("Time is ");
printDateTime(now());
}
