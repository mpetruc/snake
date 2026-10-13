// typing.cc -- a touch typing tutor for the terminal
//
// Screen (an 80x30 terminal is assumed; a shorter one is packed tighter):
//   row 0      stats bar: accuracy (left), WPM (center), time (right)
//   rows 1-9   QWERTY keyboard, upper case letters plus punctuation; the
//              key currently held down is shown inverted with one space
//              of padding on each side
//   separator  a line of dashes
//   text       one line of the practice lesson; the next letter to type
//              is inverted and only advances on a correct keypress
//              (anything else beeps)
//
// Build: g++ typing.cc -lcurses -o typing
// Run:   ./typing lessons/lesson1.txt   (ESC quits)

#include <curses.h> // required
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

WINDOW *wnd;

// ---- keyboard ----------------------------------------------------------
// one entry per physical key: the character it produces, which keyboard
// row it sits in and where in that row; wide keys (the space bar) are
// drawn as a labeled bar instead of a single letter
typedef struct {
  char ch;
  int row;    // 0 number, 1 q, 2 a, 3 z, 4 space bar
  int col;    // screen column of the key
  bool wide;  // true for the space bar
} Key;

Key keys[] = {
  // number row
  {'1',0,10,false},{'2',0,15,false},{'3',0,20,false},{'4',0,25,false},
  {'5',0,30,false},{'6',0,35,false},{'7',0,40,false},{'8',0,45,false},
  {'9',0,50,false},{'0',0,55,false},{'-',0,60,false},{'=',0,65,false},
  // top letter row
  {'q',1,13,false},{'w',1,18,false},{'e',1,23,false},{'r',1,28,false},
  {'t',1,33,false},{'y',1,38,false},{'u',1,43,false},{'i',1,48,false},
  {'o',1,53,false},{'p',1,58,false},{'[',1,63,false},{']',1,68,false},
  {'\\',1,73,false},
  // home row
  {'a',2,15,false},{'s',2,20,false},{'d',2,25,false},{'f',2,30,false},
  {'g',2,35,false},{'h',2,40,false},{'j',2,45,false},{'k',2,50,false},
  {'l',2,55,false},{';',2,60,false},{'\'',2,65,false},
  // bottom letter row
  {'z',3,17,false},{'x',3,22,false},{'c',3,27,false},{'v',3,32,false},
  {'b',3,37,false},{'n',3,42,false},{'m',3,47,false},{',',3,52,false},
  {'.',3,57,false},{'/',3,62,false},
  // space bar
  {' ',4,34,true},
};

const int NKEYS = sizeof(keys) / sizeof(keys[0]);

// screen layout, set from the terminal size once known; the numbers suit
// an 80x30 xterm, a short screen packs everything tighter
int keytop = 1;   // screen row of the first keyboard row
int keystep = 2;  // screen rows between keyboard rows
int bartop = 9;   // screen row of the space bar
int septop = 14;  // screen row of the separator
int texttop = 16; // screen row of the practice line

// ---- program state ------------------------------------------------------
vector<string> lines;   // practice lines loaded from the lesson file
int line_idx = 0;       // which line is on screen
int pos = 0;            // next letter to type inside the current line
int total_typed = 0;    // keypresses counted so far
int correct = 0;        // of those, how many were right
time_t start_time;
Key *pressed = NULL;    // key currently shown inverted

void layout(int nrows) {
  if (nrows < 30) {
    keytop = 1;
    keystep = 1;
    bartop = 6;
    septop = 8;
    texttop = 10;
  }
}

int keyrow(const Key *k) {
  return k->row == 4 ? bartop : keytop + k->row * keystep;
}

