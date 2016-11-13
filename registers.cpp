//class for registers

#include "registers.h"

Register::Register (){
}

bool Register::read_valid_bit (){
  return valid;
}

int Register::read_value (){
  return value;
}

void Register::write_valid_bit (bool n){
  valid = n;
  return;
}

void Register::write_value (int n){
  value = n;
  return;
}

void Register::reset (){
  valid = false;
  value = 0;
  return;
}