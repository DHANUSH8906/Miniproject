#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define WIDTH 60
#define HEIGHT 22
#define MAX_SHAPES 100

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

// --- Data Structures ---

typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

typedef struct { int x1, y1, x2, y2; } LineData;
typedef struct { int x, y, width, height; } RectData;
typedef struct { int cx, cy, radius; } CircleData;
typedef struct { int x1, y1, x2, y2, x3, y3; } TriangleData;

typedef struct {
    int id;
    int active;
    ShapeType type;
    char draw_char;
    char color;
    int is_filled;
    union {
        LineData line;
        RectData rect;
        CircleData circle;
        TriangleData triangle;
    } data;
} Shape;

typedef struct {
    char ch;
    char color;
} Pixel;

// Global variables
Pixel canvas[HEIGHT][WIDTH];
Shape shapes[MAX_SHAPES];
int next_shape_id = 1;

// UI State
ShapeType current_shape = SHAPE_RECTANGLE;
char current_color = 'W';
int current_fill = 0;

// --- Console Helpers ---

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() {
    printf("\x1b[?25l");
    fflush(stdout);
}

const char* getAnsiColor(char c) {
    switch (c) {
        case 'R': return "\x1b[31m";
        case 'G': return "\x1b[32m";
        case 'Y': return "\x1b[33m";
        case 'B': return "\x1b[34m";
        case 'M': return "\x1b[35m";
        case 'C': return "\x1b[36m";
        case 'W': return "\x1b[37m";
        default: return "\x1b[0m";
    }
}

// --- Object Management ---

int addObject(Shape shape) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (!shapes[i].active) {
            shapes[i] = shape;
            shapes[i].id = next_shape_id++;
            shapes[i].active = 1;
            return shapes[i].id;
        }
    }
    return -1;
}

void deleteObject(int id) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active && shapes[i].id == id) {
            shapes[i].active = 0;
            return;
        }
    }
}

// --- Drawing Algorithms ---

void plot(int x, int y, char c, char color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        canvas[y][x].ch = c;
        canvas[y][x].color = color;
    }
}

void drawLine(int x1, int y1, int x2, int y2, char c, char color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        plot(x1, y1, c, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void drawCirclePoints(int cx, int cy, int x, int y, char c, char color) {
    plot(cx + x, cy + y, c, color);
    plot(cx - x, cy + y, c, color);
    plot(cx + x, cy - y, c, color);
    plot(cx - x, cy - y, c, color);
    plot(cx + y, cy + x, c, color);
    plot(cx - y, cy + x, c, color);
    plot(cx + y, cy - x, c, color);
    plot(cx - y, cy - x, c, color);
}

void drawCircle(int cx, int cy, int radius, char c, char color, int is_filled) {
    if (is_filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) plot(cx + x, cy + y, c, color);
            }
        }
        return;
    }
    int x = 0, y = radius, p = 1 - radius;
    drawCirclePoints(cx, cy, x, y, c, color);
    while (x < y) {
        x++;
        if (p < 0) p += 2 * x + 1;
        else { y--; p += 2 * (x - y) + 1; }
        drawCirclePoints(cx, cy, x, y, c, color);
    }
}

void drawRectangle(int x, int y, int width, int height, char c, char color, int is_filled) {
    if (is_filled) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) plot(x + i, y + j, c, color);
        }
        return;
    }
    drawLine(x, y, x + width - 1, y, c, color);
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, c, color);
    drawLine(x, y, x, y + height - 1, c, color);
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, c, color);
}

