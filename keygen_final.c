#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    int keyLen;

    char *characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    if (argc != 2) {
        printf("Error, invalid number of arguments");
        return 1;
    }
    
    // Get the length of the key
    keyLen = atoi(argv[1]);

    char key[keyLen + 1]; 


    // Initialize random seed with additional randomization
    srand(time(0));


    // While the full length key hasn't been generated yet, randomly output a character between A to Z or 
    // a space if the randomly generated index is 26
    for (int i = 0; i < keyLen; i++) {
        key[i] = characters[rand() % 27];
    }

    key[keyLen] = '\0';

    fprintf(stdout, "%s", key);
    fflush(stdout);
    fprintf(stdout, "\n");
    fflush(stdout);

    return 0;
}
