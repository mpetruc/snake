Terminal Typing Tutor

What it does: help terminal users learn on improve touch typing. 
Description: assuming a standard xterm 80 columns by 30 rows at least, on the top half show a keyboard made up with regular upper case letters plus punctuation, using a QWERTY layout with enough spacing between keys. Whenever a key is depressed on the physical keyboard invert the letter on the screen with padding of one space on the left and right. On the bottom half of the screen show a practice text to type that is loaded from a text file. Display the text one line at a time and advance the cursor only when the correct letter is typed, and beep otherwise. ESC to quit. Also show the elapsed time in the right top column and percentage of correctly typed letters on the left (typing accuracy) and average typing speed (words/minute). Use the practice text files from the 'lessons' folder - they were starting easy with just a few different keys on the middle row so one doesn't need to move the fingers, but then move on to words with more and more letters on the other rows.

Constrains: Use C and the ncurses library. Adopt the programing style from @snake.cc. Start on a new git branch ('ttt'). Commit often. 

The final program should compile with `g++ typing.cc -lcurses -o typing`

To practice:

`./typing lessons/lesson1.txt`