void load_lines(const string& filename) {
  ifstream file(filename.c_str());
  string line;
  while (getline(file, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r') // CRLF files
      line.erase(line.size() - 1);
    lines.push_back(line);
  }
  file.close();
}

Key *find_key(int ch) {
  for (int i = 0; i < NKEYS; i++)
    if (toupper(keys[i].ch) == toupper(ch)) return &keys[i];
  return NULL;
}

// draw one key, with the letter inverted plus a space of padding on each
// side when it is the key currently being pressed
void draw_key(const Key *k, bool inv) {
  int r = keyrow(k);
  if (inv) attron(A_REVERSE);
  if (k->wide) {
    mvaddstr(r, k->col - 1, " SPACE ");
  } else {
    mvaddch(r, k->col - 1, ' ');            // padding left
    mvaddch(r, k->col,     toupper(k->ch)); // the letter
    mvaddch(r, k->col + 1, ' ');            // padding right
  }
  if (inv) attroff(A_REVERSE);
}

void draw_keyboard() {
  for (int i = 0; i < NKEYS; i++) draw_key(&keys[i], false);
}

void draw_separator() {
  int ncols = getmaxx(wnd);
  for (int c = 0; c < ncols; c++) mvaddch(septop, c, '-');
}

// the practice line, with the next letter to type inverted
void draw_text() {
  int ncols = getmaxx(wnd);
  move(texttop, 0);
  clrtoeol();
  if (line_idx >= (int)lines.size()) {
    mvaddstr(texttop, (ncols - 15) / 2, "Lesson complete!");
    return;
  }
  string line = lines[line_idx];
  int x = (ncols - (int)line.size()) / 2;
  for (int i = 0; i < (int)line.size(); i++) {
    if (i == pos) attron(A_REVERSE);
    mvaddch(texttop, x + i, line[i]);
    if (i == pos) attroff(A_REVERSE);
  }
}

void draw_stats() {
  int ncols = getmaxx(wnd);
  move(0, 0);
  clrtoeol();
  char buf[64];
  int pct = total_typed ? (100 * correct) / total_typed : 100;
  sprintf(buf, "Accuracy: %3d%%", pct);
  mvaddstr(0, 0, buf);

  int elapsed = (int)(time(NULL) - start_time);
  sprintf(buf, "Time: %02d:%02d", elapsed / 60, elapsed % 60);
  mvaddstr(0, ncols - (int)strlen(buf), buf);

  double minutes = elapsed > 0 ? (double)elapsed / 60.0 : 0.0;
  int wpm = minutes > 0 ? (int)((total_typed / 5.0) / minutes) : 0;
  sprintf(buf, "WPM: %d", wpm);
  mvaddstr(0, (ncols - (int)strlen(buf)) / 2, buf);
}

int main(int argc, char **argv) {
  string filename = "lessons/lesson1.txt";
  if (argc > 1) filename = argv[1];
  load_lines(filename);
  if (lines.empty()) {
    fprintf(stderr, "no practice text in %s\n", filename.c_str());
    return 1;
  }

  int nrows, ncols;
  wnd = initscr();
  cbreak();
  halfdelay(1);
  noecho();
  curs_set(0);
  getmaxyx(wnd, nrows, ncols);
  if (nrows < 20 || ncols < 60) {
    endwin();
    fprintf(stderr, "need a terminal of at least 60x20\n");
    return 1;
  }
  layout(nrows);
  start_time = time(NULL);
  clear();

  draw_keyboard();
  draw_separator();
  draw_text();
  draw_stats();
  mvaddstr(nrows - 1, (ncols - 17) / 2, "Press ESC to quit");
  refresh();

  int ch;
  while (1) {
    ch = wgetch(wnd);
    if (ch == ERR) { // no key in this tenth of a second
      draw_stats();
      refresh();
      continue;
    }
    if (ch == 27) break; // ESC quits

    // show the key being pressed on the keyboard
    Key *k = find_key(ch);
    if (k != pressed) {
      if (pressed) draw_key(pressed, false);
      pressed = k;
      if (pressed) draw_key(pressed, true);
    }

    // advance only on the correct letter, beep otherwise
    bool ok = false;
    if (line_idx < (int)lines.size()) {
      string line = lines[line_idx];
      if (pos < (int)line.size() && toupper(ch) == toupper(line[pos])) {
        ok = true;
        pos++;
        if (pos >= (int)line.size()) { // this line is done, next one
          line_idx++;
          pos = 0;
        }
      }
    }
    if (ok) correct++;
    total_typed++;
    if (!ok) beep();

    draw_text();
    draw_stats();
    refresh();
  }
  endwin(); // restore the original terminal and leave
  return 0;
}
