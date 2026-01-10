#include <iostream>

int main (){
  for (int x = 1; x <= 10; x++){
    if (x % 15 == 0){
      std::cout << "Fizz buzz" << std::endl;
    }
    else if (x % 3 == 0){
      std::cout << "Fizz" << std::endl;
    }
    else if (x % 5 == 0){
      std::cout << "Buzz" << std::endl;
    }
    else{
      std::cout << x << std::endl;
    }
  }
    return 0;
}
