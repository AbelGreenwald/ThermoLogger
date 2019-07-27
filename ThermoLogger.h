float getTemperature(void);
void setupTimer1(void);
void setupRtc(void);
void setupTime(void);
void setupPins(void);
void setupSerial(void);
void isr_printRecords(void);
void isr_flushRecords(void);

//Reset Function is a pointer to address 0x0
void(* resetFunc) (void) = 0x0;