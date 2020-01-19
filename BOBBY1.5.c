#include "PC_FileIO.c"
#include "mindsensors-motormux.h"

const int SIZE = 4;
const float WIDTH = 3.0;
const int MAX_NOTES = 250;
char notesArray[2][MAX_NOTES];
const int XYLOPHONE_LIMIT = 25;
//const int DRUM_LIMIT = -37;
const int HIT_POWER = 45;


typedef struct
{
	char half_Xylophone[SIZE];
}HalfXylophoneArray;

//Function 0: Filling Array
void fillArray(HalfXylophoneArray & half, char spot0, char spot1, char spot2, char spot3)
{
	half.half_Xylophone[0] = spot0;
	half.half_Xylophone[1] = spot1;
	half.half_Xylophone[2] = spot2;
	half.half_Xylophone[3] = spot3;
}

// Function 1: Read In Notes And Populate Arrays
void populateArray(TFileHandle fin, int timeSignature)
{
	for(int column = 0; column < MAX_NOTES; column ++)
	{
		for(int row = 0; row < 2; row ++)
		{
			notesArray[row][column] = ' ';
		}
	}

	char noteToPlay = ' ';
	for (int column = 0; column < MAX_NOTES && (readCharPC(fin, noteToPlay)); column ++)
	{
		for(int row = 0; row < 2; row ++)
		{
			if(row == 0)
			{
				notesArray[row][column] = noteToPlay;
			}
			else if (timeSignature > 0)
			{
				if((column + 1) % timeSignature == 0)
				{
					notesArray[row][column] = '1';
				}
				else
				{
					notesArray[row][column] = '0';
				}
			}
		}
	}
}

//Function 2: Timer Function
int timeToPlay()
{
	int time = 0;
	bool enterPressed = false;
	while(!enterPressed)
	{
		bool timeRecorded = false;
		while(SensorValue[S1] == 0 && !getButtonPress(buttonEnter))
		{}
		while(SensorValue[S1] == 1 || getButtonPress(buttonEnter))
		{
			if (SensorValue[S1] == 1 && !timeRecorded)
			{
				time += 30000;
				timeRecorded = true;
			}
			else if (getButtonPress(buttonEnter))
				enterPressed = true;
		}

	}
	if (time == 0)
		time = 60000; // default time if enter button pressed right away
	return time;
}

// Function 3: Get Index Function
int getIndex(HalfXylophoneArray xylophone, char note)
{

	for(int index = 0; index < SIZE; index ++)
	{
		if(xylophone.half_Xylophone[index] == note)
		{
			return index;
		}
	}
	return -1;
}

//Function 4: Use New Mallet Position To Get Motor Encoder Counts
int motorEncoderCount(char newKey, HalfXylophoneArray xylophone)
{
	int index = getIndex(xylophone, newKey);
	int distance = 0;
	if (index >= 0)
	{
		distance = (int)((index * WIDTH) * 180 / (PI * 1.5));
	}
	return distance;
}

//Function 5: Choosing A Motor Function
tMotor pickMotor(HalfXylophoneArray left, char note)
{
	if (getIndex(left, note)>=0)
		return motorA;
	else
		return motorB;
}

//Function 6: Moving Xylophone Mallets ? Check if time limit met
void moving_XylophoneMallet(char & currentKey, char newKey, HalfXylophoneArray xylophone, tMotor motorMove, int power, bool & timeReached, int playingTime)
{
	int encoderLimit = motorEncoderCount(newKey, xylophone);
	int index1 = getIndex(xylophone, currentKey);
	int index2 = getIndex(xylophone, newKey);
	if (index1 < index2)
	{
		motor[motorMove] = power;
		while (nMotorEncoder[motorMove] < encoderLimit && time1[T1] < playingTime)
		{}
	}
	else if (index1 > index2)
	{
		motor[motorMove] = -power;
		while (nMotorEncoder[motorMove] > encoderLimit && time1[T1] < playingTime)
		{}
	}

	motor[motorMove] = 0;

	if (time1[T1] > playingTime)
		timeReached  = true;
	else
		currentKey = newKey;
}

//Function 7: Play Mallets (Drum & Xylophone) Check if time limit met
void playDrumAndXylophone(tMotor motorXylophone, /*tMUXmotor motorDrum, bool drumPlay,*/ bool xylophonePlay, bool & timeReached, int playingTime)
{
	nMotorEncoder[motorXylophone] = XYLOPHONE_LIMIT;
	//MSMMotorEncoderReset(motorDrum);
	//MSMMotor(motorDrum, 0);
	motor[motorXylophone] = 0;

	/*if (drumPlay)
	MSMMotor(motorDrum, -HIT_POWER);*/
	if (xylophonePlay)
	{
		motor[motorXylophone] = HIT_POWER;
		nMotorEncoder[motorXylophone] = 0;
	}
	while(time1[T1] < playingTime && (/*(MSMMotorEncoder(motorDrum) > DRUM_LIMIT && drumPlay) ||*/ nMotorEncoder[motorXylophone] < XYLOPHONE_LIMIT))
	{
		/*if (MSMMotorEncoder(motorDrum) <= DRUM_LIMIT)
		{
		MSMMotor(motorDrum, 0);
		displayString(1, "drum down reached");
		}
		if (nMotorEncoder[motorXylophone] >= XYLOPHONE_LIMIT)
		{
		motor[motorXylophone] = 0;
		displayString(2, "xylo down reached");
		}*/
	}
	motor[motorXylophone] = 0;
	//MSMMotor(motorDrum, 0);

	/*if (drumPlay)
	MSMMotor(motorDrum, HIT_POWER);*/
	if (xylophonePlay)
		motor[motorXylophone] = -HIT_POWER;

	while(time1[T1] < playingTime && (/*MSMMotorEncoder(motorDrum) < -5 ||*/ nMotorEncoder[motorXylophone] > 0))
	{
		/*if (MSMMotorEncoder(motorDrum) >= -5)
		{
		MSMMotor(motorDrum, 0);
		displayString(3, "drum up reached");
		}*/
		if (nMotorEncoder[motorXylophone] <= 0)
		{
			motor[motorXylophone] = 0;
			displayString(4, "xylo up reached");
		}
	}
	displayString(5, "exit while loop");

	motor[motorXylophone] = 0;
	//MSMMotor(motorDrum, 0);

	if(time1[T1] >= playingTime)
	{
		timeReached = true;
	}

}

