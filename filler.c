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
#define SEG7_DISPLAY 0xFF200020 // Base address for the 7-segment display
#define SEGMENT_OFFSET 8 // Offset for each hex digit within the 32-bit word
#define LEDS_BASE_ADDRESS 0xFF200000 
#define AUDIO_BASE_ADDRESS 0xFF203040
#define PS2_BASE_ADDRESS 0xFF200100
	
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
int calculateScore(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], int player);
int isGameOver(int playerBoard[BOARD_SIZE][BOARD_SIZE]);
void dfsCount(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], bool visited[BOARD_SIZE][BOARD_SIZE], int x, int y, unsigned short color, int* score); 
void vsync();
void plot_pixel(int, int, short int);
void draw_square(int, int, short int);
void printBoardVGA(unsigned short board[BOARD_SIZE][BOARD_SIZE]);
void display_score(int score, volatile unsigned int* seg7_display, int player);
int calculateScore(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], int player);
void update_leds(volatile unsigned int* leds, int currentPlayer);

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

int read_keyBoard() {
	int PS2_data, RVALID;

	volatile int* PS2_ptr = (int*) PS2_BASE_ADDRESS;
	PS2_data = *(PS2_ptr);	// read the Data register in the PS/2 port
	RVALID = (PS2_data & 0x8000);	// extract the RVALID field

	if(RVALID != 0 && (PS2_data & 0xFF) == 0x29){
		//spacebar is pressed
		return 1;
	}
	return 0;
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

    bool prevSpacePressed = false; // Track the previous state of key 0

    while (!gameEnd) {
        bool spacePressed = read_keyBoard(); // Check the current state of spacebar

        // Execute color change on key release (transition from pressed to not pressed)
        if (prevSpacePressed && !spacePressed) {
            int switchState = read_switches();
            unsigned short selectedColor = RGB565_COLORS[switchState];

            // Ensure only the current player's corner color changes
            fill(playerBoard, board, currentPlayer, selectedColor);
					
            printBoardVGA(board); // Update the VGA display with the new board state
			
 			int scorePlayer1 = calculateScore(playerBoard, board, PLAYER1);
            int scorePlayer2 = calculateScore(playerBoard, board, PLAYER2);
			
			printf("Player 1's score: %d\n", scorePlayer1);
			printf("Player 2's score: %d\n", scorePlayer2);
				   
   		 	// Display scores on 7-segment displays
    		// Assuming 'display_score' can target individual segments, and we have a mechanism to specify which
			display_score(scorePlayer1, (volatile unsigned int*)SEG7_DISPLAY, PLAYER1);
            display_score(scorePlayer2, (volatile unsigned int*)SEG7_DISPLAY, PLAYER2);
			
			update_leds((volatile unsigned int*)LEDS_BASE_ADDRESS, currentPlayer);
			
			gameEnd = isGameOver(playerBoard);
            if (!gameEnd) {
                changePlayer(&currentPlayer); // Switch players if the game continues
            }
        }

        prevSpacePressed = spacePressed; // Update the previous key state for the next iteration

        // Optional: Implement a delay here to manage game pace and debounce handling
    }


    // Determine winner
    // int player1Score = countScore(playerBoard, PLAYER1);
    // int player2Score = countScore(playerBoard, PLAYER2);
    //if (player1Score > player2Score) {
        //printf("Player 1 wins!\n");
    //} else if (player2Score > player1Score) {
        //printf("Player 2 wins!\n");
    //} else {
        //printf("It's a tie!\n");
    //}

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
			bool colorOK;
			
			do {
        	// Initializes the colors on the board and ensures that no repeated
        	// colors are touching
        	colorOK = true;
            unsigned short color = RGB565_COLORS[rand() % COLOR_COUNT];
				 
            board[i][j] = color;
            // Initialize playerBoard to track player positions, initially empty
			if (i > 0 && board[i][j] == board[i - 1][j])
          		colorOK = false;  // Check above
        	if (j > 0 && board[i][j] == board[i][j - 1])
          		colorOK = false;  // Check left
			// Calculate the top-left pixel coordinates for this square
            int x = startX + j * SQUARE_SIZE;
            int y = startY + i * SQUARE_SIZE;

            // Draw the square on the VGA screen
            draw_square(x, y, color);
				
      		} while (!colorOK);
			
            playerBoard[i][j] = EMPTY;


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

void display_score(int score, volatile unsigned int* seg7_display, int player) {
    // Encode digits for a common cathode 7-segment display
    unsigned int digits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

    int ones = score % 10; // Ones digit
    int tens = (score / 10) % 10; // Tens digit

    // Calculate offsets based on player number
    int ones_offset = (player == PLAYER1) ? 2 : 0; // Adjust for Player 1 or Player 2
    int tens_offset = (player == PLAYER1) ? 3 : 1; // Adjust for Player 1 or Player 2

    // Clear the segments for the current player's score
    *seg7_display &= ~((0x7F << (ones_offset * SEGMENT_OFFSET)) | (0x7F << (tens_offset * SEGMENT_OFFSET)));

    // Set the new score on the 7-segment display
    *seg7_display |= (digits[ones] << (ones_offset * SEGMENT_OFFSET)); // Set ones digit
    *seg7_display |= (digits[tens] << (tens_offset * SEGMENT_OFFSET)); // Set tens digit
}

void dfsCount(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], bool visited[BOARD_SIZE][BOARD_SIZE], int x, int y, unsigned short color, int* score) {
    // Check bounds and whether the block has been visited or not
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || visited[x][y] || board[x][y] != color) {
        return;
    }

    visited[x][y] = true; // Mark the block as visited
    (*score)++; // Increment the score for each block of the same color

    // Explore adjacent blocks in all four directions
    dfsCount(playerBoard, board, visited, x + 1, y, color, score);
    dfsCount(playerBoard, board, visited, x - 1, y, color, score);
    dfsCount(playerBoard, board, visited, x, y + 1, color, score);
    dfsCount(playerBoard, board, visited, x, y - 1, color, score);
}

int calculateScore(int playerBoard[BOARD_SIZE][BOARD_SIZE], unsigned short board[BOARD_SIZE][BOARD_SIZE], int player) {
    int score = 0;
    bool visited[BOARD_SIZE][BOARD_SIZE] = {{false}};
    unsigned short color;

    // Determine the starting point based on the player
    int startX = (player == PLAYER1) ? 0 : BOARD_SIZE - 1;
    int startY = (player == PLAYER1) ? 0 : BOARD_SIZE - 1;
    color = board[startX][startY];

    // Perform DFS from the corner to count contiguous blocks of the same color
    dfsCount(playerBoard, board, visited, startX, startY, color, &score);
    return score;
}

void update_leds(volatile unsigned int* leds, int currentPlayer) {
    if (currentPlayer == PLAYER1) {
        *leds = 0x01; // Turn on the first LED for Player 1
    } else {
        *leds = 0x02; // Turn on the second LED for Player 2
    }
}

