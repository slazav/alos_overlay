#include <iostream>
#include "err/err.h"
#include "err/assert_err.h"

#include "proc.h"

int
main(int argc, char *argv[]){
  try{
    assert_eq(parse_key("N56E117"), iPoint(117,56));
    assert_eq(parse_key("N01W100"), iPoint(-100,1));
    assert_eq(parse_key("S021W10"), iPoint(-10,-21));
  }
  catch (Err & e) {
    if (e.str()!="")
      std::cerr << "Error: " << e.str() << "\n";
    return 1;
  }
  return 0;
}

