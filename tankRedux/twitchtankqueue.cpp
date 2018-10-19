
#include "pch.h"
#include <iostream>

struct TREDER {
   int maxSpeed;
   int distance;
   int decay;
};

TREDER leftTreder = { 0, 0, 0 };
TREDER rightTreder = { 0, 0, 0 };

enum OPERATOR {
   SET_LEFT_MAXSPEED,
   SET_RIGHT_MAXSPEED,
   SET_LEFT_DECAY,
   SET_RIGHT_DECAY,
   ADD_LEFT_DISTANCE,
   ADD_RIGHT_DISTANCE,
};

struct COMMAND {
   OPERATOR op;
   int value;
};

#define QUEUESIZE 1024

int enqueueIndex, dequeueIndex;

COMMAND commandQueue[QUEUESIZE];

void QueueInit()
{
   enqueueIndex = dequeueIndex = 0;
}

int Enqueue(COMMAND cmd)
{
   int nextEnqueueIndex = enqueueIndex + 1 >= QUEUESIZE ? 0 : enqueueIndex + 1;
   if (nextEnqueueIndex == dequeueIndex)
      return -1; // queue full
   commandQueue[enqueueIndex] = cmd;
   enqueueIndex = nextEnqueueIndex;
   return 0;
}

int Dequeue(COMMAND &cmd)
{
   if (enqueueIndex == dequeueIndex)
      return -1; // queue empty
   cmd = commandQueue[dequeueIndex++];
   dequeueIndex = dequeueIndex >= QUEUESIZE ? 0 : dequeueIndex;
   return 0;
}

void Move(int distance, int leftRightDelta = 0, int maxSpeed = 128) 
{
   int leftMax, rightMax, rightSign = 1, leftSign = 1;
   if (leftRightDelta > 0) {
      rightMax = maxSpeed;
      leftMax = maxSpeed - leftRightDelta;
      if (leftMax < 0) {
         leftSign = -1;
         leftMax = -leftMax;
      }
   } else {
      leftMax = maxSpeed;
      rightMax = maxSpeed - leftRightDelta;
      if (rightMax < 0) {
         rightSign = -1;
         rightMax = -rightMax;
      }
   }
   COMMAND cmd = { SET_LEFT_MAXSPEED, leftMax };
   Enqueue(cmd);
   cmd.op = SET_RIGHT_MAXSPEED;
   cmd.value = rightMax;
   Enqueue(cmd);
   cmd.op = SET_LEFT_DECAY;
   Enqueue(cmd);
   cmd.op = SET_RIGHT_DECAY;
   Enqueue(cmd);
   cmd.op = ADD_LEFT_DISTANCE;
   cmd.value = distance * leftSign;
   Enqueue(cmd);
   cmd.op = ADD_RIGHT_DISTANCE;
   cmd.value = distance * rightSign;
   Enqueue(cmd);
}

void left() {
}

void right() {
}

void attack() {
}

int ProcessCommand(COMMAND cmd) {
   switch (cmd.op) {
      case SET_LEFT_MAXSPEED:
      leftTreder.maxSpeed = cmd.value;
      break;
      case SET_RIGHT_MAXSPEED:
      rightTreder.maxSpeed = cmd.value;
      break;
      case SET_LEFT_DECAY:
      leftTreder.decay = cmd.value;
      break;
      case SET_RIGHT_DECAY:
      rightTreder.decay = cmd.value;
      break;
      case ADD_LEFT_DISTANCE:
      leftTreder.distance += cmd.value;
      break;
      case ADD_RIGHT_DISTANCE:
      rightTreder.distance += cmd.value;
      break;
      default:
      return -1;
   }
   return 0;
}

int TickTreder(TREDER treder)
{
   if (treder.distance) {
      if (treder.distance < 0) {
         treder.distance += treder.decay;
         if (treder.distance > 0)
            treder.distance = 0;
      } else {
         treder.distance -= treder.decay;
         if (treder.distance < 0)
            treder.distance = 0;
      }
   }
   return 0;
}

int UpdateTredSpeed()
{
   if (leftTreder.distance) {
      if (leftTreder.distance > 0) {
         int leftSpeed = leftTreder.maxSpeed;
      } else {
         int leftSpeed = -leftTreder.maxSpeed;
      }
   }
   if (rightTreder.distance) {
      if (rightTreder.distance > 0) {
         int rightSpeed = rightTreder.maxSpeed;
      } else {
         int rightSpeed = -rightTreder.maxSpeed;
      }
   }
   return 0;
}

int loop()
{
   COMMAND cmd;
   while (!Dequeue(cmd)) {
      ProcessCommand(cmd);
   }
   TickTreder(leftTreder);
   TickTreder(rightTreder);
   UpdateTredSpeed();
   return 0;
}


int main()
{
   QueueInit();
//   COMMAND testQueue = { SET_LEFT_MAXSPEED, 6 };
//   Enqueue(testQueue);
//   testQueue.op = ADD_LEFT_DISTANCE;
//   Enqueue(testQueue);
   Move(1 << 15);
   loop();
}
