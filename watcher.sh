#!/usr/bin/env bash

build() {
  make
}

build
while sleep 5; do build; done
