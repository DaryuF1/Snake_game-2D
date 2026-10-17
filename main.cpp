#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

const int GRID_WIDTH = 30;
const int GRID_HEIGHT = 20;
const int TILE_SIZE = 25;

const int WINDOW_WIDTH = GRID_WIDTH * TILE_SIZE;
const int WINDOW_HEIGHT = (GRID_HEIGHT * TILE_SIZE) + 50; // extra 50 pixels for bottom HUD score panel

enum directii
{
    stop,
    Right,
    Left,
    Down,
    Up
};

directii dir;
bool game_over;
bool running_app = true;
pair<int, int> snake, point;
int score, high_score = 0, ntail;
pair<int, int> tail[200];

// 3x5 Pixel Font matrix for digits 0-9 (custom built-in retro font renderer)
const bool font_3x5[10][5][3] = {
    { {1,1,1}, {1,0,1}, {1,0,1}, {1,0,1}, {1,1,1} }, // 0
    { {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0} }, // 1
    { {1,1,1}, {0,0,1}, {1,1,1}, {1,0,0}, {1,1,1} }, // 2
    { {1,1,1}, {0,0,1}, {1,1,1}, {0,0,1}, {1,1,1} }, // 3
    { {1,0,1}, {1,0,1}, {1,1,1}, {0,0,1}, {0,0,1} }, // 4
    { {1,1,1}, {1,0,0}, {1,1,1}, {0,0,1}, {1,1,1} }, // 5
    { {1,1,1}, {1,0,0}, {1,1,1}, {1,0,1}, {1,1,1} }, // 6
    { {1,1,1}, {0,0,1}, {0,0,1}, {0,0,1}, {0,0,1} }, // 7
    { {1,1,1}, {1,0,1}, {1,1,1}, {1,0,1}, {1,1,1} }, // 8
    { {1,1,1}, {1,0,1}, {1,1,1}, {0,0,1}, {1,1,1} }  // 9
};

// Helper function to render integers as digital pixel numbers on screen
void draw_number(SDL_Renderer* renderer, int num, int x, int y, int pixel_size, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);

    if (num == 0)
    {
        for (int row = 0; row < 5; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if (font_3x5[0][row][col])
                {
                    SDL_Rect p = { x + col * pixel_size, y + row * pixel_size, pixel_size, pixel_size };
                    SDL_RenderFillRect(renderer, &p);
                }
            }
        }
        return;
    }

    int temp = num;
    int digits[10];
    int num_digits = 0;
    while (temp > 0)
    {
        digits[num_digits++] = temp % 10;
        temp /= 10;
    }

    int current_x = x;
    for (int i = num_digits - 1; i >= 0; --i)
    {
        int d = digits[i];
        for (int row = 0; row < 5; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if (font_3x5[d][row][col])
                {
                    SDL_Rect p = { current_x + col * pixel_size, y + row * pixel_size, pixel_size, pixel_size };
                    SDL_RenderFillRect(renderer, &p);
                }
            }
        }
        current_x += (3 + 1) * pixel_size; // spacing between digits
    }
}

// Load high score from local file
void load_high_score()
{
    ifstream file("highscore.txt");
    if (file.is_open())
    {
        file >> high_score;
        file.close();
    }
}

// Save high score to local file
void save_high_score()
{
    ofstream file("highscore.txt");
    if (file.is_open())
    {
        file << high_score;
        file.close();
    }
}

void setup()
{
    game_over = false;

    // start dead center of the grid
    snake.first = GRID_WIDTH / 2;
    snake.second = GRID_HEIGHT / 2;

    // spawn apple safely inside boundaries
    point.first = rand() % (GRID_WIDTH - 2) + 1;
    point.second = rand() % (GRID_HEIGHT - 2) + 1;

    dir = stop;
    ntail = 0;
    score = 0;
}

void input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            running_app = false;
            game_over = true;
        }
        else if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_a: if (dir != Right) dir = Left; break;
                case SDLK_s: if (dir != Up) dir = Down; break;
                case SDLK_d: if (dir != Left) dir = Right; break;
                case SDLK_w: if (dir != Down) dir = Up; break;
                case SDLK_ESCAPE:
                    running_app = false;
                    game_over = true;
                    break;
            }
        }
    }
}

