/*
 * Maze Game
 * Copyright (C) 2026  0xadamastor
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

#define WALL_CHAR "█"
#define PATH_CHAR " "
#define VISITED_CHAR "."
#define GOAL_CHAR "$"
#define PORTAL_CHAR "◉"
#define FOG_CHAR "▒"

#define WALL 1
#define PATH 0
#define VISITED 2
#define BORDER 3
#define PORTAL_A 4
#define PORTAL_B 5
#define FOG 8

#define MAX_NAME_LEN 20
#define LEADERBOARD_FILE "maze_scores.txt"
#define COLOR_SETTINGS_FILE "maze_colors.txt"
#define MAX_SCORES 10
#define MAX_PORTALS 4
#define FOG_RADIUS 4


#define MODE_CLASSIC 0
#define MODE_PORTALS 1
#define MODE_FOG_OF_WAR 2
#define MODE_DARK_LABYRINTH 3
#define MODE_NO_RETURN 4
#define MODE_MIRROR 5
#define MODE_CUSTOM 6

#define ANSI_RESET "\033[0m"
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"
#define ANSI_BRIGHT_BLACK "\033[90m"
#define ANSI_BRIGHT_RED "\033[91m"
#define ANSI_BRIGHT_GREEN "\033[92m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_BLUE "\033[94m"
#define ANSI_BRIGHT_MAGENTA "\033[95m"
#define ANSI_BRIGHT_CYAN "\033[96m"
#define ANSI_BRIGHT_WHITE "\033[97m"
#define ANSI_BOLD "\033[1m"
#define ANSI_CLEAR "\033[2J"
#define ANSI_HOME "\033[H"


#define MIN_MAZE_SIZE 7
#define DEFAULT_EASY_WIDTH 21
#define DEFAULT_EASY_HEIGHT 15
#define DEFAULT_MEDIUM_WIDTH 31
#define DEFAULT_MEDIUM_HEIGHT 21
#define DEFAULT_HARD_WIDTH 41
#define DEFAULT_HARD_HEIGHT 31
#define MAX_MAZE_WIDTH 99
#define MAX_MAZE_HEIGHT 49
#define MIN_CUSTOM_SIZE 11
#define MAX_STACK_SIZE 10000


typedef struct {
    int x;
    int y;
} StackNode;

typedef struct {
    StackNode items[MAX_STACK_SIZE];
    int top;
} Stack;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point entrance;
    Point exit;
    int active;
} Portal;

typedef struct {
    int width;
    int height;
    int** grid;
    Portal portals[MAX_PORTALS];
    int portal_count;
    int game_mode;
    int** visible;
    int goal_revealed;
    int custom_modifiers;
} Maze;

typedef struct {
    char icon;
    Point pos;
    int mazes_completed;
    int steps;
} Player;

typedef struct {
    char name[MAX_NAME_LEN];
    int mazes_completed;
    int difficulty;
    int game_mode;
    time_t timestamp;
} Score;

typedef struct {
    const char* wall_color;
    const char* visited_color;
    const char* player_color;
    const char* goal_color;
    const char* info_color;
    const char* border_color;
    const char* portal_color;
    const char* fog_color;
    char wall_custom[20];
    char visited_custom[20];
    char player_custom[20];
    char goal_custom[20];
    char info_custom[20];
    char border_custom[20];
} ColorScheme;

ColorScheme g_colors = {
    ANSI_BLUE,          
    ANSI_GREEN,         
    ANSI_YELLOW,        
    ANSI_RED,           
    ANSI_CYAN,          
    "",                 
    ANSI_MAGENTA,       
    ANSI_BRIGHT_BLACK,  
    "",                 
    "",                 
    "",                 
    "",                 
    "",                 
    ""                  
};

void init_terminal();
void restore_terminal();
void clear_screen();
void move_cursor(int row, int col);
void hide_cursor();
void show_cursor();
int get_key();
void flush_input();

int show_menu(char* player_icon, int* difficulty, int* game_mode);
int show_mode_selection_menu();
void show_leaderboard();
void color_customization_menu();
void custom_difficulty_menu(int* width, int* height);
void save_score(const char* name, int mazes, int difficulty, int game_mode);
void load_scores(Score scores[], int* count);
void save_color_settings();
void load_color_settings();
int color_index_from_code(const char* code);
Maze create_maze(int difficulty, int custom_width, int custom_height, int game_mode);
void free_maze(Maze* maze);
void generate_maze_dfs(Maze* maze, int x, int y);
void place_portals(Maze* maze);
void init_fog_of_war(Maze* maze);
void update_fog_of_war(Maze* maze, Player* player);
void draw_maze(Maze* maze, Player* player, Point* goal);
int play_game(Maze* maze, Player* player);
void show_win_screen(Player* player);
int is_valid_move(Maze* maze, int x, int y, Player* player);
void get_player_name(char* name, int mazes, int difficulty, int game_mode);
const char* get_color_by_index(int index);
void hex_to_ansi(const char* hex, char* ansi_buf);
Point find_farthest_point(Maze* maze, int start_x, int start_y);

#ifdef _WIN32
static HANDLE hConsole;
static DWORD original_mode;

void init_terminal() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hConsole, &original_mode);
    DWORD mode = original_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, mode);
    SetConsoleOutputCP(CP_UTF8);
}

void restore_terminal() {
    SetConsoleMode(hConsole, original_mode);
}

int get_key() {
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            ch = _getch();
            switch(ch) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
            }
        }
        if (ch == 13) return '\n';
        return ch;
    }
    return -1;
}

void flush_input() {
    while (_kbhit()) _getch();
}

#else
static struct termios original_termios;

void init_terminal() {
    tcgetattr(STDIN_FILENO, &original_termios);
    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

int get_key() {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1) {
                if (seq[0] == '[') {
                    if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                        switch(seq[1]) {
                            case 'A': return 'w';
                            case 'B': return 's';
                            case 'D': return 'a';
                            case 'C': return 'd';
                        }
                    }
                }
            }
            return 27;
        }
        if (ch == 10 || ch == 13) return '\n';
        return ch;
    }
    return -1;
}

void flush_input() {
    tcflush(STDIN_FILENO, TCIFLUSH);
}
#endif

void clear_screen() {
    printf(ANSI_CLEAR ANSI_HOME);
    fflush(stdout);
}

void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
}

void hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

void show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

const char* get_color_by_index(int index) {
    switch(index) {
        case 0: return ANSI_BLACK;
        case 1: return ANSI_RED;
        case 2: return ANSI_GREEN;
        case 3: return ANSI_YELLOW;
        case 4: return ANSI_BLUE;
        case 5: return ANSI_MAGENTA;
        case 6: return ANSI_CYAN;
        case 7: return ANSI_WHITE;
        case 8: return ANSI_BRIGHT_BLACK;
        case 9: return ANSI_BRIGHT_RED;
        case 10: return ANSI_BRIGHT_GREEN;
        case 11: return ANSI_BRIGHT_YELLOW;
        case 12: return ANSI_BRIGHT_BLUE;
        case 13: return ANSI_BRIGHT_MAGENTA;
        case 14: return ANSI_BRIGHT_CYAN;
        case 15: return ANSI_BRIGHT_WHITE;
        default: return ANSI_WHITE;
    }
}

int color_index_from_code(const char* code) {
    if (strcmp(code, ANSI_BLACK) == 0) return 0;
    if (strcmp(code, ANSI_RED) == 0) return 1;
    if (strcmp(code, ANSI_GREEN) == 0) return 2;
    if (strcmp(code, ANSI_YELLOW) == 0) return 3;
    if (strcmp(code, ANSI_BLUE) == 0) return 4;
    if (strcmp(code, ANSI_MAGENTA) == 0) return 5;
    if (strcmp(code, ANSI_CYAN) == 0) return 6;
    if (strcmp(code, ANSI_WHITE) == 0) return 7;
    if (strcmp(code, ANSI_BRIGHT_BLACK) == 0) return 8;
    if (strcmp(code, ANSI_BRIGHT_RED) == 0) return 9;
    if (strcmp(code, ANSI_BRIGHT_GREEN) == 0) return 10;
    if (strcmp(code, ANSI_BRIGHT_YELLOW) == 0) return 11;
    if (strcmp(code, ANSI_BRIGHT_BLUE) == 0) return 12;
    if (strcmp(code, ANSI_BRIGHT_MAGENTA) == 0) return 13;
    if (strcmp(code, ANSI_BRIGHT_CYAN) == 0) return 14;
    if (strcmp(code, ANSI_BRIGHT_WHITE) == 0) return 15;
    return -1;
}

// Converts hex color (#RRGGBB) to ANSI 24-bit true color escape sequence
void hex_to_ansi(const char* hex, char* ansi_buf) {
    if (hex[0] == '#') hex++;
    
    unsigned int r, g, b;
    if (strlen(hex) == 6) {
        sscanf(hex, "%02x%02x%02x", &r, &g, &b);
        sprintf(ansi_buf, "\033[38;2;%u;%u;%um", r, g, b);
    } else {
        strcpy(ansi_buf, ANSI_WHITE);
    }
}

void save_color_settings() {
    FILE* file = fopen(COLOR_SETTINGS_FILE, "w");
    if (file == NULL) return;
    
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.wall_color), 
            strlen(g_colors.wall_custom) > 0 ? g_colors.wall_custom : "-");
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.visited_color), 
            strlen(g_colors.visited_custom) > 0 ? g_colors.visited_custom : "-");
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.player_color), 
            strlen(g_colors.player_custom) > 0 ? g_colors.player_custom : "-");
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.goal_color), 
            strlen(g_colors.goal_custom) > 0 ? g_colors.goal_custom : "-");
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.info_color), 
            strlen(g_colors.info_custom) > 0 ? g_colors.info_custom : "-");
    fprintf(file, "%d %s\n", color_index_from_code(g_colors.border_color), 
            strlen(g_colors.border_custom) > 0 ? g_colors.border_custom : "-");
    
    fclose(file);
}


void set_color(const char** color_ptr, char* custom_ptr, int index, const char* custom) {
    if (index >= 0) {
        *color_ptr = get_color_by_index(index);
        strcpy(custom_ptr, "");
    } else {
        strcpy(custom_ptr, custom);
        *color_ptr = custom_ptr;
    }
}

void load_color_settings() {
    FILE* file = fopen(COLOR_SETTINGS_FILE, "r");
    if (file == NULL) return;
    
    int indices[6];
    char custom_colors[6][20];
    
    
    for (int i = 0; i < 6; i++) {
        if (fscanf(file, "%d %19s\n", &indices[i], custom_colors[i]) != 2) {
            fclose(file);
            return;
        }
        if (strcmp(custom_colors[i], "-") == 0) {
            strcpy(custom_colors[i], "");
        }
    }
    
    fclose(file);
    
    
    set_color(&g_colors.wall_color, g_colors.wall_custom, indices[0], custom_colors[0]);
    set_color(&g_colors.visited_color, g_colors.visited_custom, indices[1], custom_colors[1]);
    set_color(&g_colors.player_color, g_colors.player_custom, indices[2], custom_colors[2]);
    set_color(&g_colors.goal_color, g_colors.goal_custom, indices[3], custom_colors[3]);
    set_color(&g_colors.info_color, g_colors.info_custom, indices[4], custom_colors[4]);
    set_color(&g_colors.border_color, g_colors.border_custom, indices[5], custom_colors[5]);
}

void apply_color_preset(int preset) {
    switch(preset) {
        case 0: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 4, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 2, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 3, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 1, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 6, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, -1, "");
            break;
        case 1: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 0, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 10, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 15, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 2, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 10, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 2, ""); 
            break;
        case 2: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 8, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 13, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 14, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 11, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 13, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 12, ""); 
            break;
        case 3: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 0, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 3, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 15, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 11, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 9, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 1, ""); 
            break;
        case 4: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 4, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 6, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 15, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 14, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 12, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 4, ""); 
            break;
        case 5: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 8, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 2, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 11, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 10, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 2, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 2, ""); 
            break;
        case 6: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 5, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 6, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 3, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 13, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 14, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 5, ""); 
            break;
        case 7: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 0, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 4, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 7, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 11, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 12, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 8, ""); 
            break;
        case 8: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 6, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 14, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 14, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 7, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 14, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 6, ""); 
            break;
        case 9: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 0, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 3, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 11, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 1, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 11, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 8, ""); 
            break;
        case 10: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 1, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 5, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 9, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 7, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 13, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 5, ""); 
            break;
        case 11: 
            set_color(&g_colors.wall_color, g_colors.wall_custom, 13, ""); 
            set_color(&g_colors.visited_color, g_colors.visited_custom, 14, ""); 
            set_color(&g_colors.player_color, g_colors.player_custom, 11, ""); 
            set_color(&g_colors.goal_color, g_colors.goal_custom, 15, ""); 
            set_color(&g_colors.info_color, g_colors.info_custom, 14, ""); 
            set_color(&g_colors.border_color, g_colors.border_custom, 6, ""); 
            break;
    }
    save_color_settings();
}

void show_preset_menu() {
    const char* preset_names[] = {
        "Default", "Matrix", "Neon", "Fire", "Ocean", "Forest", "Retro", "Midnight",
        "Hatsune Miku", "Kagamine Neru", "Kasane Teto", "Triple Baka"
    };
    
    int selected = 0;
    int done = 0;
    
    while (!done) {
        clear_screen();
        move_cursor(1, 2);
        printf(ANSI_BRIGHT_CYAN ANSI_BOLD "=== COLOR PRESETS ===" ANSI_RESET);
        
        for (int i = 0; i < 12; i++) {
            move_cursor(3 + i, 2);
            printf("%s%s", selected == i ? "→ " : "  ", preset_names[i]);
        }
        
        move_cursor(16, 2);
        printf("%sBack", selected == 12 ? "→ " : "  ");
        
        move_cursor(18, 2);
        printf("W/S or ↑/↓ to navigate | Enter to apply | 0 to cancel");
        fflush(stdout);
        
        flush_input();
        int ch;
        while ((ch = get_key()) == -1);
        
        if (ch == 'w' || ch == 'W') {
            selected = (selected - 1 + 13) % 13;
        } else if (ch == 's' || ch == 'S') {
            selected = (selected + 1) % 13;
        } else if (ch == '0') {
            done = 1;
        } else if (ch == '\r' || ch == '\n' || ch == 10) {
            if (selected == 12) {
                done = 1;
            } else {
                apply_color_preset(selected);
                
                clear_screen();
                move_cursor(10, 2);
                printf("%sPreset applied successfully!%s", ANSI_BRIGHT_GREEN, ANSI_RESET);
                move_cursor(12, 2);
                printf("Press any key to continue...");
                fflush(stdout);
                
                flush_input();
                while (get_key() == -1);
                done = 1;
            }
        }
    }
}

void color_customization_menu() {
    const char* color_names[] = {
        "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
        "Bright Black", "Bright Red", "Bright Green", "Bright Yellow",
        "Bright Blue", "Bright Magenta", "Bright Cyan", "Bright White"
    };
    
    int current_option = 0;
    int done = 0;
    
    while (!done) {
        clear_screen();
        move_cursor(1, 2);
        printf(ANSI_BRIGHT_CYAN ANSI_BOLD "=== COLOR CUSTOMIZATION ===" ANSI_RESET);
        
        move_cursor(3, 2);
        printf("%sWall Color (currently: %sEXAMPLE%s)",
               current_option == 0 ? "→ " : "  ",
               g_colors.wall_color, ANSI_RESET);
        
        move_cursor(4, 2);
        printf("%sVisited Path Color (currently: %sEXAMPLE%s)",
               current_option == 1 ? "→ " : "  ",
               g_colors.visited_color, ANSI_RESET);
        
        move_cursor(5, 2);
        printf("%sPlayer Color (currently: %sEXAMPLE%s)",
               current_option == 2 ? "→ " : "  ",
               g_colors.player_color, ANSI_RESET);
        
        move_cursor(6, 2);
        printf("%sGoal Color (currently: %sEXAMPLE%s)",
               current_option == 3 ? "→ " : "  ",
               g_colors.goal_color, ANSI_RESET);
        
        move_cursor(7, 2);
        printf("%sInfo Text Color (currently: %sEXAMPLE%s)",
               current_option == 4 ? "→ " : "  ",
               g_colors.info_color, ANSI_RESET);
        
        move_cursor(8, 2);
        printf("%sBorder Color (currently: %sEXAMPLE%s)",
               current_option == 5 ? "→ " : "  ",
               g_colors.border_color, ANSI_RESET);
        
        move_cursor(10, 2);
        printf("%sPresets", current_option == 6 ? "→ " : "  ");
        
        move_cursor(12, 2);
        printf("%sBack", current_option == 7 ? "→ " : "  ");
        
        move_cursor(14, 2);
        printf("W/S or ↑/↓ to navigate | Enter to select | 0 to go back");
        fflush(stdout);
        
        flush_input();
        int ch;
        while ((ch = get_key()) == -1);
        
        if (ch == 'w' || ch == 'W') {
            current_option = (current_option - 1 + 8) % 8;
        } else if (ch == 's' || ch == 'S') {
            current_option = (current_option + 1) % 8;
        } else if (ch == '0') {
            done = 1;
        } else if (ch == '\r' || ch == '\n' || ch == 10) {
            if (current_option == 7) {
                done = 1;
            } else if (current_option == 6) {
                show_preset_menu();
            } else {
                int color_selected = 0;
                int selected_color_idx = 0;
                
                while (!color_selected) {
                    clear_screen();
                    move_cursor(1, 2);
                    printf(ANSI_BRIGHT_CYAN ANSI_BOLD "=== SELECT COLOR ===" ANSI_RESET);
                    
                    for (int i = 0; i < 16; i++) {
                        move_cursor(3 + i, 2);
                        printf("%s%s%-20s (Sample Text)%s",
                               selected_color_idx == i ? "→ " : "  ",
                               get_color_by_index(i), color_names[i], ANSI_RESET);
                    }
                    
                    move_cursor(20, 2);
                    printf("%sCustom Hex Color (e.g. #FF5733 or FF5733)",
                           selected_color_idx == 16 ? "→ " : "  ");
                    
                    move_cursor(22, 2);
                    printf("W/S or ↑/↓ to navigate | Enter to select | 0 to cancel");
                    fflush(stdout);
                    
                    flush_input();
                    int color_ch;
                    while ((color_ch = get_key()) == -1);
                    
                    if (color_ch == 'w' || color_ch == 'W') {
                        selected_color_idx = (selected_color_idx - 1 + 17) % 17;
                    } else if (color_ch == 's' || color_ch == 'S') {
                        selected_color_idx = (selected_color_idx + 1) % 17;
                    } else if (color_ch == '0') {
                        color_selected = 1;
                    } else if (color_ch == '\r' || color_ch == '\n' || color_ch == 10) {
                        if (selected_color_idx < 16) {
                            const char* selected_color = get_color_by_index(selected_color_idx);
                            
                            switch(current_option) {
                                case 0: 
                                    g_colors.wall_color = selected_color;
                                    strcpy(g_colors.wall_custom, "");
                                    break;
                                case 1: 
                                    g_colors.visited_color = selected_color;
                                    strcpy(g_colors.visited_custom, "");
                                    break;
                                case 2: 
                                    g_colors.player_color = selected_color;
                                    strcpy(g_colors.player_custom, "");
                                    break;
                                case 3: 
                                    g_colors.goal_color = selected_color;
                                    strcpy(g_colors.goal_custom, "");
                                    break;
                                case 4: 
                                    g_colors.info_color = selected_color;
                                    strcpy(g_colors.info_custom, "");
                                    break;
                                case 5: 
                                    g_colors.border_color = selected_color;
                                    strcpy(g_colors.border_custom, "");
                                    break;
                            }
                            color_selected = 1;
                        } else {
                            clear_screen();
                            move_cursor(10, 2);
                            printf("Enter hex color (e.g. #FF5733 or FF5733): ");
                            fflush(stdout);
                            
                            show_cursor();
                            restore_terminal();
                            
                            char hex_input[20];
                            if (fgets(hex_input, sizeof(hex_input), stdin) != NULL) {
                                hex_input[strcspn(hex_input, "\n")] = 0;
                                
                                char ansi_color[30];
                                hex_to_ansi(hex_input, ansi_color);
                                
                                switch(current_option) {
                                    case 0:
                                        strcpy(g_colors.wall_custom, ansi_color);
                                        g_colors.wall_color = g_colors.wall_custom;
                                        break;
                                    case 1:
                                        strcpy(g_colors.visited_custom, ansi_color);
                                        g_colors.visited_color = g_colors.visited_custom;
                                        break;
                                    case 2:
                                        strcpy(g_colors.player_custom, ansi_color);
                                        g_colors.player_color = g_colors.player_custom;
                                        break;
                                    case 3:
                                        strcpy(g_colors.goal_custom, ansi_color);
                                        g_colors.goal_color = g_colors.goal_custom;
                                        break;
                                    case 4:
                                        strcpy(g_colors.info_custom, ansi_color);
                                        g_colors.info_color = g_colors.info_custom;
                                        break;
                                    case 5:
                                        strcpy(g_colors.border_custom, ansi_color);
                                        g_colors.border_color = g_colors.border_custom;
                                        break;
                                }
                                
                                move_cursor(12, 2);
                                printf("Preview: %sSample Text%s", ansi_color, ANSI_RESET);
                                move_cursor(14, 2);
                                printf("Press any key to continue...");
                                fflush(stdout);
                                getchar();
                            }
                            
                            init_terminal();
                            hide_cursor();
                            color_selected = 1;
                        }
                        
                        save_color_settings();
                    }
                }
            }
        }
    }
}

void custom_difficulty_menu(int* width, int* height) {
    clear_screen();
    move_cursor(2, 2);
    printf(ANSI_BRIGHT_CYAN ANSI_BOLD "=== CUSTOM DIFFICULTY ===" ANSI_RESET);
    
    move_cursor(4, 2);
    printf("Enter maze width (%d-%d, odd numbers only): ", MIN_CUSTOM_SIZE, MAX_MAZE_WIDTH);
    fflush(stdout);
    
    show_cursor();
    restore_terminal();
    
    char input[10];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        *width = atoi(input);
        if (*width < MIN_CUSTOM_SIZE) *width = MIN_CUSTOM_SIZE;
        if (*width > MAX_MAZE_WIDTH) *width = MAX_MAZE_WIDTH;
        if (*width % 2 == 0) (*width)++;
    }
    
    move_cursor(6, 2);
    printf("Enter maze height (%d-%d, odd numbers only): ", MIN_CUSTOM_SIZE, MAX_MAZE_HEIGHT);
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        *height = atoi(input);
        if (*height < MIN_CUSTOM_SIZE) *height = MIN_CUSTOM_SIZE;
        if (*height > MAX_MAZE_HEIGHT) *height = MAX_MAZE_HEIGHT;
        if (*height % 2 == 0) (*height)++;
    }
    
    init_terminal();
    hide_cursor();
    
    clear_screen();
    move_cursor(8, 2);
    printf(ANSI_BRIGHT_GREEN "Custom maze set to %dx%d!" ANSI_RESET, *width, *height);
    move_cursor(10, 2);
    printf("Press any key to continue...");
    fflush(stdout);
    
    flush_input();
    while (get_key() == -1);
}


void shuffle_directions(int dirs[][2], int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp_x = dirs[i][0];
        int temp_y = dirs[i][1];
        dirs[i][0] = dirs[j][0];
        dirs[i][1] = dirs[j][1];
        dirs[j][0] = temp_x;
        dirs[j][1] = temp_y;
    }
}



Maze create_maze(int difficulty, int custom_width, int custom_height, int game_mode) {
    Maze maze;
    
    
    maze.portal_count = 0;
    maze.game_mode = game_mode;
    maze.visible = NULL;
    maze.goal_revealed = 0;
    maze.custom_modifiers = 0;

    if (difficulty == 4) {
        maze.width = custom_width;
        maze.height = custom_height;
    } else {
        switch(difficulty) {
            case 1:
                maze.width = DEFAULT_EASY_WIDTH;
                maze.height = DEFAULT_EASY_HEIGHT;
                break;
            case 2:
                maze.width = DEFAULT_MEDIUM_WIDTH;
                maze.height = DEFAULT_MEDIUM_HEIGHT;
                break;
            case 3:
                maze.width = DEFAULT_HARD_WIDTH;
                maze.height = DEFAULT_HARD_HEIGHT;
                break;
            default:
                maze.width = DEFAULT_EASY_WIDTH;
                maze.height = DEFAULT_EASY_HEIGHT;
        }
    }

    
    if (maze.width % 2 == 0) maze.width++;
    if (maze.height % 2 == 0) maze.height++;
    
    
    if (maze.width < MIN_MAZE_SIZE) maze.width = DEFAULT_EASY_WIDTH;
    if (maze.height < MIN_MAZE_SIZE) maze.height = DEFAULT_EASY_HEIGHT;
    if (maze.width > MAX_MAZE_WIDTH * 1.5) maze.width = MAX_MAZE_WIDTH;
    if (maze.height > MAX_MAZE_HEIGHT * 1.5) maze.height = MAX_MAZE_HEIGHT;

    
    maze.grid = (int**)malloc(maze.height * sizeof(int*));
    if (maze.grid == NULL) {
        
        maze.width = DEFAULT_EASY_WIDTH;
        maze.height = DEFAULT_EASY_HEIGHT;
        maze.grid = (int**)malloc(maze.height * sizeof(int*));
        if (maze.grid == NULL) {
            
            maze.width = 0;
            maze.height = 0;
            return maze;
        }
    }
    
    for (int i = 0; i < maze.height; i++) {
        maze.grid[i] = (int*)malloc(maze.width * sizeof(int));
        if (maze.grid[i] == NULL) {
            
            for (int j = 0; j < i; j++) {
                free(maze.grid[j]);
            }
            free(maze.grid);
            
            maze.width = DEFAULT_EASY_WIDTH;
            maze.height = DEFAULT_EASY_HEIGHT;
            return create_maze(1, 0, 0, game_mode);
        }
        
        for (int j = 0; j < maze.width; j++) {
            if (i == 0 || i == maze.height - 1 || j == 0 || j == maze.width - 1) {
                maze.grid[i][j] = BORDER;
            } else {
                maze.grid[i][j] = WALL;
            }
        }
    }

    int start_x = 1;
    int start_y = 1;
    if (start_x % 2 == 0 && start_x + 1 < maze.width - 1) start_x++;
    if (start_y % 2 == 0 && start_y + 1 < maze.height - 1) start_y++;
    
    generate_maze_dfs(&maze, start_x, start_y);

    
    int base_mode = game_mode & 0xFF;
    
    if (base_mode == MODE_CUSTOM) {
        // Extract modifier flags from upper 8 bits of game_mode
        maze.custom_modifiers = (game_mode >> 8);
        
        if (maze.custom_modifiers & 1) { 
            place_portals(&maze);
        }
        if (maze.custom_modifiers & 2) { 
            init_fog_of_war(&maze);
        }
        if (maze.custom_modifiers & 4) { 
            init_fog_of_war(&maze);
            maze.goal_revealed = 0;
        }
        
    } else {
        
        if (game_mode == MODE_PORTALS) {
            place_portals(&maze);
        }
        if (game_mode == MODE_FOG_OF_WAR || game_mode == MODE_DARK_LABYRINTH) {
            init_fog_of_war(&maze);
        }
    }

    return maze;
}


void stack_init(Stack* s) {
    s->top = -1;
}

int stack_is_empty(Stack* s) {
    return s->top == -1;
}

int stack_is_full(Stack* s) {
    return s->top >= MAX_STACK_SIZE - 1;
}

void stack_push(Stack* s, int x, int y) {
    if (!stack_is_full(s)) {
        s->top++;
        s->items[s->top].x = x;
        s->items[s->top].y = y;
    }
}

StackNode stack_pop(Stack* s) {
    StackNode node = {-1, -1};
    if (!stack_is_empty(s)) {
        node = s->items[s->top];
        s->top--;
    }
    return node;
}


// Iterative DFS maze generation using explicit stack (avoids recursion limits)
void generate_maze_dfs(Maze* maze, int start_x, int start_y) {
    Stack stack;
    stack_init(&stack);
    stack_push(&stack, start_x, start_y);
    maze->grid[start_y][start_x] = PATH;

    while (!stack_is_empty(&stack)) {
        StackNode current = stack_pop(&stack);
        int x = current.x;
        int y = current.y;

        int directions[4][2] = {{2, 0}, {0, 2}, {-2, 0}, {0, -2}};
        shuffle_directions(directions, 4);

        for (int i = 0; i < 4; i++) {
            int nx = x + directions[i][0];
            int ny = y + directions[i][1];

            if (nx > 0 && nx < maze->width - 1 && ny > 0 && ny < maze->height - 1 &&
                maze->grid[ny][nx] == WALL) {
                maze->grid[y + directions[i][1] / 2][x + directions[i][0] / 2] = PATH;
                maze->grid[ny][nx] = PATH;
                stack_push(&stack, x, y);
                stack_push(&stack, nx, ny);
                break;
            }
        }
    }
}


void place_portals(Maze* maze) {
    int portals_to_place = (maze->width * maze->height) / 200 + 2; 
    if (portals_to_place > MAX_PORTALS / 2) portals_to_place = MAX_PORTALS / 2;
    
    maze->portal_count = 0;
    
    for (int p = 0; p < portals_to_place && maze->portal_count < MAX_PORTALS - 1; p++) {
        Point entrance = {0, 0};
        Point exit = {0, 0};
        
        
        int attempts = 0;
        while (attempts < 100) {
            int x = 2 + rand() % (maze->width - 4);
            int y = 2 + rand() % (maze->height - 4);
            if (maze->grid[y][x] == PATH) {
                entrance.x = x;
                entrance.y = y;
                break;
            }
            attempts++;
        }
        
        
        attempts = 0;
        while (attempts < 100) {
            int x = 2 + rand() % (maze->width - 4);
            int y = 2 + rand() % (maze->height - 4);
            int dist_x = abs(x - entrance.x);
            int dist_y = abs(y - entrance.y);
            if (maze->grid[y][x] == PATH && (dist_x > 5 || dist_y > 5)) {
                exit.x = x;
                exit.y = y;
                break;
            }
            attempts++;
        }
        
        if (entrance.x > 0 && exit.x > 0) {
            maze->portals[maze->portal_count].entrance = entrance;
            maze->portals[maze->portal_count].exit = exit;
            maze->portals[maze->portal_count].active = 1;
            maze->portal_count++;
            
            
            maze->portals[maze->portal_count].entrance = exit;
            maze->portals[maze->portal_count].exit = entrance;
            maze->portals[maze->portal_count].active = 1;
            maze->portal_count++;
        }
    }
}


void init_fog_of_war(Maze* maze) {
    maze->visible = (int**)malloc(maze->height * sizeof(int*));
    for (int i = 0; i < maze->height; i++) {
        maze->visible[i] = (int*)calloc(maze->width, sizeof(int));
    }
}


void update_fog_of_war(Maze* maze, Player* player) {
    if (maze->visible == NULL) return;
    
    for (int y = 0; y < maze->height; y++) {
        for (int x = 0; x < maze->width; x++) {
            int dx = abs(x - player->pos.x);
            int dy = abs(y - player->pos.y);
            
            // Circular visibility using distance formula
            if (dx * dx + dy * dy <= FOG_RADIUS * FOG_RADIUS) {
                maze->visible[y][x] = 1;
            }
        }
    }
}

void free_maze(Maze* maze) {
    for (int i = 0; i < maze->height; i++) {
        free(maze->grid[i]);
    }
    free(maze->grid);
    
    
    if (maze->visible != NULL) {
        for (int i = 0; i < maze->height; i++) {
            free(maze->visible[i]);
        }
        free(maze->visible);
    }
}

void draw_maze(Maze* maze, Player* player, Point* goal) {
    clear_screen();

    move_cursor(0, 2);
    const char* mode_names[] = {"Classic", "Portals", "Fog of War", "Dark Labyrinth", "No Return", "Mirror", "Custom"};
    int base_mode = maze->game_mode & 0xFF;
    printf("%sMazes: %d | Steps: %d | Mode: %s | %sWASD/Q%s",
           g_colors.info_color, player->mazes_completed, player->steps,
           mode_names[base_mode], ANSI_BOLD, ANSI_RESET);

    for (int y = 0; y < maze->height; y++) {
        move_cursor(y + 2, 2);
        for (int x = 0; x < maze->width; x++) {
            
            if (maze->visible != NULL && !maze->visible[y][x]) {
                printf("%s%s%s", g_colors.fog_color, FOG_CHAR, ANSI_RESET);
                continue;
            }
            
            int cell = maze->grid[y][x];
            
            
            int is_portal = 0;
            for (int p = 0; p < maze->portal_count; p++) {
                if (maze->portals[p].entrance.x == x && maze->portals[p].entrance.y == y) {
                    printf("%s%s%s", g_colors.portal_color, PORTAL_CHAR, ANSI_RESET);
                    is_portal = 1;
                    break;
                }
            }
            if (is_portal) continue;
            
            
            switch(cell) {
                case BORDER:
                    printf("%s%s%s", g_colors.border_color, WALL_CHAR, ANSI_RESET);
                    break;
                case WALL:
                    printf("%s%s%s", g_colors.wall_color, WALL_CHAR, ANSI_RESET);
                    break;
                case VISITED:
                    printf("%s%s%s", g_colors.visited_color, VISITED_CHAR, ANSI_RESET);
                    break;
                default:
                    printf("%s", PATH_CHAR);
            }
        }
    }

    // Dark labyrinth: goal only appears when within fog radius
    int has_dark = (maze->game_mode == MODE_DARK_LABYRINTH) || 
                   ((base_mode == MODE_CUSTOM) && (maze->custom_modifiers & 4));
    if (!has_dark || maze->goal_revealed) {
        move_cursor(goal->y + 2, goal->x + 2);
        printf("%s%s%s", g_colors.goal_color, GOAL_CHAR, ANSI_RESET);
    }

    move_cursor(player->pos.y + 2, player->pos.x + 2);
    printf("%s%c%s", g_colors.player_color, player->icon, ANSI_RESET);

    fflush(stdout);
}

int is_valid_move(Maze* maze, int x, int y, Player* player) {
    (void)player;
    if (x < 0 || x >= maze->width || y < 0 || y >= maze->height) return 0;
    
    int cell = maze->grid[y][x];
    
    
    if (cell == WALL || cell == BORDER) return 0;
    
    return 1;
}


Point find_farthest_point(Maze* maze, int start_x, int start_y) {
    
    int** visited = (int**)malloc(maze->height * sizeof(int*));
    for (int i = 0; i < maze->height; i++) {
        visited[i] = (int*)calloc(maze->width, sizeof(int));
    }
    
    
    Point* queue = (Point*)malloc(maze->width * maze->height * sizeof(Point));
    int queue_start = 0;
    int queue_end = 0;
    
    Point farthest = {start_x, start_y};
    
    
    queue[queue_end++] = (Point){start_x, start_y};
    visited[start_y][start_x] = 1;
    
    int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    
    while (queue_start < queue_end) {
        Point current = queue[queue_start++];
        farthest = current; // Last visited point is the farthest from start 
        
        for (int i = 0; i < 4; i++) {
            int nx = current.x + directions[i][0];
            int ny = current.y + directions[i][1];
            
            if (nx > 0 && nx < maze->width - 1 && 
                ny > 0 && ny < maze->height - 1 &&
                !visited[ny][nx] && maze->grid[ny][nx] == PATH) {
                visited[ny][nx] = 1;
                queue[queue_end++] = (Point){nx, ny};
            }
        }
    }
    
    
    for (int i = 0; i < maze->height; i++) {
        free(visited[i]);
    }
    free(visited);
    free(queue);
    
    return farthest;
}



int play_game(Maze* maze, Player* player) {
    Point goal = {0, 0};

    if (maze->width < MIN_MAZE_SIZE || maze->height < MIN_MAZE_SIZE) {
        return 0;
    }

    player->pos.x = 1;
    player->pos.y = 1;
    player->steps = 0;

    
    goal = find_farthest_point(maze, player->pos.x, player->pos.y);
    
    
    if (maze->grid[goal.y][goal.x] != PATH) {
        goal.x = maze->width - 2;
        goal.y = maze->height - 2;
    }
    
    
    if (maze->visible != NULL) {
        update_fog_of_war(maze, player);
        
        
        int base_mode = maze->game_mode & 0xFF;
        int has_dark = (maze->game_mode == MODE_DARK_LABYRINTH) || 
                       ((base_mode == MODE_CUSTOM) && (maze->custom_modifiers & 4));
        if (has_dark) {
            int dx = goal.x - player->pos.x;
            int dy = goal.y - player->pos.y;
            if (dx*dx + dy*dy <= FOG_RADIUS*FOG_RADIUS) {
                maze->goal_revealed = 1;
            }
        }
    }

    draw_maze(maze, player, &goal);

    int running = 1;
    while (running) {
        int ch = get_key();
        if (ch == -1) continue;

        int new_x = player->pos.x;
        int new_y = player->pos.y;
        int moved = 0;
        
        
        int base_mode = maze->game_mode & 0xFF;
        int is_mirror = (maze->game_mode == MODE_MIRROR) || 
                        ((base_mode == MODE_CUSTOM) && (maze->custom_modifiers & 16));

        // Mirror mode inverts all movement controls
        switch(ch) {
            case 'w':
            case 'W':
                if (is_mirror) new_y++; else new_y--;
                moved = 1;
                break;
            case 's':
            case 'S':
                if (is_mirror) new_y--; else new_y++;
                moved = 1;
                break;
            case 'a':
            case 'A':
                if (is_mirror) new_x++; else new_x--;
                moved = 1;
                break;
            case 'd':
            case 'D':
                if (is_mirror) new_x--; else new_x++;
                moved = 1;
                break;
            case 'q':
            case 'Q':
                return 0;
        }

        if (moved && is_valid_move(maze, new_x, new_y, player)) {
            // No-return mode: cells become walls after stepping on them
            int has_no_return = (maze->game_mode == MODE_NO_RETURN) || 
                                ((base_mode == MODE_CUSTOM) && (maze->custom_modifiers & 8));
            if (has_no_return) {
                maze->grid[player->pos.y][player->pos.x] = WALL;
            } else {
                maze->grid[player->pos.y][player->pos.x] = VISITED;
            }
            
            player->pos.x = new_x;
            player->pos.y = new_y;
            player->steps++;
            
            
            for (int p = 0; p < maze->portal_count; p++) {
                if (maze->portals[p].entrance.x == new_x && 
                    maze->portals[p].entrance.y == new_y &&
                    maze->portals[p].active) {
                    player->pos.x = maze->portals[p].exit.x;
                    player->pos.y = maze->portals[p].exit.y;
                    break;
                }
            }
            
            
            if (maze->visible != NULL) {
                update_fog_of_war(maze, player);
                
                
                int has_dark = (maze->game_mode == MODE_DARK_LABYRINTH) || 
                               ((base_mode == MODE_CUSTOM) && (maze->custom_modifiers & 4));
                if (has_dark && !maze->goal_revealed) {
                    int dx = goal.x - player->pos.x;
                    int dy = goal.y - player->pos.y;
                    if (dx*dx + dy*dy <= FOG_RADIUS*FOG_RADIUS) {
                        maze->goal_revealed = 1;
                    }
                }
            }
            
            draw_maze(maze, player, &goal);
        }

        if (player->pos.x == goal.x && player->pos.y == goal.y) {
            player->mazes_completed++;
            return 1;
        }
    }

    return 0;
}

void show_win_screen(Player* player) {
    clear_screen();
    move_cursor(10, 30);
    printf("%s%s*** YOU WIN! ***%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);

    move_cursor(12, 25);
    printf("Total Mazes Completed: %d", player->mazes_completed);
    move_cursor(14, 25);
    printf("Play again? (Y/N): ");
    fflush(stdout);
}

void load_scores(Score scores[], int* count) {
    FILE* file = fopen(LEADERBOARD_FILE, "r");
    *count = 0;

    if (file == NULL) {
        return;
    }

    while (*count < MAX_SCORES) {
        long timestamp_long;
        if (fscanf(file, "%19s %d %d %d %ld\n",
           scores[*count].name, &scores[*count].mazes_completed,
           &scores[*count].difficulty, &scores[*count].game_mode, &timestamp_long) == 5) {
            scores[*count].timestamp = (time_t)timestamp_long;
            (*count)++;
        } else {
            break;
        }
    }

    fclose(file);
}


void save_score(const char* name, int mazes, int difficulty, int game_mode) {
    Score scores[MAX_SCORES];
    int count = 0;

    load_scores(scores, &count);

    if (count < MAX_SCORES) {
        strncpy(scores[count].name, name, MAX_NAME_LEN - 1);
        scores[count].name[MAX_NAME_LEN - 1] = '\0';
        scores[count].mazes_completed = mazes;
        scores[count].difficulty = difficulty;
        scores[count].game_mode = game_mode;
        scores[count].timestamp = time(NULL);
        count++;
    } else {
        int lowest_idx = 0;
        for (int i = 1; i < count; i++) {
            if (scores[i].mazes_completed < scores[lowest_idx].mazes_completed) {
                lowest_idx = i;
            }
        }

        if (mazes > scores[lowest_idx].mazes_completed) {
            strncpy(scores[lowest_idx].name, name, MAX_NAME_LEN - 1);
            scores[lowest_idx].name[MAX_NAME_LEN - 1] = '\0';
            scores[lowest_idx].mazes_completed = mazes;
            scores[lowest_idx].difficulty = difficulty;
            scores[lowest_idx].game_mode = game_mode;
            scores[lowest_idx].timestamp = time(NULL);
        }
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j].mazes_completed < scores[j + 1].mazes_completed) {
                Score temp = scores[j];
                scores[j] = scores[j + 1];
                scores[j + 1] = temp;
            }
        }
    }

    FILE* file = fopen(LEADERBOARD_FILE, "w");
    if (file == NULL) {
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %d %d %d %ld\n", scores[i].name,
                scores[i].mazes_completed, scores[i].difficulty, scores[i].game_mode, (long)scores[i].timestamp);
    }

    fclose(file);
}

void show_leaderboard() {
    clear_screen();

    move_cursor(2, 2);
    printf("%s%s=== TOP 10 LEADERBOARD ===%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);

    Score scores[MAX_SCORES];
    int count = 0;
    load_scores(scores, &count);

    if (count == 0) {
        move_cursor(4, 2);
        printf("No scores yet!");
    } else {
        move_cursor(4, 2);
        printf("Rank  Name            Mazes  Difficulty  Mode          Date");
        move_cursor(5, 2);
        printf("-------------------------------------------------------------------");

        const char* diff_names[] = {"", "Easy", "Medium", "Hard", "Custom"};
        const char* mode_names[] = {"Classic", "Portals", "Fog", "Dark", "NoRet", "Mirror", "Custom"};

        for (int i = 0; i < count; i++) {
            struct tm *tm_info = localtime(&scores[i].timestamp);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            
            int base_mode = scores[i].game_mode & 0xFF;
            const char* mode_name = (base_mode < 8) ? mode_names[base_mode] : "Unknown";
            
            move_cursor(6 + i, 2);
            printf("%s#%-4d %-15s %-6d %-11s %-13s %s%s",
                   g_colors.player_color,
                   i + 1, scores[i].name, scores[i].mazes_completed,
                   diff_names[scores[i].difficulty], mode_name, time_str, ANSI_RESET);
        }
    }

    move_cursor(20, 2);
    printf("Press any key to continue... (or 0 to go back)");
    fflush(stdout);
    
    flush_input();
    while (get_key() == -1);
}

void get_player_name(char* name, int mazes, int difficulty, int game_mode) {
    clear_screen();

    move_cursor(8, 25);
    printf("%s%sNEW RECORD!%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);

    move_cursor(10, 20);
    printf("You completed %d mazes!", mazes);
    move_cursor(12, 20);
    printf("Enter your name: ");
    fflush(stdout);

    show_cursor();
    restore_terminal();
    
    char input[MAX_NAME_LEN];
    if (fgets(input, MAX_NAME_LEN, stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        strncpy(name, input, MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';
    }

    int len = strlen(name);
    if (len == 0 || name[0] == ' ') {
        strcpy(name, "Anonymous");
    }

    init_terminal();
    hide_cursor();

    save_score(name, mazes, difficulty, game_mode);

    clear_screen();
    move_cursor(12, 20);
    printf("%sScore saved! Check the leaderboard!%s", g_colors.goal_color, ANSI_RESET);
    move_cursor(14, 25);
    printf("Press any key...");
    fflush(stdout);
    
    flush_input();
    while (get_key() == -1);
}

int show_custom_mode_menu() {
    int modifiers = 0;
    int current_option = 0;
    int done = 0;
    
    #define CUSTOM_PORTALS 1
    #define CUSTOM_FOG 2
    #define CUSTOM_DARK 4
    #define CUSTOM_NO_RETURN 8
    #define CUSTOM_MIRROR 16
    
    while (!done) {
        clear_screen();
        move_cursor(2, 20);
        printf("%s%s=== CUSTOM MODE - SELECT MODIFIERS ===%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);
        
        move_cursor(4, 4);
        printf("%s[%c] Portals%s", current_option == 0 ? "→ " : "  ",
               (modifiers & CUSTOM_PORTALS) ? 'X' : ' ', ANSI_RESET);
        
        move_cursor(5, 4);
        printf("%s[%c] Fog of War%s", current_option == 1 ? "→ " : "  ",
               (modifiers & CUSTOM_FOG) ? 'X' : ' ', ANSI_RESET);
        
        move_cursor(6, 4);
        printf("%s[%c] Dark Labyrinth (Goal hidden)%s", current_option == 2 ? "→ " : "  ",
               (modifiers & CUSTOM_DARK) ? 'X' : ' ', ANSI_RESET);
        
        move_cursor(7, 4);
        printf("%s[%c] No Return (Path becomes wall)%s", current_option == 3 ? "→ " : "  ",
               (modifiers & CUSTOM_NO_RETURN) ? 'X' : ' ', ANSI_RESET);
        
        move_cursor(8, 4);
        printf("%s[%c] Mirror Mode (Inverted controls)%s", current_option == 4 ? "→ " : "  ",
               (modifiers & CUSTOM_MIRROR) ? 'X' : ' ', ANSI_RESET);
        
        move_cursor(10, 4);
        printf("%sStart Game", current_option == 5 ? "→ " : "  ");
        
        move_cursor(11, 4);
        printf("%sBack", current_option == 6 ? "→ " : "  ");
        
        move_cursor(13, 2);
        printf("W/S: Navigate | Enter/Space: Toggle | 0: Back");
        fflush(stdout);

        flush_input();
        int ch;
        while ((ch = get_key()) == -1);
        
        if (ch == 'w' || ch == 'W') {
            current_option = (current_option - 1 + 7) % 7;
        } else if (ch == 's' || ch == 'S') {
            current_option = (current_option + 1) % 7;
        } else if (ch == '0') {
            return -1;
        } else if (ch == '\r' || ch == '\n' || ch == 10 || ch == ' ') {
            if (current_option == 5) {
                return modifiers;
            } else if (current_option == 6) {
                return -1;
            } else if (current_option == 0) {
                modifiers ^= CUSTOM_PORTALS;
            } else if (current_option == 1) {
                modifiers ^= CUSTOM_FOG;
            } else if (current_option == 2) {
                modifiers ^= CUSTOM_DARK;
            } else if (current_option == 3) {
                modifiers ^= CUSTOM_NO_RETURN;
            } else if (current_option == 4) {
                modifiers ^= CUSTOM_MIRROR;
            }
        }
    }
    
    return 0;
}

int show_mode_selection_menu() {
    int selected_mode = 0;
    int mode_done = 0;
    
    while (!mode_done) {
        clear_screen();
        move_cursor(2, 25);
        printf("%s%s=== SELECT GAME MODE ===%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);
        
        move_cursor(4, 4);
        printf("%s%s0. Classic Mode%s", selected_mode == 0 ? "→ " : "  ",
               selected_mode == 0 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(5, 6);
        printf("Complex mazes with loops & multiple paths");
        
        move_cursor(7, 4);
        printf("%s%s1. Portal Mode%s", selected_mode == 1 ? "→ " : "  ",
               selected_mode == 1 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(8, 6);
        printf("%sTeleport through magical portals!%s", g_colors.portal_color, ANSI_RESET);
        
        move_cursor(10, 4);
        printf("%s%s2. Fog of War Mode%s", selected_mode == 2 ? "→ " : "  ",
               selected_mode == 2 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(11, 6);
        printf("Limited visibility - explore carefully!");
        
        move_cursor(13, 4);
        printf("%s%s3. Dark Labyrinth%s", selected_mode == 3 ? "→ " : "  ",
               selected_mode == 3 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(14, 6);
        printf("Goal appears only when visible!");
        
        move_cursor(16, 4);
        printf("%s%s4. No Return%s", selected_mode == 4 ? "→ " : "  ",
               selected_mode == 4 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(17, 6);
        printf("%sPath becomes wall behind you!%s", ANSI_BRIGHT_YELLOW, ANSI_RESET);
        
        move_cursor(19, 4);
        printf("%s%s5. Mirror Mode%s", selected_mode == 5 ? "→ " : "  ",
               selected_mode == 5 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(20, 6);
        printf("%sInverted controls - W→S, A→D%s", ANSI_BRIGHT_CYAN, ANSI_RESET);
        
        move_cursor(22, 4);
        printf("%s%s6. Custom Mode%s", selected_mode == 6 ? "→ " : "  ",
               selected_mode == 6 ? ANSI_BOLD : "", ANSI_RESET);
        move_cursor(23, 6);
        printf("%sMix and match modifiers!%s", ANSI_BRIGHT_MAGENTA, ANSI_RESET);
        
        move_cursor(25, 4);
        printf("%sBack", selected_mode == 7 ? "→ " : "  ");
        
        move_cursor(30, 2);
        printf("W/S or ↑/↓ to navigate | Enter to select | 0 to go back");
        fflush(stdout);

        flush_input();
        int mode_ch;
        while ((mode_ch = get_key()) == -1);
        
        if (mode_ch == 'w' || mode_ch == 'W') {
            selected_mode = (selected_mode - 1 + 8) % 8;
        } else if (mode_ch == 's' || mode_ch == 'S') {
            selected_mode = (selected_mode + 1) % 8;
        } else if (mode_ch == '0') {
            return -1;
        } else if (mode_ch == '\r' || mode_ch == '\n' || mode_ch == 10) {
            if (selected_mode == 7) {
                return -1;
            }
            if (selected_mode == 6) {
                // Custom mode: pack modifiers in upper bits (bits 8-15)
                int mods = show_custom_mode_menu();
                if (mods == -1) continue;
                return MODE_CUSTOM | (mods << 8);
            }
            return selected_mode;
        }
    }
    
    return MODE_CLASSIC;
}

int show_menu(char* player_icon, int* difficulty, int* game_mode) {
    int current_option = 0;
    int done = 0;
    
    while (!done) {
        clear_screen();

        move_cursor(2, 30);
        printf("%s%s=== TTY Maze ===%s", g_colors.goal_color, ANSI_BOLD, ANSI_RESET);

        move_cursor(4, 2);
        printf("%sPlay Game", current_option == 0 ? "→ " : "  ");
        move_cursor(5, 2);
        printf("%sView Leaderboard", current_option == 1 ? "→ " : "  ");
        move_cursor(6, 2);
        printf("%sCustomize Colors", current_option == 2 ? "→ " : "  ");
        move_cursor(7, 2);
        printf("%sExit", current_option == 3 ? "→ " : "  ");
        move_cursor(9, 2);
        printf("W/S or ↑/↓ to navigate | Enter to select | 0 to exit");
        fflush(stdout);

        int ch;
        flush_input();
        while ((ch = get_key()) == -1);
        
        if (ch == 'w' || ch == 'W') {
            current_option = (current_option - 1 + 4) % 4;
        } else if (ch == 's' || ch == 'S') {
            current_option = (current_option + 1) % 4;
        } else if (ch == '0') {
            return 0;
        } else if (ch == '\r' || ch == '\n' || ch == 10) {
            if (current_option == 0) {
                done = 1;
            } else if (current_option == 1) {
                show_leaderboard();
            } else if (current_option == 2) {
                color_customization_menu();
            } else if (current_option == 3) {
                return 0;
            }
        }
    }

    clear_screen();
    move_cursor(10, 2);
    printf("Press any key for your character icon: %s %s", g_colors.player_color, ANSI_RESET);
    move_cursor(12, 2);
    printf("(Press Enter to confirm)");
    fflush(stdout);

    flush_input();
    
    char selected_icon = '@';
    int confirmed = 0;
    
    move_cursor(10, 45);
    printf("%s%c%s", g_colors.player_color, selected_icon, ANSI_RESET);
    fflush(stdout);
    
    while (!confirmed) {
        int ch = -1;
        while (ch == -1) {
            ch = get_key();
        }
        
        if (ch == '\n' || ch == '\r' || ch == 10) {
            confirmed = 1;
        } else if (ch >= 33 && ch <= 126) {
            selected_icon = (char)ch;
            move_cursor(10, 45);
            printf("%s%c%s ", g_colors.player_color, selected_icon, ANSI_RESET);
            fflush(stdout);
        }
    }
    
    *player_icon = selected_icon;

    int selected_diff = 0;
    int diff_done = 0;
    
    while (!diff_done) {
        clear_screen();
        move_cursor(2, 2);
        printf("Select Difficulty:");
        move_cursor(4, 4);
        printf("%sEasy (21x15)", selected_diff == 0 ? "→ " : "  ");
        move_cursor(5, 4);
        printf("%sMedium (31x21)", selected_diff == 1 ? "→ " : "  ");
        move_cursor(6, 4);
        printf("%sHard (41x31)", selected_diff == 2 ? "→ " : "  ");
        move_cursor(7, 4);
        printf("%sCustom", selected_diff == 3 ? "→ " : "  ");
        move_cursor(8, 4);
        printf("%sBack", selected_diff == 4 ? "→ " : "  ");
        move_cursor(10, 2);
        printf("W/S or ↑/↓ to navigate | Enter to select | 0 to go back");
        fflush(stdout);

        flush_input();
        int diff_ch;
        while ((diff_ch = get_key()) == -1);
        
        if (diff_ch == 'w' || diff_ch == 'W') {
            selected_diff = (selected_diff - 1 + 5) % 5;
        } else if (diff_ch == 's' || diff_ch == 'S') {
            selected_diff = (selected_diff + 1) % 5;
        } else if (diff_ch == '0') {
            return show_menu(player_icon, difficulty, game_mode);
        } else if (diff_ch == '\r' || diff_ch == '\n' || diff_ch == 10) {
            if (selected_diff == 4) {
                return show_menu(player_icon, difficulty, game_mode);
            }
            *difficulty = selected_diff + 1;
            diff_done = 1;
        }
    }
    
    
    *game_mode = show_mode_selection_menu();
    if (*game_mode == -1) {
        return show_menu(player_icon, difficulty, game_mode);
    }

    return 1;
}

int main() {
    srand(time(NULL));
    init_terminal();
    hide_cursor();

    load_color_settings();

    Player player;
    player.mazes_completed = 0;
    player.icon = '@';
    player.steps = 0;

    int playing = 1;
    int difficulty = 0;
    int game_mode = MODE_CLASSIC;
    int custom_width = 21;
    int custom_height = 15;

    while (playing) {
        if (!show_menu(&player.icon, &difficulty, &game_mode)) {
            if (player.mazes_completed > 0) {
                char name[MAX_NAME_LEN];
                get_player_name(name, player.mazes_completed, difficulty, game_mode);
            }
            break;
        }

        if (difficulty == 4) {
            custom_difficulty_menu(&custom_width, &custom_height);
        }

        int session_active = 1;
        while (session_active) {
            Maze maze = create_maze(difficulty, custom_width, custom_height, game_mode);
            
            
            if (maze.grid == NULL || maze.width < MIN_MAZE_SIZE || maze.height < MIN_MAZE_SIZE) {
                clear_screen();
                move_cursor(10, 2);
                printf("Error creating maze! Press any key to return to menu...");
                fflush(stdout);
                flush_input();
                while (get_key() == -1);
                free_maze(&maze);
                break;
            }

            int result = play_game(&maze, &player);

            if (result == 1) {
                show_win_screen(&player);
                flush_input();
                int ch = -1;
                while (1) {
                    ch = get_key();
                    if (ch == 'y' || ch == 'Y' || ch == 'n' || ch == 'N') {
                        break;
                    }
                }
                if (ch == 'n' || ch == 'N') {
                    session_active = 0;

                    if (player.mazes_completed > 0) {
                        char name[MAX_NAME_LEN];
                        get_player_name(name, player.mazes_completed, difficulty, game_mode);
                    }

                    player.mazes_completed = 0;
                }
            } else {
                session_active = 0;

                if (player.mazes_completed > 0) {
                    char name[MAX_NAME_LEN];
                    get_player_name(name, player.mazes_completed, difficulty, game_mode);
                }

                player.mazes_completed = 0;
            }

            free_maze(&maze);
        }
    }

    show_cursor();
    restore_terminal();
    clear_screen();

    printf("Thanks for playing!\n");

    return 0;
}
