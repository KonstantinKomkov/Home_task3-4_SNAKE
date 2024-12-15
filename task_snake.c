#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses/ncurses.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#define MIN_Y  2
#define CONTROLS 3
enum {LEFT=1, UP, RIGHT, DOWN, STOP_GAME=KEY_F(10)};
enum {MAX_TAIL_SIZE=100, START_TAIL_SIZE=6, MAX_FOOD_SIZE=20, FOOD_EXPIRE_SECONDS=10};

// Здесь храним коды управления змейкой
struct control_buttons
{
    int down;
    int up;
    int left;
    int right;
}control_buttons;

struct control_buttons default_controls [CONTROLS] = {{KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
                                                    {'S', 'W', 'A', 'D'}, {'s', 'w', 'a', 'd'}};

/*
 Голова змейки содержит в себе
 x,y - координаты текущей позиции
 direction - направление движения
 tsize - размер хвоста
 *tail -  ссылка на хвост
 */
typedef struct snake_t
{
    int x;
    int y;
    int direction;
    size_t tsize;
    struct tail_t *tail;
    struct control_buttons controls[CONTROLS];
} snake_t;

/*
 Хвост это массив состоящий из координат x,y
 */
typedef struct tail_t
{
    int x;
    int y;
} tail_t;

void initTail(struct tail_t t[], size_t size)
{
    struct tail_t init_t = {0, 0};
    for(size_t i = 0; i < size; i++)
    {
        t[i] = init_t;
    }
}

void initHead(struct snake_t *head, int x, int y)
{
    head->x = x;
    head->y = y;
    head->direction = RIGHT;
}

void initSnake(snake_t *head, size_t size, int x, int y)
{
    tail_t*  tail  = (tail_t*) malloc(MAX_TAIL_SIZE*sizeof(tail_t));
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head, x, y);
    head->tail = tail; // прикрепляем к голове хвост
    head->tsize = size + 1;
    for(int i = 0; i < CONTROLS; i++)
    {
        head->controls[i] = default_controls[i];
    }
}

/*
 Движение головы с учетом текущего направления движения
 */
void go(struct snake_t *head)
{
    char ch = '@';
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x); // macro - размер терминала
    mvprintw(head->y, head->x, " "); // очищаем один символ
    switch (head->direction)
    {
        case LEFT:
            if(head->x <= 1) // Циклическое движение, чтобы не
// уходить за пределы экрана
                head->x = max_x - 1;
            mvprintw(head->y, --(head->x), "%c", ch);
        break;
        case RIGHT:
            if(head->x >= max_x - 2)
                head->x = 0;
            mvprintw(head->y, ++(head->x), "%c", ch);
        break;
        case UP:
            if(head->y <= 2)
                head->y = max_y - 1;
            mvprintw(--(head->y), head->x, "%c", ch);
        break;
        case DOWN:
            if(head->y >= max_y - 2)
                head->y = 1;
            mvprintw(++(head->y), head->x, "%c", ch);
        break;
        default:
        break;
    }
    refresh();
}

void changeDirection(struct snake_t* snake, const int32_t key)
{
    for(int i = 0; i < CONTROLS; i++)
    {
        if(key == snake->controls[i].down)
            snake->direction = DOWN;
        else if(key == snake->controls[i].up)
            snake->direction = UP;
        else if(key == snake->controls[i].right)
            snake->direction = RIGHT;
        else if(key == snake->controls[i].left)
            snake->direction = LEFT;
    }
}

/*
 Движение хвоста с учетом движения головы
 */
void goTail(struct snake_t *head)
{
    char ch = '*';
    mvprintw(head->tail[head->tsize-1].y, head->tail[head->tsize-1].x, " ");
    for(size_t i = head->tsize-1; i>0; i--)
    {
        head->tail[i] = head->tail[i-1];
        if(head->tail[i].y || head->tail[i].x)
            mvprintw(head->tail[i].y, head->tail[i].x, "%c", ch);          
    }
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;
}

void limit_play_area (void) // Границы игрового поля
{
    char ch = '#';
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    for(int i = 0; i < max_x; i++)
    {
        mvprintw(1, i, "%c", ch);
        mvprintw(max_y - 1, i, "%c", ch);
    }
    for(int i = 1; i < max_y; i++)
    {
        mvprintw(i, max_x - 1, "%c", ch);
        mvprintw(i, 0, "%c", ch);
    }
}

int end_game (struct snake_t *head) // Конец игры
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    for(int i = 1; i < head->tsize; i++)
    {
        if(head->tail[i].y == head->y && head->tail[i].x == head->x)
        {
            mvprintw(max_y/2, (max_x/2)-5, "GAME OVER");
            mvprintw((max_y/2)+1, (max_x/2)-10, "Press 'F10' for EXIT");
            return 1;
        }
    }
    return 0;
}

int checkDirection(snake_t* snake, int32_t key) // Проверка корректности выбранного направления
{
    for(int i = 0; i < CONTROLS; i++)
    {
        if(snake->direction == UP && key == snake->controls[i].down)
            return 1;
        else if(snake->direction == DOWN && key == snake->controls[i].up)
            return 1;
        else if(snake->direction == LEFT && key == snake->controls[i].right)
            return 1;
        else if(snake->direction == RIGHT && key == snake->controls[i].left)
            return 1;
    }
    return 0;
}

int main()
{
    snake_t* snake = (snake_t*)malloc(sizeof(snake_t));
    initSnake(snake,START_TAIL_SIZE,10,10);
    initscr();
    keypad(stdscr, TRUE); // Включаем F1, F2, стрелки и т.д.
    raw();                // Отключаем line buffering
    noecho();            // Отключаем echo() режим при вызове getch
    curs_set(FALSE);    // Отключаем курсор
    mvprintw(0, 1,"Use arrows for control. Press 'F10' for EXIT. Your score is");
    timeout(0);    // Отключаем таймаут после нажатия клавиши в цикле
    int key_pressed = 0;
    limit_play_area(); // Видимые границы игрового поля
    while( key_pressed != STOP_GAME )
    {
        key_pressed = getch(); // Считываем клавишу
        if(end_game(snake) != 1) // Проверка на столкновение с хвостом
        {
            go(snake);
            goTail(snake);
            timeout(100); // Задержка при отрисовке
            if(checkDirection(snake, key_pressed) == 0) // Проверка корректности выбранного направления
            {
                changeDirection(snake, key_pressed);
            }
        }
    }
    free(snake->tail);
    free(snake);
    endwin(); // Завершаем режим curses mod
    return 0;
}