void logic()
{
    // follow-the-leader logic for shifting the snake body parts
    int xprev = snake.first, yprev = snake.second;
    int xprev2, yprev2;
    for (int i = 0; i < ntail; i++)
    {
        xprev2 = tail[i].first;
        yprev2 = tail[i].second;
        tail[i].first = xprev;
        tail[i].second = yprev;
        xprev = xprev2;
        yprev = yprev2;
    }

    // move head based on current direction
    switch (dir)
    {
        case Left:  snake.first--; break;
        case Down:  snake.second++; break;
        case Right: snake.first++; break;
        case Up:    snake.second--; break;
        default: break;
    }

    // hit the walls? game over bro
    if (snake.first < 0 || snake.first >= GRID_WIDTH || snake.second < 0 || snake.second >= GRID_HEIGHT)
        game_over = true;

    // hit your own tail? game over
    for (int i = 0; i < ntail; i++)
    {
        if (snake.first == tail[i].first && snake.second == tail[i].second)
            game_over = true;
    }

    // ate the apple? grow up and respawn it elsewhere
    if (snake.first == point.first && snake.second == point.second)
    {
        score += 10;
        ntail++;
        point.first = rand() % (GRID_WIDTH - 2) + 1;
        point.second = rand() % (GRID_HEIGHT - 2) + 1;

        // update high score instantly if beaten
        if (score > high_score)
        {
            high_score = score;
            save_high_score();
        }
    }
}

void draw(SDL_Renderer* renderer)
{
    // dark background for playfield
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    SDL_Rect rect;
    rect.w = TILE_SIZE - 1;
    rect.h = TILE_SIZE - 1;

    // draw snake tail
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    for (int i = 0; i < ntail; i++)
    {
        rect.x = tail[i].first * TILE_SIZE;
        rect.y = tail[i].second * TILE_SIZE;
        SDL_RenderFillRect(renderer, &rect);
    }

    // draw snake head
    SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
    rect.x = snake.first * TILE_SIZE;
    rect.y = snake.second * TILE_SIZE;
    SDL_RenderFillRect(renderer, &rect);

    // draw apple
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    rect.x = point.first * TILE_SIZE;
    rect.y = point.second * TILE_SIZE;
    SDL_RenderFillRect(renderer, &rect);

    // --- BOTTOM HUD PANEL FOR SCORES ---
    SDL_Rect hud_bg = { 0, GRID_HEIGHT * TILE_SIZE, WINDOW_WIDTH, 50 };
    SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
    SDL_RenderFillRect(renderer, &hud_bg);

    // Draw Score labels / numbers (Green for Score, Gold for High Score)
    // Draw "SCORE" indicator mini blocks or just the number clearly positioned
    draw_number(renderer, score, 30, (GRID_HEIGHT * TILE_SIZE) + 12, 4, 0, 255, 0);
    draw_number(renderer, high_score, WINDOW_WIDTH - 120, (GRID_HEIGHT * TILE_SIZE) + 12, 4, 255, 215, 0);

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout << "Failed to init SDL: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Snake 2D - Portfolio Project",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        cout << "Failed to create window: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        cout << "Failed to create renderer: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    srand(time(NULL)); // seed random once at startup
    load_high_score(); // load previous high score from disk

    // Outer application loop (allows restarting without closing window)
    while (running_app)
    {
        setup();

        // Main game loop
        while (!game_over && running_app)
        {
            input();
            logic();
            draw(renderer);
            SDL_Delay(100);
        }

        // Game Over screen feedback (turns screen dark red)
        if (running_app)
        {
            // Draw a distinct dark red screen for Game Over state
            SDL_SetRenderDrawColor(renderer, 60, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Draw final score on game over screen centered or at the bottom HUD
            draw_number(renderer, score, WINDOW_WIDTH / 2 - 40, WINDOW_HEIGHT / 2 - 20, 6, 255, 255, 255);
            SDL_RenderPresent(renderer);

            bool waiting_for_input = true;
            while (waiting_for_input && running_app)
            {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_QUIT)
                    {
                        running_app = false;
                        waiting_for_input = false;
                    }
                    else if (event.type == SDL_KEYDOWN)
                    {
                        if (event.key.keysym.sym == SDLK_r)
                        {
                            waiting_for_input = false; // restarts outer loop
                        }
                        else if (event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            running_app = false;
                            waiting_for_input = false;
                        }
                    }
                }
                SDL_Delay(50);
            }
        }
    }

    // Clean up resources before exiting
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}