#ifndef AMERICANO_MAIN_H
#define AMERICANO_MAIN_H

enum RobotState_t {
  // right at the start before
  // we do anything
  IDLE = 0,
  // This is the state we'll likely spend
  // the most time in (looking for the beacon)
  SEARCHING = 1,
  // When we're near the beacon
  // but not quite aligned for a drop yet
  ALIGNMENT = 2,
  // In position to drop the object
  DROP_OBJECT = 3,
  // signal that we're done somehow
  // and stop moving
  COMPLETE = 4,
  // special state that we use to recover
  // from collisions (can be thought of
  // like an interrupt state)
  COLLISION = 5
};

char *stateToString(enum RobotState_t state);

void core();

#endif
