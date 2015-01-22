# Stepper-Motor-Elevator-Model
Software written for an Elevator Simulator circuit. 

### Hardware used:  
P24FJ32GA002 MCU  
Stepper Motor  
Piezoelectric Buzzer  
Pushbuttons  
LEDs  
Seven Segment Display  

### Operation of Program: 
This program is designed to simulate an elevator. There are two inputs 
(up and down) and three floors. A stepper motor rotates to pull up a
paper elevator, depending on the input buttons. The interrupt is
designed to simulate a fire alarm within the elevator.

### Hardware Notes:
  There are three indicator LEDs which indicate the floor level, as well
  as a red LED which indicates the fire alarm. A seven segment display
  outputs the floor number. Two pushbutton switches are used for up and
  down buttons and another one is used for the fire alarm. A piezoelectric
  buzzer is used during the fire alarm sequence and to indicate that the
  floor has arrived.
