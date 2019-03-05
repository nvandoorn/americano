#!/usr/bin/env bash

buildRun() {
  make && ./main
}


test
while sleep 5; do buildRun; done
