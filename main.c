/*
Блокируем мьютексы (это важно) при отрисовке
Один поток считывает в бесконечном цикле
Второй поток в бесконеччном цикле принимает сообщения

TODO:
    (*) Добавить возможность изменения размера окна

    (X) Функция close_client при завершении
    (X) КОММЕНТАРИИ!!
    (X) Логи чата
    (Х) Почитать про дизайн окон
    (X) Возможность стирать через backspace по символу
    (X) Убрать возможность пустого имени
    (X) Прокрутку окон ввода/вывода с учетом отрисовки полей
    (O) Добавить шифрование -- бессмысленно в связи с использованием широкого вещания
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "messages.h"

#define MAX_NAME_LEN 32

/**
 * The struct "cords" represents coordinates with x and y values, as well as maximum x and y values.
 */
struct cords
{
    int x, y, max_x, max_y;
};

WINDOW *input_win, *output_win;
pthread_mutex_t mutex;

char ip[16];
char name[MAX_NAME_LEN];
int port;

void sigchld_handler(int signum)
{
    _exit(-1);
}

/**
 * The function initializes two sub windows within the main window, sets up various settings and
 * attributes for the windows, and enables scrolling for the windows.
 */
void init_windows()
{
    // Receive all signals as characters, no buffering input
    raw();

    initscr();

    keypad(stdscr, TRUE);

    // Don't show uncooked input
    noecho();

    // Refresh stdscr before sub windows
    refresh();

    // No new lines when enter key is pressed
    nonl();

    // Set up colours
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);

    bkgd(COLOR_PAIR(1));
    refresh();

    // Get terminal window dimensions
    int height, htop, hbot, width;
    getmaxyx(stdscr, height, width);
    htop = ((height / 3) * 2) - 2;
    hbot = (height / 3) - 2;
    width -= 2;

    // Create output_win sub window
    output_win = newwin(htop, width, 1, 1);
    box(output_win, 0, 0);
    wrefresh(output_win);

    // Create bottom sub window
    input_win = newwin(hbot, width, htop + 3, 1);

    box(input_win, 0, 0);
    wmove(input_win, 1, 1);
    wrefresh(input_win);

    scrollok(input_win, TRUE);
    scrollok(output_win, TRUE);
}

/**
 * The function `print_to_window` prints a message to a window and automatically scrolls the window if
 * the cursor is in the last line.
 *
 * @param window The `window` parameter is a pointer to a `WINDOW` structure, which represents a window
 * on the screen. It is used to specify the window where the message will be printed.
 * @param message A pointer to a constant character array that contains the message to be printed to
 * the window.
 */
void print_to_window(WINDOW *window, const char *message)
{
    struct cords cords;
    getyx(window, cords.y, cords.x);
    getmaxyx(window, cords.max_y, cords.max_x);
    cords.max_x -= 1;
    cords.max_y -= 1;
    // If cursor on the last line scroll up and clear the last line for new info
    if (cords.y >= cords.max_y - 1)
    {
        scroll(window);
        wmove(window, cords.max_y - 1, 1);
        wclrtoeol(window);
    }
    else
    {
        wmove(window, cords.y + 1, 1);
    }

    wprintw(window, "%s", message);
    box(window, 0, 0);
    wrefresh(window);
}

/**
 * The clear_window function clears the contents of a given window by filling it with empty spaces.
 *
 * @param win The "win" parameter is a pointer to a WINDOW structure, which represents a window in the
 * ncurses library.
 */
void clear_window(WINDOW *win)
{
    struct cords cords;
    getmaxyx(win, cords.y, cords.x);
    cords.y -= 2;
    cords.x -= 2;

    char line_buff[cords.x + 1];
    line_buff[cords.x] = 0;
    memset(line_buff, ' ', cords.x);

    for (int i = 1; i <= cords.y; i++)
    {
        mvwprintw(win, i, 1, line_buff);
    }

    wrefresh(win);
}

/**
 * The function `input_thread` handles user input in a chat application, allowing the user to send
 * messages by pressing enter and handling special key inputs like backspace.
 *
 * @param arg The "arg" parameter is a void pointer that can be used to pass any additional arguments
 * to the input_thread function. In this case, it is not used and can be ignored.
 *
 * @return a `void` pointer, which is `NULL`.
 */
