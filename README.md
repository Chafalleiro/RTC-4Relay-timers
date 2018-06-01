# Alarms
Arduino set of timers and alarms to control a 4 relay shield. Uses also RTC

GNU General Public License to Copyright (c) 2018, Alfonso Abelenda Escudero (chafalladas.com) (Read License file)

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

RTC-4Relay timers v.01. Alfonso Abelenda Escudero 2018
******************************************************************************
 Pins
	 ZS-042: SDA-A4(PC5),SCL-A5(PC4),SQW-pin 2 (PD2)
	 This sketch demonstrates how to use the ZS-042 RTC
	 and a 4 way relay module to swtih the relays in a timely programmed way
	 using the serial port to set the alarm and timer parameters.

	 There are ten alarms, two form the RTC (RTCAlarm) than can be scheduled 
	 in a concrete day or dayly manner. Eight are Arduino programmed alarms
	 that occurs in a 24 hour cycle. Each of the latter are associated
	 with a timer that can be one time triggered or repeated in cycles.

TODO: routine to change EEPROM cell when they reach their writes lifespan, that can be around seven years.
TODO use AT24C32 EEPROM to get advantage of its 1M cicles of rewriting. 32Kb 1M rewrites. Seven decades of use per register writing one every minute.
TODO more actions doable with timers.
TODO: LCD display routines.
TODO: Make something useful with the RTC alarms.
TODO: portable control GUI in Win, Mac and Linux.

PrtTime
ResetAllTheAlarmsNow
Clean
DisplayHolydays
SetTime 2018/05/28 00:00:01
SetOnTimer 1 00:01:02 1 2 - SetOnTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
SetOffTimer 1 00:00:33 1 2 - SetOffTimer [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-repeatable [5]-relay
SetAlarm 1 00:20:23 0 1 -SetAlarm [0]-number [1]-hours:[2]-minutes:[3]-seconds [4]-Random [5]-Timer
ActTimer 1 1 - ActTimer 1 1 [0]-number [1]-boolean
ActTimerOff 1 0 - ActTimerOff 1 1 [0]-number [1]-boolean
ActAlarm 1 1 - ActAlarm [0]-number [1]-boolean
ActRelay 1 1 - ActRelay [0]-number [1]-boolean
DisplayAlarms 0 - DisplayAlarms [0]-alarm number (no argument displays all)
SetWeekdayOff 0 L 1 - SetWeekdayOff [0]-alarm [1]-Weekoff day (L,M,X,J,V,S,D, or M,t,W,T,F,s,S)  [2]-working day 0,1
DisplayWeekdayOff 0 - DisplayWeekdayOff [0]-alarm (no argument displays all)
SetHolyday 0 01/01/18 1 -  SetHolyday [0]-holyday index [0]-day [1]-month [2]-year [3]-yearly (0 do once - 1 yearly - 2 deactivated)

