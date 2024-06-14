#include <iostream>
#include <fstream>
#include <pthread.h>
#include <unordered_set>
#include <string.h>

int board [9][9];

//bitfields and locks
unsigned int row_bits = 0;
pthread_mutex_t row_mutex; //these locks are to prevent a race condition on setting the bits

unsigned int col_bits = 0;
pthread_mutex_t col_mutex;

unsigned int square_bits = 0;
pthread_mutex_t square_mutex;

//Checks in a row is valid
void * check_row(void * arg){
    int row = *(static_cast<int*>(arg));
    //hashset = only unique characters
    //if something is already in the hashset, then we have already seen it in this row
    std::unordered_set<int> seen;
    for (int i = 0; i < 9; i++){
        int val = board[row][i];
        if ((val == 0) || (seen.find(val) != seen.end())){
            pthread_mutex_lock(&row_mutex);
            row_bits |= (1 << row); //set the bit so we could recover the exact row if we want
            pthread_mutex_unlock(&row_mutex);
            break;
        }
        seen.insert(val);
    }
    return NULL;
}

//Check if a column is valid
void * check_col(void * arg){
    int col = *(static_cast<int*>(arg));
    std::unordered_set<int> seen;
    for (int i = 0; i < 9; i++){
        int val = board[i][col];
        if ((val == 0) || (seen.find(val) != seen.end())){
            pthread_mutex_lock(&col_mutex);
            col_bits |= (1 << col); //set the bit so we could recover the exact column if we want
            pthread_mutex_unlock(&col_mutex);
            break;
        }
        seen.insert(val);
    }
    return NULL;
}

//checks if a square is valid
void * check_square(void * arg){
    int square = *(static_cast<int*>(arg));
    std::unordered_set<int> seen;
    //just a way for me to find which square i want this thread to look at
    int xorg = square%3;
    int yorg = int(square/3);
    for (int y = 0; y < 2; y++){
        for (int x = 0; x < 2; x++){
            int val = board[yorg+y][xorg+x];
            if ((val == 0) || (seen.find(val) != seen.end())){
                pthread_mutex_lock(&square_mutex);
                square_bits |= (1 << square); //set the bit so we could recover the exact square if we want
                pthread_mutex_unlock(&square_mutex);
                break;
            }
            seen.insert(val);
        }
    }
    return NULL;
}

int main(int argc, char **argv){
    bool silent = true;
    if (argc != 1){
        if (strncmp(argv[1], "-v", 3) == 0){
            silent = false;
        }
    }
    std::ifstream input("board.txt");
    if (input.fail()){
        if (silent == false){
            std::cout << "File not found\n";
        }
        return 1;
    }
    int * p = &board[0][0];
    int number_of_rows = 0;
    std::string number;

    //getting input
    while (input >> number){
        if (number_of_rows >= 9){
            break;
        }
        if (number.length() != 9){
            if (silent == false){
                std::cout << "Invalid board dimensions\n";
            }
            return 1;
        }
        std::string::const_iterator it = number.begin();
        while (it != number.end()){
            if (!std::isdigit(*it)){
                if (silent == false){
                    std::cout << "Board contains non-digit characters\n";
                }
                return 1;
            }
            //0 is asacii code 48, so since we get ascii codes we need to subtract 48 to get integers
            *p = static_cast<int>(*it) - 48;
            p++;
            it++;
        }
        number_of_rows++;
    }
    if (number_of_rows != 9){
        if (silent == false){
            std::cout << "Invalid board dimensions\n";
        }
        return 1;
    }

    if (silent == false) {
        std::cout << "The board: \n";
        for (int y = 0; y < 9; y++){
            for (int x = 0; x < 9; x++){
                std::cout << board[y][x];
                if (x%3 == 2 && x != 8){
                    std::cout << "|";
                }
            }
            std::cout << std::endl;
            if (y%3 == 2 && y != 8){
                std::cout << "-----------\n";
            }
        }
        std::cout << std::endl;
    }
    
    //threads and locks
    pthread_t row_threads[9];
    int row_args[9];
    pthread_mutex_init(&row_mutex, NULL);

    pthread_t col_threads[9];
    int col_args[9];
    pthread_mutex_init(&col_mutex, NULL);

    pthread_t square_threads[9];
    int square_args[9];
    pthread_mutex_init(&square_mutex, NULL);

    //thread creation
    for (int i = 0; i < 9; i++){
        row_args[i] = i;
        col_args[i] = i;
        square_args[i] = i;
        if (pthread_create(&row_threads[i], NULL, check_row, &row_args[i]) != 0){
            if (silent == false){
                std::cout << "Something went wrong checking the rows\n";
            }
            return 1;
        }
        if (pthread_create(&col_threads[i], NULL, check_col, &col_args[i]) != 0){
            if (silent == false){
                std::cout << "Something went wrong checking the columns\n";
            }
            return 1;
        }
        if (pthread_create(&square_threads[i], NULL, check_square, &square_args[i]) != 0){
            if (silent == false){
                std::cout << "Something went wrong checking the squares\n";
            }
            return 1;
        }
    }

    //wait for everything to finish
    for (int i = 0; i < 9; i++){
        pthread_join(row_threads[i], 0);
        pthread_join(col_threads[i], 0);
        pthread_join(square_threads[i], 0);
    }
    
    //some debug stuff
    // std::cout << "row bits: " << row_bits << "\n";
    // std::cout << "col bits: " << col_bits << "\n";
    // std::cout << "square bits: " << square_bits << "\n";

    //thread destruction
    pthread_mutex_destroy(&row_mutex);
    pthread_mutex_destroy(&col_mutex);
    pthread_mutex_destroy(&square_mutex);

    //each of the bitsets are initalized to 0, so if there are any changes result will not be 0.
    //it will be 0 otherwise
    int result = row_bits | col_bits | square_bits;

    if (result != 0){
        if (silent == false){
            std::cout << "This is not a valid sudoku board\n";
        }
        return 1;
    }
    else{
        if (silent == false){
            std::cout << "This is a valid sudoku board\n";
        }
        return 0;
    }
}