//Function 8: Return to Start Function
void returnToStart(char & currentKeyLeft, char & currentKeyRight, HalfXylophoneArray left, HalfXylophoneArray right)
{
	//char finalLeft = 'F';
	//char finalRight = 'G';
	//int encoderLimitLeft = motorEncoderCount(finalLeft, left);
	//int encoderLimitRight = motorEncoderCount(finalRight, right);

	motor[motorA] = -45;

	while(nMotorEncoder[motorA] > 0)
	{}
	motor[motorA] = 0;
	motor[motorB] = -45;
	while(nMotorEncoder[motorB] > 0)
	{}
	motor[motorB] = 0;

	currentKeyLeft = 'F';
	currentKeyRight = 'G';
}

task main()
{
	// START-UP
	// CONFIGURE SENSORS
	SensorType[S1] = sensorEV3_Touch;
	//SensorType[S2] = sensorI2CCustom;
	//MSMMUXinit();

	// SONG SELECTION
	int buttonPressed = -1;
	//prompt for button press
	displayString(2, "Please press a button");
	displayString(3, "to choose a song");
	displayString(5, "Press right button for");
	displayString(6, "Jingle Bells");
	displayString(8, "Press left button for");
	displayString(9, "Joy to the World");
	displayString(11, "Press up button for");
	displayString(12, "Do You Want to Build a Snowman?");
	//wait for button press
	while(!getButtonPress(buttonAny))
	{}
	while(getButtonPress(buttonAny))
	{
		if(getButtonPress(buttonRight))
		{
			buttonPressed = 0;
		}
		else if(getButtonPress(buttonLeft))
		{
			buttonPressed = 1;
		}
		else if(getButtonPress(buttonUp))
		{
			buttonPressed = 2;
		}
	}
	eraseDisplay();

	TFileHandle fin;
	bool fileOkay;
	if (buttonPressed == 0)
		fileOkay = openReadPC(fin,"jingleBells.txt");
	else if (buttonPressed == 1)
		fileOkay = openReadPC(fin,"joyToTheWorld.txt");
	else
		fileOkay = openReadPC(fin,"snowman2.txt");
	//check for file open
	if(!fileOkay)
	{
		displayString(3, "Error!");
		wait1Msec(5000);
	}

	else
	{
		eraseDisplay();
		displayString(3, "Press touch sensor for");
		displayString(4, "number of 30s intervals");
		displayString(5, "to play for.");
		int timeLength = timeToPlay();
		// READING FROM FILE
		int timeSignature = 0;
		readIntPC(fin, timeSignature);
		populateArray(fin, timeSignature);

		eraseDisplay();

		bool timeReached = false;
		time1[T1] = 0;

		char currMallet_L = 'F', newMallet_L = ' ',	currMallet_R = 'G', newMallet_R = ' ';

		HalfXylophoneArray leftHalf;
		HalfXylophoneArray rightHalf;

		fillArray(leftHalf, 'F', 'E', 'D', 'C');
		fillArray(rightHalf, 'G', 'A', 'B', 'Z');

		nMotorEncoder[motorA] = 0;
		nMotorEncoder[motorB] = 0;
		nMotorEncoder[motorC] = 0;
		nMotorEncoder[motorD] = 0;
		//MSMMotorEncoderReset(mmotor_S2_1);

		while (!timeReached) // shut down
		{
			for (int column = 0; notesArray[0][column] != ' ' && !timeReached && column < MAX_NOTES; column++)
			{
				tMotor motorToMove = pickMotor(leftHalf, notesArray[0][column]);
				//bool drumPlay = false;
				bool xylophonePlay = false;
				/*if (notesArray[1][column] == '1')
				drumPlay = true;*/
				if (notesArray[0][column] != 'R' && notesArray[0][column] != ' ')
					xylophonePlay = true;

				if (motorToMove == motorA)
				{
					if (xylophonePlay)
					{
						newMallet_L = notesArray[0][column];
						// move left half carriage
						moving_XylophoneMallet(currMallet_L, newMallet_L, leftHalf, motorToMove, 60, timeReached, timeLength);
					}
					//play function call
					playDrumAndXylophone(motorD, /*mmotor_S2_1, drumPlay,*/ xylophonePlay, timeReached, timeLength);
					wait1Msec(500);
				}
				else
				{
					if (xylophonePlay)
					{
						newMallet_R = notesArray[0][column];
						// move right half carriage
						moving_XylophoneMallet(currMallet_R, newMallet_R, rightHalf, motorToMove, 60, timeReached, timeLength);
					}
					//play function call
					playDrumAndXylophone(motorC, /*mmotor_S2_1, drumPlay,*/ xylophonePlay, timeReached, timeLength);
					wait1Msec(500);
				}
			}
			returnToStart(currMallet_L, currMallet_R, leftHalf, rightHalf);
		}
	}
	// close file // shut down
	// bring mallets to centre // shut down
}
