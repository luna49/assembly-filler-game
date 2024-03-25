#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define BOARD_SIZE 8
#define COLOR_COUNT 6
#define EMPTY 0xFFFF // Assuming white (or any distinct color) represents an empty block in RGB565
#define PLAYER1 1
#define PLAYER2 2
#define SQUARE_SIZE 30 // Size of each square in pixels
#define VGA_PIXEL_BUFFER_BASE_ADDRESS 0x08000000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GRID_WIDTH 200
#define GRID_HEIGHT 200
#define SWITCHES_BASE_ADDRESS 0xFF200040 // Example address
#define KEYS_BASE_ADDRESS 0xFF200050 // Example address

// RGB565 Colors for game
const unsigned short RGB565_COLORS[COLOR_COUNT] = {
    0xF800, // Red
    0x07E0, // Green
    0x001F, // Blue
    0x07FF, // Cyan
    0xF81F, // Magenta
    0xFFE0  // Yellow
};

void initializeBoard(unsigned short board[BOARD_SIZE][BOARD_SIZE], int playerBoard[BOARD_SIZE][BOARD_SIZE]);
int checkAdjacent(unsigned short board[BOARD_SIZE][BOARD_SIZE], int row, int col, unsigned short color);
void printBoard(unsigned short board[BOARD_SIZE][BOARD_SIZE]);
void fill(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], int player, unsigned short color);
void changePlayer(int *currentPlayer);
int countScore(int playerBoard[BOARD_SIZE][BOARD_SIZE], int player);
int isGameOver(int playerBoard[BOARD_SIZE][BOARD_SIZE]);
void vsync();
void plot_pixel(int, int, short int);
void draw_square(int, int, short int);
void printBoardVGA(unsigned short board[BOARD_SIZE][BOARD_SIZE]);

typedef struct {
    int x, y;
} Point;


void enqueue(Point queue[], int *rear, Point item) {
    queue[(*rear)++] = item; // Add item to the queue
}

Point dequeue(Point queue[], int *front) {
    return queue[(*front)++]; // Remove and return item from the queue
}

bool isValid(int x, int y, unsigned short board[BOARD_SIZE][BOARD_SIZE], unsigned short targetColor) {
    return (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && board[x][y] == targetColor);
}

int read_switches() {
    volatile int* switches_ptr = (int*) SWITCHES_BASE_ADDRESS;
    int switch_state = *switches_ptr; // Read the state of the switches

    for (int i = 0; i < COLOR_COUNT; i++) {
        if (switch_state & (1 << i)) { // Check if switch i is on
            return i; // Return the switch number (maps to color index)
        }
    }
    return 0; // Default to 0 if no switch is on
}

int read_key0() {
    volatile int* keys_ptr = (int*) KEYS_BASE_ADDRESS;
    int key_state = *keys_ptr; // Read the state of the keys

    if (key_state & 0x1) { // Check if key 0 is pressed
        return 1;
    } else {
        return 0;
    }
}


int main() {
 	unsigned short board[BOARD_SIZE][BOARD_SIZE]; // Corrected type to unsigned short
    int playerBoard[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer = PLAYER1;
    int gameEnd = 0;
    int selectedColor;

    srand(time(NULL));
    initializeBoard(board, playerBoard);
    printBoardVGA(board);

    bool prevKey0Pressed = false; // Track the previous state of key 0

    while (!gameEnd) {
        bool key0Pressed = read_key0(); // Check the current state of key 0

        // Execute color change on key release (transition from pressed to not pressed)
        if (prevKey0Pressed && !key0Pressed) {
            int switchState = read_switches();
            unsigned short selectedColor = RGB565_COLORS[switchState];

            // Ensure only the current player's corner color changes
            fill(playerBoard, board, currentPlayer, selectedColor);

            printBoardVGA(board); // Update the VGA display with the new board state

            gameEnd = isGameOver(playerBoard);
            if (!gameEnd) {
                changePlayer(&currentPlayer); // Switch players if the game continues
            }
        }

        prevKey0Pressed = key0Pressed; // Update the previous key state for the next iteration

        // Optional: Implement a delay here to manage game pace and debounce handling
    }


    // Determine winner
    int player1Score = countScore(playerBoard, PLAYER1);
    int player2Score = countScore(playerBoard, PLAYER2);
    if (player1Score > player2Score) {
        printf("Player 1 wins!\n");
    } else if (player2Score > player1Score) {
        printf("Player 2 wins!\n");
    } else {
        printf("It's a tie!\n");
    }

    return 0;
}

void initializeBoard(unsigned short board[BOARD_SIZE][BOARD_SIZE], int playerBoard[BOARD_SIZE][BOARD_SIZE]) {
    srand(time(NULL)); // Seed for random color generation, ideally called once in main

    // Calculate the starting x and y coordinates to center the board on the screen
    int startX = (SCREEN_WIDTH - (BOARD_SIZE * SQUARE_SIZE)) / 2;
    int startY = (SCREEN_HEIGHT - (BOARD_SIZE * SQUARE_SIZE)) / 2;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            // Assign a random color from the predefined RGB565 colors
            unsigned short color = RGB565_COLORS[rand() % COLOR_COUNT];
            board[i][j] = color;
            // Initialize playerBoard to track player positions, initially empty
            playerBoard[i][j] = EMPTY;

            // Calculate the top-left pixel coordinates for this square
            int x = startX + j * SQUARE_SIZE;
            int y = startY + i * SQUARE_SIZE;

            // Draw the square on the VGA screen
            draw_square(x, y, color);
        }
    }

    // Optional: Display initial positions or markers for players if applicable
    // This part would involve directly plotting special markers or using a different drawing function
}

