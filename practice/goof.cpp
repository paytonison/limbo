#include <iostream>
#include <string>

void pearl(std::string <jam>& jam) {
    std::cout << "Enter a string: ";
    std::getline(std::cin, jam);
    std::cout << "You entered: " << jam << std::endl;
}

int main() {
    std::string jam = "";
    pearl(jam);
    return 0;
}
