#include "core.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FAKE_SENSOR_VAL 50
#define MOTOR_OFFSET_RIGHT 0
#define MOTOR_OFFSET_LEFT 0

#define POWER_LEVEL_LOW 30
#define POWER_LEVEL_MED 50
#define POWER_LEVEL_HIGH 70
#define SEND_IT 100

#define IR_THRESHOLD_STRAIGHT 50
#define IR_THRESHOLD_HAS_SIGNAL 200
#define MIN_FREE_FORWARD_SPACE 50 // cm

#define MOVE_TIME_SEARCH 1000

#define POWER_LEVEL_APPROACH 20
#define MOVE_TIME_APPROACH 500

enum RobotCorrection_t { MOVE_STRAIGHT = 0, MOVE_RIGHT = 1, MOVE_LEFT = 2 };

// use a global
struct RobotContext_t {
  enum RobotState_t state;
  enum RobotState_t prevState;
};

int randomIntInRange(int upper, int lower) {
  return rand() % (upper + 1 - lower) + lower;
}

void waitMs(int waitTimeMs) { usleep(waitTimeMs * 1000); }

char *stateToString(enum RobotState_t state) {
  switch (state) {
  case IDLE:
    return "IDLE";
  case SEARCHING:
    return "SEARCHING";
  case ALIGNMENT:
    return "ALIGNMENT";
  case DROP_OBJECT:
    return "DROP_OBJECT";
  case COMPLETE:
    return "COMPLETE";
  case COLLISION:
    return "COLLISION";
  default:
    return "STATE OUT OF BOUNDS";
  }
}

void setMotor(int motorNumber, int powerLevel) {
  printf("Setting motor #%d to %d power level\n", motorNumber, powerLevel);
}

int readSensor(char *sensorName) {
  int val = randomIntInRange(500, -500);
  printf("Reading %d from sensor %s\n", val, sensorName);
  // all sensors can return an int in [-500,500]
  return val;
}

int readIrRight() { return readSensor("Photo Transistor Right"); }

int readIrLeft() { return readSensor("Photo Transistor Left"); }

enum RobotCorrection_t readIrDirection() {
  int irRight, irLeft, irDiff;
  bool diffInRange, hasRightSignal, hasLeftSignal;
  irRight = readIrRight();
  irLeft = readIrLeft();
  irDiff = irLeft - irRight;
  diffInRange = irDiff < IR_THRESHOLD_STRAIGHT;
  hasRightSignal = irRight > IR_THRESHOLD_HAS_SIGNAL;
  hasLeftSignal = irLeft > IR_THRESHOLD_HAS_SIGNAL;
  if (diffInRange && hasRightSignal && hasLeftSignal)
    return MOVE_STRAIGHT;
  else if (irDiff < 0) {
    return MOVE_RIGHT;
  } else {
    return MOVE_LEFT;
  }
}

int readForwardDistance() { return readSensor("Sonic Distance Sensor"); }

// TODO
void sonicSensorCorrection() {}

void dropObject() { printf("Dropping object...\n"); }

void controlDriveMotors(int powerLevelLeft, int powerLevelRight) {
  // motor "1" is the left wheel
  setMotor(1, powerLevelLeft + MOTOR_OFFSET_LEFT);
  // motor "2" is the right wheel;
  setMotor(2, powerLevelRight + MOTOR_OFFSET_RIGHT);
}

void driveStraight(int powerLevel, int time) {
  controlDriveMotors(powerLevel, powerLevel);
  waitMs(time);
  controlDriveMotors(0, 0);
}

void turnRight(int powerLevel, int time) {
  controlDriveMotors(powerLevel, -powerLevel);
  waitMs(time);
  controlDriveMotors(0, 0);
}

void turnLeft(int powerLevel, int time) {
  controlDriveMotors(-powerLevel, powerLevel);
  waitMs(time);
  controlDriveMotors(0, 0);
}

// TODO
bool checkForCollisions() { return false; }

void waitForStartButton() {
  char c;
  while (true) {
    c = getchar();
    if (c == 'n') {
      return;
    }
  }
}