// Check if adjacent cells have the same color in RGB565 format.
int checkAdjacent(unsigned short board[BOARD_SIZE][BOARD_SIZE], int row, int col, unsigned short color) {
    int offsets[] = {-1, 1};
    for (int i = 0; i < 2; i++) {
        int newRow = row + offsets[i];
        int newCol = col + offsets[i];
        if (newRow >= 0 && newRow < BOARD_SIZE && board[newRow][col] == color) return 1;
        if (newCol >= 0 && newCol < BOARD_SIZE && board[row][newCol] == color) return 1;
    }
    return 0;
}

// Print the current state of the game board in RGB565 format.
void printBoard(unsigned short board[BOARD_SIZE][BOARD_SIZE]) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            printf("%04X ", board[row][col]);
        }
        printf("\n");
    }
}

// Fill the player's territory with the selected RGB565 color.
void fill(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], int player, unsigned short color) {
    int startX = (player == PLAYER1) ? 0 : BOARD_SIZE - 1;
    int startY = (player == PLAYER1) ? 0 : BOARD_SIZE - 1;

    unsigned short targetColor = board[startX][startY];
    if (targetColor == color) return; // If the target color is the same as the selected color, do nothing.

    Point queue[BOARD_SIZE * BOARD_SIZE]; // Queue for BFS
    int front = 0, rear = 0;

    enqueue(queue, &rear, (Point){startX, startY});
    board[startX][startY] = color; // Change the corner color immediately

    while (front < rear) {
        Point p = dequeue(queue, &front);

        // Check all 4 adjacent squares
        Point directions[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (int i = 0; i < 4; i++) {
            int newX = p.x + directions[i].x;
            int newY = p.y + directions[i].y;

            if (isValid(newX, newY, board, targetColor)) {
                board[newX][newY] = color; // Change the color
                playerBoard[newX][newY] = player; // Assign the square to the player
                enqueue(queue, &rear, (Point){newX, newY});
            }
        }
    }
}


// Change the current player.
void changePlayer(int *currentPlayer) {
    *currentPlayer = (*currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
}

// Count the number of blocks owned by a player.
int countScore(int playerBoard[BOARD_SIZE][BOARD_SIZE], int player) {
    int score = 0;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (playerBoard[row][col] == player) score++;
        }
    }
    return score;
}

// Check if the game is over (no empty blocks remaining).
int isGameOver(int playerBoard[BOARD_SIZE][BOARD_SIZE]) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (playerBoard[row][col] == EMPTY) {
                return 0; // Game is not over since there are still empty blocks.
            }
        }
    }
    return 1; // No empty blocks found, game is over.
}

void plot_pixel(int x, int y, short int color) {
    // Calculate the address of the pixel to be plotted
    volatile short int* pixel_address = (volatile short int*)(VGA_PIXEL_BUFFER_BASE_ADDRESS + (y << 10) + (x << 1));
    *pixel_address = color;
}

void draw_square(int x, int y, short int color) {
    for (int dx = 0; dx < SQUARE_SIZE; dx++) {
        for (int dy = 0; dy < SQUARE_SIZE; dy++) {
            plot_pixel(x + dx, y + dy, color);
        }
    }
}

void printBoardVGA(unsigned short board[BOARD_SIZE][BOARD_SIZE]) {
    // Calculate the starting x and y coordinates to center the board on the screen
    int startX = (SCREEN_WIDTH - (BOARD_SIZE * SQUARE_SIZE)) / 2;
    int startY = (SCREEN_HEIGHT - (BOARD_SIZE * SQUARE_SIZE)) / 2;

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            // Calculate the top-left pixel coordinates for this square
            int x = startX + j * SQUARE_SIZE;
            int y = startY + i * SQUARE_SIZE;

            // Draw the square on the VGA screen with the current color
            draw_square(x, y, board[i][j]);
        }
    }
}


void vsync()
{
	volatile int * pixel_ctrl_ptr = (int *) 0xff203020; // base address
	int status;
	*pixel_ctrl_ptr = 1; // start the synchronization process
	// - write 1 into front buffer address register
	status = *(pixel_ctrl_ptr + 3); // read the status register
	while ((status & 0x01) != 0) // polling loop waiting for S bit to go to 0
	{
		status = *(pixel_ctrl_ptr + 3);
	}
}

