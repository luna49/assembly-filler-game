// Working filler game in C (does not error check for if player 1 and player 2 choose the same color in 1 round)

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_SIZE 8
#define NUM_COLORS 6

void initializeBoard();
void displayBoard();
int getPlayerColorChoice();
void updatePlayerBoard(int);
void switchPlayer();
bool checkGameOver();
void calculateScores();

int board[BOARD_SIZE][BOARD_SIZE];
int playerBoard[BOARD_SIZE]
               [BOARD_SIZE];  // 0 for none, 1 for player 1, 2 for player 2
int currentPlayer = 1;        // Start with player 1
int player1Score = 0;
int player2Score = 0;

int main() {
  initializeBoard();
  displayBoard();

  while (!checkGameOver()) {
    int colorChoice = getPlayerColorChoice();
    updatePlayerBoard(colorChoice);
    displayBoard();
    calculateScores();  // Calculate scores after updating player board
    switchPlayer();
  }

  printf("Game Over\n");

  // Final scores and winner
  calculateScores();  // Calculate final scores
  int scorePlayer1 = 0;
  int scorePlayer2 = 0;

  // Iterate through player board to count scores
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      if (playerBoard[i][j] == 1) {
        scorePlayer1++;
      } else if (playerBoard[i][j] == 2) {
        scorePlayer2++;
      }
    }
  }

  printf("Final scores:\n");
  printf("Player 1 score: %d\n", scorePlayer1);
  printf("Player 2 score: %d\n", scorePlayer2);
  if (scorePlayer1 > scorePlayer2) {
    printf("Player 1 wins!\n");
  } else if (scorePlayer2 > scorePlayer1) {
    printf("Player 2 wins!\n");
  } else {
    printf("It's a tie!\n");
  }

  return 0;
}

void initializeBoard() {
  srand(time(NULL));  // Seed the random number generator

  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      bool colorOK;
      do {
        // Initializes the colors on the board and ensures that no repeated
        // colors are touching
        colorOK = true;
        board[i][j] = rand() % NUM_COLORS;
        if (i > 0 && board[i][j] == board[i - 1][j])
          colorOK = false;  // Check above
        if (j > 0 && board[i][j] == board[i][j - 1])
          colorOK = false;  // Check left
      } while (!colorOK);
    }
  }

  // Initialize player positions
  playerBoard[BOARD_SIZE - 1][0] = 1;  // Player 1 starts at bottom left
  playerBoard[0][BOARD_SIZE - 1] = 2;  // Player 2 starts at top right
}

void displayBoard() {
  printf("\nCurrent Board:\n");
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      printf("%d ", board[i][j]);
    }
    printf("\t");
    for (int j = 0; j < BOARD_SIZE; j++) {
      printf("%d ", playerBoard[i][j]);
    }
    printf("\n");
  }
}

int getPlayerColorChoice() {
  int choice;
  printf("Player %d, enter your color choice (0-%d): ", currentPlayer,
         NUM_COLORS - 1);
  scanf("%d", &choice);
  return choice;
}

void updatePlayerBoard(int colorChoice) {
  // Iterate through the board
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      // Check if the current tile is adjacent to the player's territory
      if (playerBoard[i][j] == currentPlayer) {
        // Check adjacent tiles
        if ((currentPlayer == 1 &&
             (i > 0 || j < BOARD_SIZE - 1)) ||  // Player 1: Bottom or left
            (currentPlayer == 2 &&
             (i < BOARD_SIZE - 1 || j > 0))) {  // Player 2: Top or right
          // Check adjacent tiles and update if they are of the chosen color
          if (i > 0 && board[i - 1][j] == colorChoice)
            playerBoard[i - 1][j] = currentPlayer;
          if (i < BOARD_SIZE - 1 && board[i + 1][j] == colorChoice)
            playerBoard[i + 1][j] = currentPlayer;
          if (j > 0 && board[i][j - 1] == colorChoice)
            playerBoard[i][j - 1] = currentPlayer;
          if (j < BOARD_SIZE - 1 && board[i][j + 1] == colorChoice)
            playerBoard[i][j + 1] = currentPlayer;
        }
      }
    }
  }
}

void switchPlayer() { currentPlayer = (currentPlayer == 1) ? 2 : 1; }

bool checkGameOver() {
  // Iterate through player board
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      // If there's an empty space, the game is not over
      if (playerBoard[i][j] == 0) {
        return false;
      }
    }
  }
  // If all spaces are filled, the game is over
  return true;
}

void calculateScores() {
  int scorePlayer1 = 0;
  int scorePlayer2 = 0;

  // Iterate through player board to count scores
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      if (playerBoard[i][j] == 1) {
        scorePlayer1++;
      } else if (playerBoard[i][j] == 2) {
        scorePlayer2++;
      }
    }
  }

  // Print scores
  printf("Player 1 score: %d\n", scorePlayer1);
  printf("Player 2 score: %d\n", scorePlayer2);
}
