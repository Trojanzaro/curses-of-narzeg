#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct chr_struct
{
	WINDOW *win;
	int hp;		// hit points
	int str;	// strength modifier
	int ac;		// armour class
	int y;		// y coordinates
	int x;		// x coordinates
	int gold;	// gold
	char *name; // name (TODO: implement~ name, save, load)
} CHARACTER;

typedef struct col_tile
{
	int y;
	int x;
	int ac;
	int hp;
	int type; // 42 for chests, 52 for monsters, 0 for nothing
	int gold;
} TILE;

TILE **map;
TILE obj_index[20];
char e_stat[] = "                                                 "; // 49 spaces

WINDOW *create_newwin(int height, int width, int starty, int startx);
WINDOW *create_command_window(WINDOW *main_win, CHARACTER *ch);
CHARACTER *new_character(WINDOW *win);
TILE create_object(int type, int gold, int y, int x);

void move_character(CHARACTER *ch, int y, int x);
bool in_bounds(CHARACTER *mc);
void reload_stats(WINDOW *loacal_win, CHARACTER *ch);
int check_map(CHARACTER *ch);
void reload_tiles(WINDOW *local_win);
void render_character(CHARACTER *chr);
void clear_character(CHARACTER *chr);


int main(int argc, char *argv[])
{
	srand(time(NULL));
	WINDOW *main_win; //main window, where character moves and stuff exists
	WINDOW *cmd_win;  //command window, where stats lie and commands are shown
	CHARACTER *mc;
	int ch, i;
	int chr_x, chr_y;
	char temp[10];

	initscr();
	cbreak();			  /* Line buffering disabled, Pass on everty thing to me 		*/
	keypad(stdscr, TRUE); /* I need that nifty F1 	*/
	noecho();
	nodelay(stdscr, TRUE);

	// INITIALIZE MAP
	map = (TILE **)malloc(sizeof(TILE *) * LINES);
	for (i = 0; i < LINES; i++)
		map[i] = (TILE *)malloc(sizeof(TILE) * COLS);
	refresh();

	//Create the main window
	main_win = create_newwin(LINES, COLS, 0, 0);

	// ADD some random objects
	obj_index[0] = create_object(52, 5, 15, 20);
	obj_index[1] = create_object(42, 60, 10, 50);

	//Create main character
	mc = new_character(main_win);
	chr_x = mc->x;
	chr_y = mc->y;

	//Create the command window
	cmd_win = create_command_window(main_win, mc);

	while ((ch = getch()) != KEY_F(1))
	{
		switch (ch)
		{
		case KEY_UP:
			if (in_bounds(mc))
				move_character(mc, --chr_y, chr_x);
			else
				move_character(mc, ++chr_y, chr_x);
			break;
		case KEY_DOWN:
			if (in_bounds(mc))
				move_character(mc, ++chr_y, chr_x);
			else
				move_character(mc, --chr_y, chr_x);
			break;
		case KEY_LEFT:
			if (in_bounds(mc))
				move_character(mc, chr_y, --chr_x);
			else
				move_character(mc, chr_y, ++chr_x);
			break;
		case KEY_RIGHT:
			if (in_bounds(mc))
				move_character(mc, chr_y, ++chr_x);
			else
				move_character(mc, chr_y, --chr_x);
			break;
		case 'e':
		case 'E':
			strcpy(e_stat, "                                          ");
			reload_stats(cmd_win, mc);
			if (check_map(mc) == 42)
			{
				//if it's a chest open and get all the gold (TODO: implement locked chests)
				mc->gold += map[mc->y][mc->x].gold;
				strcpy(e_stat, "Chess opened! Received: ");
				sprintf(temp, "%d", map[mc->y][mc->x].gold);
				map[mc->y][mc->x].gold = 0;
				strcat(e_stat, temp);
				strcat(e_stat, " Gold!");
			}
			else if (check_map(mc) == 52)
			{ //if monster is here attack it
				if (map[mc->y][mc->x].hp <= 0)
				{
					strcpy(e_stat, "Monster Dead! Looting remaining gold");
					mc->gold += map[mc->y][mc->x].gold;
					map[mc->y][mc->x].gold = 0;
				}
				else
				{
					short pd20 = (rand() % 20) + mc->str; //'p' prefix stands for player
					short md20 = (rand() % 20) + mc->str; //'m' prefix stands for monster
					char pattack_roll[5];
					char mattack_roll[5];
					strcpy(e_stat, "Attack:: ");


					////////////////////
					//// Player initiative
					////////////////////
					if (pd20 >= map[mc->y][mc->x].ac)
					{
						int d8 = (rand() % 8) + mc->str;
						map[mc->y][mc->x].hp -= d8;
						strcat(e_stat, "P-> HIT! Dmg: ");
						sprintf(pattack_roll, "%d", d8);
						strcat(e_stat, pattack_roll);
					}
					else
					{
						strcat(e_stat, "P-> MISS!");
					}

					////////////////////
					//// Monster initiative
					////////////////////
					if (md20 >= mc->ac)
					{
						int d8 = (rand() % 8);
						mc->hp -= d8;
						strcat(e_stat, " M-> HIT! Dmg: ");
						sprintf(mattack_roll, "%d", d8);
						strcat(e_stat, mattack_roll);
					}
					else
					{
						strcat(e_stat, " M-> MISS!");
					}

				}
			}
			else
			{
			}
			break;
		}
		wrefresh(cmd_win);
		wrefresh(main_win);

		reload_stats(cmd_win, mc);
		reload_tiles(main_win);
		//coll_cetect(mc);
		refresh();
	}

	endwin(); /* End curses mode		  */
	return 0;
}

