#include <stdio.h>
#include <Windows.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include "shortcut.h"

unsigned int* extend_keylist(unsigned int keys[], unsigned int l, unsigned int n_key);
bool all_pressed(unsigned int keys[], unsigned int l);

const short int high_bitmask = 1 << (sizeof(short int) * CHAR_BIT);

int main(int argc, char* argv[])
{
	// reduce thread priority for performance
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	// delay in ms between polls
	unsigned short int delay = 150;

	// config file path
	char path[256];
	strcpy(path, getenv("USERPROFILE"));
	strcat(&path, "/shortcuts.kse");

	// process switches
	for (unsigned short int i = 1; i < argc; i++)
	{
		char s1, s2, cont[256];
		printf("%s\n", argv[i]);
		if (sscanf(argv[i], "%c%c%s", &s1, &s2, cont) != EOF && s1 == '-')
		{
			switch (s2)
			{
			case 'd':
				delay = atoi(cont);
				break;
			}
		}
		else
			strcpy(path, argv[i]);
	}

	// read shortcuts
	shortcut* shortcuts = NULL;
	unsigned int* action_keys = NULL;
	unsigned int shortcut_ct = 0,
		ak_ct = 0;
	FILE* config = fopen(path, "r");
	char line[256];
	while (fgets(line, 256, config) != NULL)
	{
		if (line[0] == '#') // skip commented lines
			continue;

		else if (line[0] == 'A')
		{
			char temp[4];
			strncpy(temp, &line[2], 3);
			extend_keylist(action_keys, ak_ct, atoi(temp));
		}

		else
		{
			shortcut nshortcut;
			char temp[5];

			strncpy(temp, line, 4);
			temp[4] = '\0';
			nshortcut.ccode = atoi(temp);

			strncpy(temp, &line[5], 1);
			temp[1] = '\0';
			nshortcut.shift = atoi(temp);

			nshortcut.key_ct = 0;
			strncpy(temp, &line[7], 3);
			for (unsigned short int i = 11; !strchr(temp, '.'); i += 4)
			{
				nshortcut.keys = extend_keylist(nshortcut.keys, nshortcut.key_ct, atoi(temp));
				strncpy(temp, &line[i], 3);
			}

			shortcuts = append_to_array(shortcuts, shortcut_ct, &nshortcut);
			shortcut_ct++;
		}
	}
	fclose(config);

	// hide console
	HWND console = GetConsoleWindow();
	ShowWindow(console, SW_HIDE);

	// main loop
	bool active = false; // to track whether window is in foreground
	HWND last; // last active window (for switching back when deactivating shortcut)
	INPUT ip;
	time_t begin = time(0);
	while (true)
	{
		// check for keys
		if (GetKeyState(VK_CONTROL) & GetKeyState(VK_MENU) & high_bitmask)
		{
			// if window isn't in foreground, move it to foreground so others are blocked
			if (!active)
			{
				last = GetForegroundWindow();

				EnableWindow(console, true);
				SetForegroundWindow(console);
				//SetActiveWindow(console); // what's the difference?

				active = true;
			}

			bool shift = GetKeyState(VK_SHIFT) ^ GetKeyState(VK_CAPITAL);

			for (unsigned short int i = 0; i < shortcut_ct; i++) // go through shortcuts, check if any are active
				if (shortcuts[i].shift == shift && all_pressed(shortcuts[i].keys, shortcuts[i].key_ct))
				{
					// make input struct
					ip.type = INPUT_KEYBOARD;
					ip.ki.dwFlags = KEYEVENTF_UNICODE;
					ip.ki.wScan = shortcuts[i].ccode;
					ip.ki.wVk = 0;
					ip.ki.dwExtraInfo = 0;

					// send input
					EnableWindow(console, false);
					SetForegroundWindow(last);
					SendInput(1, &ip, sizeof(INPUT));
					EnableWindow(console, true);
					SetForegroundWindow(last);

					// sleep for 100ms less than delay
					Sleep(delay - 100 + begin - time(0));

					// stop looking for more shortcuts
					break;
				}
		}
		else if (active)
		{
			EnableWindow(console, false);
			SetForegroundWindow(last);

			active = false;
		}

		// wait the remaining 100ms so the loop doesn't go nuts
		Sleep(100 + begin - time(0));
		begin = time(0);
	}

	return 1; // should not exit
}

struct shortcut* append_to_array(shortcut shortcuts[], unsigned int l, shortcut* new_element)
{
	shortcut* arr = malloc(sizeof(shortcut) * (l + 1));
	memcpy(arr, shortcuts, sizeof(shortcut) * l);
	arr[l] = *new_element;

	free(shortcuts);

	return arr;
}

unsigned int* extend_keylist(unsigned int keys[], unsigned int l, unsigned int n_key)
{
	unsigned int* arr = malloc(sizeof(unsigned int) * (l + 1));
	memcpy(arr, keys, sizeof(unsigned int) * l);
	arr[l] = n_key;

	free(keys);

	return arr;
}

bool all_pressed(unsigned int keys[], unsigned int l)
{
	for (unsigned int i = 0; i < l; i++) if (!GetKeyState(keys[i]) & high_bitmask)
		return false;

	return true;
}