int edgeFunction(int ax, int ay, int bx, int by, int px, int py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, char c, char color, int is_filled) {
    if (is_filled) {
        int minX = x1; if (x2 < minX) minX = x2; if (x3 < minX) minX = x3;
        int minY = y1; if (y2 < minY) minY = y2; if (y3 < minY) minY = y3;
        int maxX = x1; if (x2 > maxX) maxX = x2; if (x3 > maxX) maxX = x3;
        int maxY = y1; if (y2 > maxY) maxY = y2; if (y3 > maxY) maxY = y3;

        if (edgeFunction(x1, y1, x2, y2, x3, y3) < 0) {
            int tx = x2, ty = y2; x2 = x3; y2 = y3; x3 = tx; y3 = ty;
        }
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                int w1 = edgeFunction(x2, y2, x3, y3, x, y);
                int w2 = edgeFunction(x3, y3, x1, y1, x, y);
                int w3 = edgeFunction(x1, y1, x2, y2, x, y);
                if (w1 >= 0 && w2 >= 0 && w3 >= 0) plot(x, y, c, color);
            }
        }
        return;
    }
    drawLine(x1, y1, x2, y2, c, color);
    drawLine(x2, y2, x3, y3, c, color);
    drawLine(x3, y3, x1, y1, c, color);
}

// --- Rendering & Display ---

void render() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x].ch = ' ';
            canvas[y][x].color = ' ';
        }
    }
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active) {
            Shape* s = &shapes[i];
            switch (s->type) {
                case SHAPE_LINE:
                    drawLine(s->data.line.x1, s->data.line.y1, 
                             s->data.line.x2, s->data.line.y2, s->draw_char, s->color);
                    break;
                case SHAPE_RECTANGLE:
                    drawRectangle(s->data.rect.x, s->data.rect.y, 
                                  s->data.rect.width, s->data.rect.height, s->draw_char, s->color, s->is_filled);
                    break;
                case SHAPE_CIRCLE:
                    drawCircle(s->data.circle.cx, s->data.circle.cy, 
                               s->data.circle.radius, s->draw_char, s->color, s->is_filled);
                    break;
                case SHAPE_TRIANGLE:
                    drawTriangle(s->data.triangle.x1, s->data.triangle.y1,
                                 s->data.triangle.x2, s->data.triangle.y2,
                                 s->data.triangle.x3, s->data.triangle.y3, s->draw_char, s->color, s->is_filled);
                    break;
            }
        }
    }
}

void drawUI() {
    // 1. Draw Canvas Border
    printf("\x1b[0m");
    for (int y = 0; y < HEIGHT + 2; y++) {
        gotoxy(0, y); putchar('|');
        gotoxy(WIDTH + 1, y); putchar('|');
    }
    for (int x = 0; x < WIDTH + 2; x++) {
        gotoxy(x, 0); putchar('-');
        gotoxy(x, HEIGHT + 1); putchar('-');
    }
    gotoxy(0, 0); putchar('+');
    gotoxy(WIDTH + 1, 0); putchar('+');
    gotoxy(0, HEIGHT + 1); putchar('+');
    gotoxy(WIDTH + 1, HEIGHT + 1); putchar('+');

    // 2. Draw Canvas Content
    render();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            Pixel p = canvas[y][x];
            gotoxy(x + 1, y + 1);
            if (p.ch != ' ') {
                printf("%s%c\x1b[0m", getAnsiColor(p.color), p.ch);
            } else {
                putchar(' ');
            }
        }
    }

    // 3. Draw Menu on right
    int mx = WIDTH + 4;
    gotoxy(mx, 2); printf("=== TOOLS ===");
    gotoxy(mx, 4); printf("[R]ectangle %s", current_shape == SHAPE_RECTANGLE ? "<" : " ");
    gotoxy(mx, 5); printf("[C]ircle    %s", current_shape == SHAPE_CIRCLE ? "<" : " ");
    gotoxy(mx, 6); printf("[L]ine      %s", current_shape == SHAPE_LINE ? "<" : " ");
    gotoxy(mx, 7); printf("[T]riangle  %s", current_shape == SHAPE_TRIANGLE ? "<" : " ");

    gotoxy(mx, 9); printf("=== COLOR ===");
    gotoxy(mx, 11); printf("\x1b[31mRed \x1b[0m    %s", current_color == 'R' ? "<" : " ");
    gotoxy(mx, 12); printf("\x1b[32mGrn \x1b[0m    %s", current_color == 'G' ? "<" : " ");
    gotoxy(mx, 13); printf("\x1b[34mBlu \x1b[0m    %s", current_color == 'B' ? "<" : " ");
    gotoxy(mx, 14); printf("\x1b[33mYel \x1b[0m    %s", current_color == 'Y' ? "<" : " ");
    gotoxy(mx, 15); printf("\x1b[37mWht \x1b[0m    %s", current_color == 'W' ? "<" : " ");

    gotoxy(mx, 17); printf("=== OPTIONS ===");
    gotoxy(mx, 19); printf("Fill: %s", current_fill ? "ON " : "OFF");
    
    gotoxy(mx, 21); printf("[-] Undo Last");
    gotoxy(mx, 22); printf("[X] Clear All");
    gotoxy(mx, 23); printf("[Q] Quit     ");
    
    // Debug info at the bottom
    gotoxy(0, HEIGHT + 4);
    printf("DEBUG: Mode=%d | Tool=%d | Col=%c | Fill=%d            ", current_shape, current_shape, current_color, current_fill);
}