void reload_stats(WINDOW *local_win, CHARACTER *ch)
{
	int x = getmaxx(local_win);
	int y = getmaxy(local_win);
	//MC stats
	mvwprintw(local_win, (y - y) + 1, 1, "HP = %d   ", ch->hp);
	mvwprintw(local_win, (y - y) + 2, 1, "STR = %d   ", ch->str);
	mvwprintw(local_win, (y - y) + 3, 1, "AC = %d   ", ch->ac);
	////////////
	mvwprintw(local_win, (y - y) + 8, 1, "GOLD = %d   ", ch->gold);

	//Commands
	mvwprintw(local_win, (y - y) + 1, 20, "E = INTERACT");
	//////////////////
	//DEBUG SPACE
	//////////////////
	//mvwprintw(local_win, (y-y) + 2, 20, "Tile[%d][%d]: %d", ch->y, ch->x, map[ch->y][ch->x]);

	mvwprintw(local_win, (y - y) + 8, 20, "F1 = Exit Game");

	//Interact status
	mvwprintw(local_win, (y - y) + 3, 50, "%s", e_stat);
	wrefresh(local_win);
}

WINDOW *create_newwin(int height, int width, int starty, int startx)
{
	WINDOW *local_win;
	local_win = newwin(height, width, starty, startx);
	wborder(local_win, ACS_CKBOARD, ACS_CKBOARD, ACS_CKBOARD, ACS_CKBOARD, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	wrefresh(local_win);
	return local_win;
}

WINDOW *create_command_window(WINDOW *main_win, CHARACTER *ch)
{
	WINDOW *local_win = subwin(main_win, 10, COLS - 2, LINES - 11, 1);
	int x = getmaxx(local_win);
	int y = getmaxy(local_win);
	box(local_win, 0, 0);
	reload_stats(local_win, ch);
	return local_win;
}

CHARACTER *new_character(WINDOW *win)
{
	CHARACTER *chr = (CHARACTER *)malloc(sizeof(CHARACTER)); //allocate memory
	//initialize stats
	chr->gold = 0;
	chr->hp = 100;
	chr->str = 1;
	chr->ac = 15;
	chr->name = "Player1";
	chr->x = COLS / 2;
	chr->y = LINES / 2;
	chr->win = win;
	render_character(chr);
	return chr;
}

TILE create_object(int type, int gold, int y, int x)
{
	TILE t;
	t.type = type;
	t.gold = gold;
	t.ac = 15;
	t.hp = 20;
	t.y = y;
	t.x = x;
	map[y][x] = t;
	return t;
}

bool in_bounds(CHARACTER *mc)
{
	if ((mc->x < COLS - 2 && mc->y < LINES - 12) // Max bounds of main window and command window
		&&
		(mc->x > 1 && mc->y > 1))
		return true;
	else
		return false;
}

void move_character(CHARACTER *chr, int y, int x)
{
	clear_character(chr);
	chr->x = x;
	chr->y = y;
	render_character(chr);
}

int check_map(CHARACTER *ch)
{
	TILE t = map[ch->y][ch->x];
	return t.type;
}

void reload_tiles(WINDOW *local_win)
{
	int i = 0;
	for (i = 0; i < 5; i++)
		if (obj_index[i].type == 52)
			mvwaddch(local_win, obj_index[i].y, obj_index[i].x, 'M');
		else if (obj_index[i].type == 42)
			mvwaddch(local_win, obj_index[i].y, obj_index[i].x, '$');
}

void render_character(CHARACTER *chr) 
{
	mvwaddch(chr->win, chr->y-1, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y-1, chr->x, '#');
	mvwaddch(chr->win, chr->y-1, chr->x+1, ' ');
	mvwaddch(chr->win, chr->y, chr->x-1, '=');
	mvwaddch(chr->win, chr->y, chr->x, '@');
	mvwaddch(chr->win, chr->y, chr->x+1, '=');
	mvwaddch(chr->win, chr->y+1, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y+1, chr->x, '|');
	mvwaddch(chr->win, chr->y+1, chr->x+1, ' ');
	mvwaddch(chr->win, chr->y+2, chr->x-1, '/');
	mvwaddch(chr->win, chr->y+2, chr->x, ' ');
	mvwaddch(chr->win, chr->y+2, chr->x+1, '\\');
	wrefresh(chr->win);
}


void clear_character(CHARACTER *chr) 
{
	mvwaddch(chr->win, chr->y-1, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y-1, chr->x, ' ');
	mvwaddch(chr->win, chr->y-1, chr->x+1, ' ');
	mvwaddch(chr->win, chr->y, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y, chr->x, ' ');
	mvwaddch(chr->win, chr->y, chr->x+1, ' ');
	mvwaddch(chr->win, chr->y+1, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y+1, chr->x, ' ');
	mvwaddch(chr->win, chr->y+1, chr->x+1, ' ');
	mvwaddch(chr->win, chr->y+2, chr->x-1, ' ');
	mvwaddch(chr->win, chr->y+2, chr->x, ' ');
	mvwaddch(chr->win, chr->y+2, chr->x+1, ' ');
	wrefresh(chr->win);
}