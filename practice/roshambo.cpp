#include <iostream>
#include <random>

int numberGenerator(){
  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<int> distro(0,2);
  int randomNumber = distro(generator);
  
  return randomNumber;
}

void game(int playerChoice, int robotChoice) {
  if (playerChoice == 0) {
    std::cout << "Player throws rock.\n";
    if (robotChoice == 0) {
        std::cout << "Robot throws rock.\n";
        std::cout << "Tie game.\n";
    } else if (robotChoice == 1) {
        std::cout << "Robot throws paper.\n";
        std::cout << "Robot wins.\n";
    } else if (robotChoice == 2) {
        std::cout << "Robot throws scissors.\n";
        std::cout << "Player wins.\n";
    }

  } else if (playerChoice == 1) {
    std::cout << "Player throws paper.\n";
    if (robotChoice == 0) {
        std::cout << "Robot throws rock.\n";
        std::cout << "Player wins.\n";
    } else if (robotChoice == 1) {
        std::cout << "Robot throws paper.\n";
        std::cout << "Tie game.\n";
    } else if (robotChoice == 2) {
        std::cout << "Robot throws scissors.\n";
        std::cout << "Robot wins.\n";
    }

  } else if (playerChoice == 2) {
    std::cout << "Player throws scissors.\n";
    if (robotChoice == 0) {
        std::cout << "Robot throws rock.\n";
        std::cout << "Robot wins.\n";
    } else if (robotChoice == 1) {
        std::cout << "Robot throws paper.\n";
        std::cout << "Player wins.\n";
    } else if (robotChoice == 2) {
        std::cout << "Robot throws scissors.\n";
        std::cout << "Tie game.\n";
    }

  } else {
    std::cout << "Error.\n";
  }
}

int main() {
  int playerChoice{};
  std::cout << "Pick a number.\n0: Rock\n1: Paper\n2: Scissors.\n";
  std::cin >> playerChoice;

  int robotChoice{};
  robotChoice = numberGenerator();

  game(playerChoice, robotChoice);
  return 0;
}
