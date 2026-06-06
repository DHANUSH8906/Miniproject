#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 80
#define HEIGHT 40
#define MAX_SHAPES 100

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
    union {
        LineData line;
        RectData rect;
        CircleData circle;
        TriangleData triangle;
    } data;
} Shape;

// Global variables
char canvas[HEIGHT][WIDTH];
Shape shapes[MAX_SHAPES];
int next_shape_id = 1;


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
    printf("Error: Maximum number of shapes reached.\n");
    return -1;
}

void deleteObject(int id) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active && shapes[i].id == id) {
            shapes[i].active = 0;
            return;
        }
    }
    printf("Warning: Shape with ID %d not found.\n", id);
}

Shape* getObject(int id) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active && shapes[i].id == id) {
            return &shapes[i];
        }
    }
    return NULL;
}


// --- Drawing Algorithms ---

void plot(int x, int y, char c) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        // We divide x by 2 for visually square aspect ratio on many terminal fonts? 
        // No, let's keep it simple 1:1 mapping to the 2D array.
        canvas[y][x] = c;
    }
}

void drawLine(int x1, int y1, int x2, int y2, char c) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        plot(x1, y1, c);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void drawCirclePoints(int cx, int cy, int x, int y, char c) {
    plot(cx + x, cy + y, c);
    plot(cx - x, cy + y, c);
    plot(cx + x, cy - y, c);
    plot(cx - x, cy - y, c);
    plot(cx + y, cy + x, c);
    plot(cx - y, cy + x, c);
    plot(cx + y, cy - x, c);
    plot(cx - y, cy - x, c);
}

void drawCircle(int cx, int cy, int radius, char c) {
    int x = 0;
    int y = radius;
    int p = 1 - radius;

    drawCirclePoints(cx, cy, x, y, c);

    while (x < y) {
        x++;
        if (p < 0) {
            p += 2 * x + 1;
        } else {
            y--;
            p += 2 * (x - y) + 1;
        }
        drawCirclePoints(cx, cy, x, y, c);
    }
}

void drawRectangle(int x, int y, int width, int height, char c) {
    drawLine(x, y, x + width - 1, y, c); // Top
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, c); // Bottom
    drawLine(x, y, x, y + height - 1, c); // Left
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, c); // Right
}

void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, char c) {
    drawLine(x1, y1, x2, y2, c);
    drawLine(x2, y2, x3, y3, c);
    drawLine(x3, y3, x1, y1, c);
}


// --- Rendering & Display ---

void render() {
    // Clear canvas
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x] = ' ';
        }
    }

    // Draw all active shapes
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active) {
            Shape* s = &shapes[i];
            switch (s->type) {
                case SHAPE_LINE:
                    drawLine(s->data.line.x1, s->data.line.y1, 
                             s->data.line.x2, s->data.line.y2, s->draw_char);
                    break;
                case SHAPE_RECTANGLE:
                    drawRectangle(s->data.rect.x, s->data.rect.y, 
                                  s->data.rect.width, s->data.rect.height, s->draw_char);
                    break;
                case SHAPE_CIRCLE:
                    drawCircle(s->data.circle.cx, s->data.circle.cy, 
                               s->data.circle.radius, s->draw_char);
                    break;
                case SHAPE_TRIANGLE:
                    drawTriangle(s->data.triangle.x1, s->data.triangle.y1,
                                 s->data.triangle.x2, s->data.triangle.y2,
                                 s->data.triangle.x3, s->data.triangle.y3, s->draw_char);
                    break;
            }
        }
    }
}

void displayPicture() {
    // Render the shapes to the canvas first
    render();

    // Print top border
    printf("+");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");

    // Print canvas row by row
    for (int y = 0; y < HEIGHT; y++) {
        printf("|");
        for (int x = 0; x < WIDTH; x++) {
            putchar(canvas[y][x]);
        }
        printf("|\n");
    }

    // Print bottom border
    printf("+");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");
}


// --- Main Demonstration ---

int main() {
    // Initialize shapes array
    for (int i = 0; i < MAX_SHAPES; i++) {
        shapes[i].active = 0;
    }

    printf("--- Initial Empty Canvas ---\n");
    displayPicture();

    // Add some shapes
    Shape line = { .type = SHAPE_LINE, .draw_char = '*' };
    line.data.line = (LineData){ 5, 5, 25, 15 }; // Top-Left
    int lineId = addObject(line);

    Shape rect = { .type = SHAPE_RECTANGLE, .draw_char = '*' };
    rect.data.rect = (RectData){ 45, 5, 30, 10 }; // Top-Right
    int rectId = addObject(rect);

    Shape circle = { .type = SHAPE_CIRCLE, .draw_char = '*' };
    circle.data.circle = (CircleData){ 20, 27, 8 }; // Bottom-Left
    int circleId = addObject(circle);

    Shape tri = { .type = SHAPE_TRIANGLE, .draw_char = '*' };
    tri.data.triangle = (TriangleData){ 60, 25, 50, 35, 70, 35 }; // Bottom-Right
    int triId = addObject(tri);

    printf("\n--- Canvas after adding shapes ---\n");
    printf("Line ID: %d\n", lineId);
    printf("Rectangle ID: %d\n", rectId);
    printf("Circle ID: %d\n", circleId);
    printf("Triangle ID: %d\n", triId);
    displayPicture();

    // Modify a shape
    printf("\n--- Modifying the circle ---\n");
    Shape* c = getObject(circleId);
    if (c != NULL) {
        c->data.circle.radius = 12; // Increase radius
    }
    displayPicture();

    // Delete shape(s)
    int num_del;
    printf("\nHow many objects do you want to delete? ");
    if (scanf("%d", &num_del) == 1) {
        for (int i = 0; i < num_del; i++) {
            int delete_id;
            printf("Enter the ID of shape to delete: ");
            if (scanf("%d", &delete_id) == 1) {
                printf("--- Deleting shape %d ---\n", delete_id);
                deleteObject(delete_id);
            } else {
                int c; while ((c = getchar()) != '\n' && c != EOF); // Clear buffer
                i--;
            }
        }
        displayPicture();
    }

    return 0;
}