void handleIdle(struct RobotContext_t *ctx) {
  // wait for start button
  waitForStartButton();
  // and then go into our "main"
  // searching state
  ctx->state = SEARCHING;
}

bool shouldLeaveSearching() { return true; }

/**
 * How do we want our search to work?
 *
 * Each time we run this,
 * we'll follow roughly the steps below
 *
 * 1. Check how much space is directly ahead of us.
 *    If there is only a small amount (a few cm or less),
 *    we should turn until we have more "search space".
 *    This makes sure we're never pointed at a wall
 *
 * 2. Look at the difference between the readings
 *    on the two phototransistors.
 *
 *    If the diff ~ 0 and signal levels meet a
 *    certain minimum threshold, then we should proceed
 *    straight for a short distance
 *
 *    If the diff > 0, we should turn until
 *    this difference gets closer. If it strats increasing,
 *    back track and turn the other way.
 */
void handleSearching(struct RobotContext_t *ctx) {
  enum RobotCorrection_t move;
  if (readForwardDistance() < MIN_FREE_FORWARD_SPACE) {
    sonicSensorCorrection();
  }
  move = readIrDirection();
  switch (move) {
  case MOVE_STRAIGHT:
    driveStraight(POWER_LEVEL_MED, MOVE_TIME_SEARCH);
    break;
  case MOVE_RIGHT:
    turnRight(POWER_LEVEL_LOW, MOVE_TIME_SEARCH);
    break;
  case MOVE_LEFT:
    turnLeft(POWER_LEVEL_LOW, MOVE_TIME_SEARCH);
    break;
  }
  if (shouldLeaveSearching()) {
    ctx->state = ALIGNMENT;
  }
};

/**
 * This state is reponsible for the approach,
 * so if things get too inaccurate, you kick back
 * to the SEARCHING state
 *
 * TODO
 */
void handleAlignment(struct RobotContext_t *ctx) {
  enum RobotCorrection_t move = readIrDirection();
  // if we aren't still aligned
  // to move straight, kick back
  // to the searching state,
  // and bail hard on this
  if (move != MOVE_STRAIGHT) {
    ctx->state = SEARCHING;
    return;
  }
}

void handleDrop(struct RobotContext_t *ctx) {
  dropObject();
  waitMs(1000);
  driveStraight(-POWER_LEVEL_MED, 2000);
}

void handleComplete(struct RobotContext_t *ctx) { printf("Done!\n"); }

void handleCollision(struct RobotContext_t *ctx) {
  // we must search again before returning
  // to the alignment state after recovering from
  // a collision
  if (ctx->prevState == ALIGNMENT) {
    ctx->state = SEARCHING;
  } else {
    ctx->state = ctx->prevState;
  }
}

void startStateMachine(struct RobotContext_t *ctx) {
  bool inCollisionState;
  while (true) {
    printf("Current state %s\n", stateToString(ctx->state));
    // we assume one of routines called in the switch
    // statement changes ctx->state, so we make a backup
    // of it before any of those routines run
    ctx->prevState = ctx->state;
    inCollisionState = ctx->state == COLLISION;
    if (checkForCollisions() && !inCollisionState) {
      ctx->state = COLLISION;
    }
    switch (ctx->state) {
    case IDLE:
      handleIdle(ctx);
      break;
    case SEARCHING:
      handleSearching(ctx);
      break;
    case ALIGNMENT:
      handleAlignment(ctx);
      break;
    case DROP_OBJECT:
      handleDrop(ctx);
      break;
    case COMPLETE:
      handleComplete(ctx);
      break;
    case COLLISION:
      handleCollision(ctx);
      break;
    }
  }
}
/**
 * Problem description:
 *
 * 1. Robot must locate infrared beacon
 * 2. Get within a certain distance to the infrared beacon
 * 3. Drop object on beacon
 * 4. Signal completion somehow
 *
 * * must avoid walls as best possible
 */
void core() {
  // RobotContext_t refers to the "state of the world"
  // and "context of our robot" in our current enviornment

  // initial state is idle
  struct RobotContext_t ctx = {.state = IDLE};
  struct RobotContext_t *ctxPtr = &ctx;
  startStateMachine(ctxPtr);
}
