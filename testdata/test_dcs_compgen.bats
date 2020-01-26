#!/usr/bin/env bats

# bats is a bash test framework.
# see: https://github.com/bats-core/bats-core
# How to run (2 ways):
#   simply execute this script.
#   execute 'bats .' from this dir to run all bats tests

setup() {
  PATH=../cmake-build-debug:$PATH
}

@test "dcs_compgen help" {
  run dcs_compgen -h
  echo $status
  [ "$status" -eq 0 ]
  [ "${lines[0]}" = "Usage: (null) COMMAND [OPTIONS] COMP" ]
}

@test "dcs_compgen no argument" {
  run dcs_compgen
  [ "$status" -eq 255 ]
  [ "$output" = "ERROR: no valid domain found in input: \"\"" ]
}

@test "Find all BL12I-" {
  run dcs_compgen --redirect=redirect BL12I-
  [ "$status" -eq 0 ]
  echo "$output" | diff -B expected_output_dcs_compgen-BL12I.txt -
}