void *input_thread(void *arg)
{
    // Ignore resize signals to prevent crashing
    signal(SIGWINCH, SIG_IGN);

    char input[MAX_BUFFER_SIZE];
    int input_len = 0;
    int c;
    struct cords cords;
    getmaxyx(input_win, cords.max_y, cords.max_x);

    cords.max_x -= 1;
    cords.max_y -= 1;
    cords.y = 1;
    cords.x = 1;
    wmove(input_win, cords.y, cords.x);

    if (signal(SIGCHLD, &sigchld_handler) == SIG_ERR)
    {
        perror("client_loop(): failed to set SIGCHLD handler");
        exit(-1);
    }

    while ((c = wgetch(input_win)) != SIGQUIT)
    {
        // pthread_mutex_lock(&mutex);
        // refresh();
        // pthread_mutex_unlock(&mutex);

        if (c == KEY_ENTER || c == '\r')
        {
            pthread_mutex_lock(&mutex);
            input[input_len] = 0;
            send_message(name, ip, port, input);
            memset(input, 0, MAX_BUFFER_SIZE);
            input_len = 0;

            clear_window(input_win);
            wrefresh(output_win);
            // Reset cursor cords after cleaning window
            cords.x = 1;
            cords.y = 1;
            wmove(input_win, cords.y, cords.x);

            pthread_mutex_unlock(&mutex);
        }
        else if (c == KEY_BACKSPACE || c == 127)
        {
            if (input_len == 0)
                continue;
            pthread_mutex_lock(&mutex);
            input[input_len] = 0;
            input_len--;
            cords.x--;
            // If no chars to erase on current line
            if (cords.x == 0)
            {
                // If current line is the only one
                if (cords.y == 1)
                {
                    cords.x = 1;
                }
                else
                {
                    cords.y--;
                    cords.x = cords.max_x - 2;
                }
            }
            mvwdelch(input_win, cords.y, cords.x);
            // Erase old box's line
            wclrtoeol(input_win);

            box(input_win, 0, 0);
            pthread_mutex_unlock(&mutex);
        }

        else if (input_len < MAX_BUFFER_SIZE)
        {
            pthread_mutex_lock(&mutex);
            // Add current char to visible line
            waddch(input_win, c);
            c = (char)c;
            input[input_len] = c;
            input_len++;
            cords.x++;
            // If no space on current line
            if (cords.x > cords.max_x - 1)
            {
                // If current line is last line
                // Scroll up adn erase last line for new input
                if (cords.y > cords.max_y - 2)
                {
                    scroll(input_win);
                    cords.x = 1;
                    cords.y = cords.max_y - 1;
                    wmove(input_win, cords.y, 1);
                    wclrtobot(input_win);
                    // wmove(input_win, cords.y+1, 1);
                    // wclrtoeol(input_win);
                }
                else
                {
                    cords.x = 1;
                    cords.y += 1;
                }
                wmove(input_win, cords.y, cords.x);
            }
            box(input_win, 0, 0);
            wrefresh(input_win);
            pthread_mutex_unlock(&mutex);
        }
        // Out of MAX_BUFFER_SIZE
        // Clear current input
        else
        {
            pthread_mutex_lock(&mutex);
            input[input_len] = 0;
            memset(input, 0, MAX_BUFFER_SIZE);
            input_len = 0;

            clear_window(input_win);
            wrefresh(output_win);
            cords.x = 1;
            cords.y = 1;
            wmove(input_win, cords.y, cords.x);

            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

/**
 * The function `output_thread` continuously accepts messages, locks a mutex, prints the message to a
 * window, adds the message to a log, clears the buffer, unlocks the mutex, and repeats.
 *
 * @param arg The "arg" parameter is a void pointer that can be used to pass any additional arguments
 * to the output_thread function. In this case, it is not being used and can be ignored.
 *
 * @return a `NULL` pointer.
 */
void *output_thread(void *arg)
{
    char buff[MAX_BUFFER_SIZE];
    while (1)
    {
        if (accept_message(buff))
        {
            pthread_mutex_lock(&mutex);
            print_to_window(output_win, buff);
            add_to_log(buff);
            memset(buff, 0, MAX_BUFFER_SIZE);
            pthread_mutex_unlock(&mutex);
        }
        // sleep(1);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t input_tid, output_tid;
    if (argc != 3)
    {
        printf("Usage: %s <IP address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    // Read port number and transform to int
    port = atoi(argv[2]);

    strcpy(ip, argv[1]);

    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    if (name[0] == '\n' || strchr(name, ' ') != NULL)
    {
        printf("Error: Name can't be empty or contain spaces\n");
        return EXIT_FAILURE;
    }
    name[strcspn(name, "\n")] = 0;

    // Init socket and all connection stuff
    if (init_client(port))
    {
        return EXIT_FAILURE;
    }

    // Init input and output windows, set up all major parametrs for ncurses
    init_windows();
    // echo();

    // Init mutex to controll output to windows
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&input_tid, NULL, input_thread, NULL);
    pthread_create(&output_tid, NULL, output_thread, NULL);

    pthread_join(input_tid, NULL);
    pthread_join(output_tid, NULL);

    pthread_mutex_destroy(&mutex);

    delwin(input_win);
    delwin(output_win);

    endwin();
    close_client();

    return EXIT_SUCCESS;
}
