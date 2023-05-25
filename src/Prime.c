#include <stdio.h>
#include <math.h>

int isPrime(unsigned int number) {
    if (number < 2) {
        return 0; 
    }
    
    unsigned int sqrtNumber = sqrt(number);
    for (unsigned int i = 2; i <= sqrtNumber; i++) {
        if (number % i == 0) {
            return 0; 
        }
    }
    
    return 1;  
}