void handleMouseClick(int mouse_y, int mouse_x) {
    if (mouse_x >= 1 && mouse_x <= WIDTH && mouse_y >= 1 && mouse_y <= HEIGHT) {
        int cx = mouse_x - 1;
        int cy = mouse_y - 1;
        
        Shape s;
        s.active = 0;
        s.color = current_color;
        s.is_filled = current_fill;
        s.draw_char = '*';

        if (current_shape == SHAPE_RECTANGLE) {
            s.type = SHAPE_RECTANGLE;
            s.data.rect.x = cx - 5;
            s.data.rect.y = cy - 3;
            s.data.rect.width = 10;
            s.data.rect.height = 6;
            addObject(s);
        } else if (current_shape == SHAPE_CIRCLE) {
            s.type = SHAPE_CIRCLE;
            s.data.circle.cx = cx;
            s.data.circle.cy = cy;
            s.data.circle.radius = 5;
            addObject(s);
        } else if (current_shape == SHAPE_LINE) {
            s.type = SHAPE_LINE;
            s.data.line.x1 = cx - 4;
            s.data.line.y1 = cy - 2;
            s.data.line.x2 = cx + 4;
            s.data.line.y2 = cy + 2;
            s.is_filled = 0;
            addObject(s);
        } else if (current_shape == SHAPE_TRIANGLE) {
            s.type = SHAPE_TRIANGLE;
            s.data.triangle.x1 = cx;
            s.data.triangle.y1 = cy - 4;
            s.data.triangle.x2 = cx - 8;
            s.data.triangle.y2 = cy + 4;
            s.data.triangle.x3 = cx + 8;
            s.data.triangle.y3 = cy + 4;
            addObject(s);
        }
    } 
    else {
        int mx = WIDTH + 4;
        if (mouse_x >= mx && mouse_x <= mx + 15) {
            if (mouse_y == 4) current_shape = SHAPE_RECTANGLE;
            else if (mouse_y == 5) current_shape = SHAPE_CIRCLE;
            else if (mouse_y == 6) current_shape = SHAPE_LINE;
            else if (mouse_y == 7) current_shape = SHAPE_TRIANGLE;
            
            else if (mouse_y == 11) current_color = 'R';
            else if (mouse_y == 12) current_color = 'G';
            else if (mouse_y == 13) current_color = 'B';
            else if (mouse_y == 14) current_color = 'Y';
            else if (mouse_y == 15) current_color = 'W';

            else if (mouse_y == 19) current_fill = !current_fill;
            
            else if (mouse_y == 21) {
                int max_id = -1;
                for (int i=0; i<MAX_SHAPES; i++) {
                    if (shapes[i].active && shapes[i].id > max_id) max_id = shapes[i].id;
                }
                if (max_id != -1) deleteObject(max_id);
            }
            else if (mouse_y == 22) {
                for (int i=0; i<MAX_SHAPES; i++) shapes[i].active = 0;
            }
            else if (mouse_y == 23) {
                system("cls");
                exit(0);
            }
        }
    }
}

    int main() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Enable ANSI escape sequences
    DWORD outMode = 0;
    GetConsoleMode(hStdout, &outMode);
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdout, outMode);

    SetConsoleMode(hStdin, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);

    // Use VT sequences to enter Alternate Buffer, Clear Screen, Hide Cursor, and Enable Mouse
    printf("\x1b[?1049h"); // Alternate screen buffer
    printf("\x1b[2J\x1b[H"); // Clear screen and home cursor
    printf("\x1b[?25l"); // Hide cursor
    printf("\x1b[?1003h\x1b[?1015h\x1b[?1006h"); // Enable Mouse tracking (Any Event)
    fflush(stdout);

    for (int i=0; i<MAX_SHAPES; i++) shapes[i].active = 0;

    drawUI();

    INPUT_RECORD irInBuf[128];
    DWORD cNumRead;
    int prev_button_state = 0;
    char seq[32];
    int seq_len = 0;
    int in_esc = 0;
    
    while (1) {
        if (!ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead)) {
            continue;
        }

        int ui_needs_redraw = 0;

        for (DWORD i = 0; i < cNumRead; i++) {
            if (irInBuf[i].EventType == MOUSE_EVENT) {
                // Native Windows Console Mouse Event (now handles VS Code too!)
                MOUSE_EVENT_RECORD mer = irInBuf[i].Event.MouseEvent;
                int is_pressed = (mer.dwButtonState != 0);
                
                if (is_pressed && !prev_button_state) {
                    handleMouseClick(mer.dwMousePosition.Y, mer.dwMousePosition.X);
                    ui_needs_redraw = 1;
                }
                prev_button_state = is_pressed;
                
            } else if (irInBuf[i].EventType == KEY_EVENT) {
                KEY_EVENT_RECORD ker = irInBuf[i].Event.KeyEvent;
                if (ker.bKeyDown) {
                    char c = ker.uChar.AsciiChar;
                    
                    if (c == '\x1b') {
                        in_esc = 1;
                        seq_len = 0;
                        seq[seq_len++] = c;
                    } else if (in_esc) {
                        if (seq_len < 31) seq[seq_len++] = c;
                        
                        // Check if sequence is complete
                        if (c == 'M' || c == 'm' || (c >= 'A' && c <= 'Z' && c != 'O')) {
                            seq[seq_len] = '\0';
                            
                            // Parse VT Mouse Sequence (e.g. \x1b[<0;65;5M)
                            if (strncmp(seq, "\x1b[<", 3) == 0) {
                                int btn, mx, my;
                                char type;
                                if (sscanf(seq + 3, "%d;%d;%d%c", &btn, &mx, &my, &type) == 4) {
                                    if (type == 'M' && (btn == 0 || btn == 32)) { 
                                        // Convert 1-based VT coords to 0-based
                                        handleMouseClick(my - 1, mx - 1);
                                        ui_needs_redraw = 1;
                                    }
                                }
                            }
                            in_esc = 0;
                        }
                    } else {
                        if (c == 'q' || c == 'Q') {
                            // Restore terminal state before quitting
                            printf("\x1b[?1003l\x1b[?1015l\x1b[?1006l"); // Disable mouse
                            printf("\x1b[?25h"); // Show cursor
                            printf("\x1b[?1049l"); // Leave alternate buffer
                            fflush(stdout);
                            return 0;
                        }
                    }
                }
            }
        }

        if (ui_needs_redraw) {
            drawUI();
        }
    }

    return 0;
}
