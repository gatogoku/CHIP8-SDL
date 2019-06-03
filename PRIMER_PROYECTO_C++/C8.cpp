#include "pch.h"
#include "C8.h"
#include <string>
using namespace std;
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <random>
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <windows.h>
#include "resource.h"
#include <Commdlg.h>
#include <iostream> 
#include <windows.h> // WinApi header 
SDL_Window* window;
SDL_Renderer* renderer;
OPENFILENAME ofn;
char szFile[100];

void putpixel(SDL_Surface* surface, int x, int y, Uint32 pixel){
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to set */
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	switch (bpp) {
	case 1:*p = pixel;break;
	case 2:*(Uint16*)p = pixel;break;
	case 3:if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {p[0] = (pixel >> 16) & 0xff;p[1] = (pixel >> 8) & 0xff;p[2] = pixel & 0xff;}else {p[0] = pixel & 0xff;p[1] = (pixel >> 8) & 0xff;p[2] = (pixel >> 16) & 0xff;}break;
	case 4:*(Uint32*)p = pixel;break;
	}
}

Uint32 getpixel(SDL_Surface * surface, int x, int y){
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	switch (bpp) {
	case 1:return *p;break;
	case 2:return *(Uint16*)p;break;
	case 3:if (SDL_BYTEORDER == SDL_BIG_ENDIAN) { return p[0] << 16 | p[1] << 8 | p[2]; }else {return p[0] | p[1] << 8 | p[2] << 16;}break;
	case 4:return *(Uint32*)p;break;
	default:return 0;       /* shouldn't happen, but avoids warnings */
	}
}

int C8::SDL_GRAPHICS(){
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;window = SDL_CreateWindow("SDL CHIP-8 EMULATOR", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_SWSURFACE | SDL_WINDOW_RESIZABLE );//SDL_SWSURFACE | SDL_WINDOW_RESIZABLE );//
	SDL_SetWindowSize(window, 640, 320); renderer = SDL_CreateRenderer(window, -1, 0); SDL_RenderClear(renderer); SDL_RenderPresent(renderer); return 0;//SDL_Delay(2000);//SDL_Quit();	
}

string tobinary(int num){
	int  a[8]; int n = 0; int  i = 0; string res = "";
	for (i = 0; num > 0; i++) { (a[i] = num % 2)+=0 ; num = num / 2; }
	for (i = 7; i >= 0; i--) { if (a[i] == 1 ) {res+= "1";}else { res+="0"; }}
	return res;
}

int C8::DibujarChars() { 
	int mul = 40; int ind = 0; SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
	for (;;) {
		SDL_FillRect(screenSurface, NULL, RGB(0, 0, 0)); SDL_UpdateWindowSurface(window);
		for (int y = 0; y < (5); y++) {string b = tobinary(CHIP8_CHARS[y + ind]); for (int x = 0; x < (4); x++) {
				for (int c = 0; c < mul; c++) {for (int d = 0; d < mul; d++) {
					int pix = RGB(32 + rand() % (256 - 32), 32 + rand() % (256 - 32), 32 + rand() % (256 - 32));;if (b.substr(x, 1) == "1") { putpixel(screenSurface, 50 + x * mul + c, 50 + y * mul + d, pix); }// RGB(255, 1, 1));
					}}
			}}
		ind += 5;if (ind >= 79) { ind = 0; }SDL_RenderPresent(renderer); SDL_UpdateWindowSurface(window); std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	} return 0;
}

C8::C8(){
	C8::IniciarCPU(); C8::OpenRom();C8::SDL_GRAPHICS();//C8::DibujarChars();
	EMULATOR_RUNNING = true;
	while(EMULATOR_RUNNING) {FetchKeyBoard();PROCESS_CPU();}
}

C8::~C8() {}

int C8::OpenRom(){
	ZeroMemory(&ofn, sizeof(ofn));ofn.lStructSize = sizeof(ofn);ofn.hwndOwner = NULL;ofn.lpstrFile = szFile;//ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";ofn.nFilterIndex = 1;ofn.lpstrFileTitle = NULL;ofn.nMaxFileTitle = 0;ofn.lpstrInitialDir = NULL;ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	GetOpenFileName(&ofn);//MessageBox(NULL, ofn.lpstrFile, "File Name", MB_OK);
	FILE* fp = std::fopen(ofn.lpstrFile, "r");
		if (!fp) {std::perror("File opening failed");return EXIT_FAILURE;}
		int c;int index = 512;
		while ((c = std::getc(fp)) != EOF) { MEMORY[index] = (c);index += 1;}
		if (std::ferror(fp))std::puts("I/O error when reading");
		else if (std::feof(fp))std::puts("End of file reached successfully");
		std::fclose(fp);
	return 0;
}

int C8::IniciarCPU() {
	DELAY_TIMER = 0;SOUND_TIMER = 0;OPC = 0;I = 0;SP = 0;PC = 512;KEY_PRESSED = NULL;
	int ind = 0;
	for (int c = 0; c < 16; c++) {V[c] = 0; STACK[c] = 0; KEYS[c] = 0;}
	for (int c = 0; c < sizeof(MEMORY) / 2; c++) {MEMORY[ind] = 0; ind += 1;}
	ind = 0;
	for (int c = 0; c < 80; c++) {MEMORY[c] = CHIP8_CHARS[c]; }
	for (int c = 0; c < (64*32); c++) {GRAPHICS[c] = 0;}
	return 0;
}

int C8::PROCESS_CPU() {
		OPC = (MEMORY[PC] << 8) | MEMORY[PC + 1];
		unsigned short D = OPC & 0x000F; unsigned short X = OPC >> 8 & 15;unsigned short Y = OPC >> 4 & 15;
		unsigned short N = OPC >>12 & 15;unsigned short NN = OPC & 0x00FF;unsigned short NNN = OPC & 0x0FFF;
		unsigned short x = V[X];unsigned short y = V[Y];unsigned short pixel;
		SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
		switch (OPC & 0xF000) {
		case 0x0000:
			switch (OPC) {
			case  0x00E0:SDL_FillRect(screenSurface, NULL, RGB(0, 0, 0));for (int c = 0; c < (64 * 32); c++) GRAPHICS[c] = 0;DibujarSpritesChip8(); PC += 2;break;
			case  0x00EE:SP -= 1; PC = STACK[SP];  PC += 2;break;}
			break;
		case  0x1000:v = PC;PC = NNN;if (v == NNN) { PC += 2; }break;
		case  0x2000: STACK[SP] = PC; SP += 1; PC = NNN;break;
		case  0x3000:if (V[X] == NN) { PC += 4; } else { PC += 2; }break;
		case  0x4000:if (V[X] != NN) { PC += 4; } else { PC += 2; }break;
		case  0x5000:if (V[X] == V[Y]) { PC += 4; } else { PC += 2; }break;
		case  0x6000:V[X] = NN; PC += 2;break;
		case  0x7000:c = V[X] + NN;c &= 0xFF; V[X] = c; PC += 2; break;//	if (!(c < 256)) { c -= 256; }
		case  0x8000:
			switch (OPC & 0x000F) {
			case  0x0000:V[X] = V[Y]; PC += 2;break;
			case  0x0001:V[X] = V[X] | V[Y]; PC += 2;break;
			case  0x0002:V[X] = V[X] & V[Y]; PC += 2;break;
			case  0x0003:V[X] = V[X] ^ V[Y]; PC += 2;break;
			case  0x0004:
				c = V[X] + V[Y];if ((c > 256)) { V[15] = 1; V[X] = NN; }else { V[15] = 0; }V[X] = c;
			/*	V[(OPC & 0x0F00) >> 8] += V[(OPC & 0x00F0) >> 4];
				if (V[(OPC & 0x00F0) >> 4] > (0xFF - V[(OPC & 0x0F00) >> 8]))V[0xF] = 1; else { V[0xF] = 0; }*/PC += 2;break;
			case  0x0005:if (V[X] >= V[Y]) {V[15] = 1; V[X] = V[X] - V[Y];} else  {V[15] = 0; int value = ((V[X]) + 256) - (V[Y]); V[X] = value;}PC += 2;break;//corregida
			case  0x0006://corregida
				/*b = V[X] << 7;
				if (b == 1) { V[15] = 1; }
				else { V[15] = 0; }
				V[X] >>= 1;*/
				V[0xF] = V[X] & 0x1;V[X] >>= 1;PC += 2;break;
			case  0x0007:
				if (V[X] < V[Y]) { V[15] = 1; }else { V[15] = 0; }
				if (V[Y] >= V[X]) {V[15] = 1; V[X] = V[Y] - V[X];}else if (!(V[X] >= V[Y])) {V[15] = 0; int value = ((V[X]) + 256) - (V[Y]); V[X] = value;}PC += 2;break;
			case  0x000E:b = V[X] >> 7;if (b == 1) { V[15] = 1; }else { V[15] = 0; }V[X] >>= 1; PC += 2;break;}break;
		case  0x9000:if (V[X] != V[Y]) { PC += 4; } else { PC += 2; }break;
		case  0xA000:I = NNN; PC += 2;break;
		case  0xB000:PC = V[0] + NNN;break;
		case  0xC000:v = std::rand() * 255;V[X] = v; PC += 2;break;
		case  0xD000:
			V[0xF] = 0;
			for (int yline = 0; yline < D; yline++){pixel = MEMORY[I + yline];for (int xline = 0; xline < 8; xline++){
					if ((pixel & (0x80 >> xline)) != 0){if (GRAPHICS[(x + xline + ((y + yline) * 64))] == 1){V[0xF] = 1;}GRAPHICS[x + xline + ((y + yline) * 64)] ^= 1;}
				}}std::this_thread::sleep_for(std::chrono::milliseconds(16));PC += 2;DibujarSpritesChip8();break;
		case  0xE000:
			switch (OPC & 0x00FF) {
			case  0x009E:if (V[X] == KEY_PRESSED) { PC += 4; } else { PC += 2; }break;
			case  0x00A1:if (V[X] != KEY_PRESSED) { PC += 4; } else { PC += 2; }break;
			}break;
		case  0xF000:
			switch (OPC & 0x00FF) {
			case  0x0007:V[X] = DELAY_TIMER; PC += 2;break;
			case  0x000A:while (KEY_PRESSED == NULL) { FetchKeyBoard(); }if (KEY_PRESSED != NULL) { V[X] = KEY_PRESSED; }PC += 2;break;
			case  0x0015:DELAY_TIMER = V[X]; PC += 2;break;
			case  0x0018:SOUND_TIMER = V[X]; PC += 2;break;
			case  0x001E:
				//I += V[X]; I &= 0xFFF;
				if (I + V[(OPC & 0x0F00) >> 8] > 0xFFF) { V[0xF] = 1; }else { V[0xF] = 0; }; I += V[(OPC & 0x0F00) >> 8]; PC += 2; break;
			case  0x0029:
				switch (V[X]) {
				case 0:I = 0; break;case 1:I = 5; break;case 2:I = 10; break;case 3:I = 15; break;
				case 4:I = 20; break;case 5:I = 25; break;case 6:I = 30; break;case 7:I = 35; break;
				case 8:I = 40; break;case 9:I = 45; break;case 10:I = 50; break;case 11:I = 55; break;
				case 12:I = 60; break;case 13:I = 65; break;case 14:I = 70; break;case 15:I = 75; break;
				}PC += 2;//I = V[(OPC & 0x0F00) >> 8] * 0x5;
				break;
			case  0x0033:
				MEMORY[I] = V[X] / 100;               //  ' Store hundreds in I
				MEMORY[I + 1] = (V[X] / 10) % 10;  //   ' Tens in I + 1
				MEMORY[I + 2] = (V[X] % 100) % 10; //  ' And ones in I + 2
				PC += 2;break;
			case  0x0055:for (int a = 0; a <= X; a++) { MEMORY[I + a] = V[a]; }I += (X + 1);PC += 2;break;
			case  0x0065:for (int a = 0; a <= X; a++) { V[a] = MEMORY[I + a]; }I += (X + 1);PC += 2;break;
			}break;
		default:printf("\nUnknown op code: %.4X\n", OPC);exit(3);
		}
		if (DELAY_TIMER > 0) {DELAY_TIMER -= 1;}
		if (SOUND_TIMER > 0) { int freq = 2000; int duration = 25; SOUND_TIMER = 0; Beep(2000, 25); // 1000 hertz (C5) for 25 milliseconds//cin.get(); // wait 
			return 0;}
		return 0;
}

int C8::DrawScreen(int x, int y) { int pixel_X = x % 64; int pixel_Y = y % 32; int s = GRAPHICS[pixel_X + (pixel_Y * 64)] ^ 1; return s; }

int C8::FetchKeyBoard() {
	KEY_PRESSED = NULL;
	if (GetAsyncKeyState('1')) {KEY_PRESSED = HEX_MAP_BIN[1];}
	if (GetAsyncKeyState('2')) {KEY_PRESSED = HEX_MAP_BIN[2];}
	if (GetAsyncKeyState('3')) {KEY_PRESSED = HEX_MAP_BIN[3];}
	if (GetAsyncKeyState('4')) {KEY_PRESSED = HEX_MAP_BIN[4];}
	if (GetAsyncKeyState('Q')) {KEY_PRESSED = HEX_MAP_BIN[5];}
	if (GetAsyncKeyState('W')) {KEY_PRESSED = HEX_MAP_BIN[6];}
	if (GetAsyncKeyState('E')) {KEY_PRESSED = HEX_MAP_BIN[7];}
	if (GetAsyncKeyState('R')) {KEY_PRESSED = HEX_MAP_BIN[8];}
	if (GetAsyncKeyState('A')) {KEY_PRESSED = HEX_MAP_BIN[9];}
	if (GetAsyncKeyState('S')) {KEY_PRESSED = HEX_MAP_BIN[10];}
	if (GetAsyncKeyState('D')) {KEY_PRESSED = HEX_MAP_BIN[11];}
	if (GetAsyncKeyState('F')) {KEY_PRESSED = HEX_MAP_BIN[12];}
	if (GetAsyncKeyState('Z')) {KEY_PRESSED = HEX_MAP_BIN[13];}
	if (GetAsyncKeyState('X')) {KEY_PRESSED = HEX_MAP_BIN[14];}
	if (GetAsyncKeyState('C')) {KEY_PRESSED = HEX_MAP_BIN[15];}
	if (GetAsyncKeyState('V')) {KEY_PRESSED = HEX_MAP_BIN[0];}
	if (GetAsyncKeyState(0X1B)) { EMULATOR_RUNNING = false;SDL_Quit();}//if (GetAsyncKeyState(0x1B)) MessageBox(NULL, "ESC", "", MB_OK | MB_SYSTEMMODAL);
	return 0;
}//Uint32 pix = 0b00010000111111110100100111110111;

int C8::DibujarSpritesChip8() { //	Uint32 pix = RGB(std::rand() * 255, std::rand() * 255, std::rand() * 255, 255);
	SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
	int mul = 10; int ind = 0; int count = 0; int count2 = 0; int i1 = 0; int i2 = 0;
	int pix = RGB(32 + rand() % (256 - 32), 32 + rand() % (256 - 32), 32 + rand() % (256 - 32));
	for (int y = 0; y < (32); y++) {for (int x = 0; x < (64); x++) {
			for (int c = 0; c < mul; c++) {for (int d = 0; d < mul; d++) {
if (GRAPHICS[x + y * 64] == 1) {putpixel(screenSurface, x*mul+c, d+y*mul, pix); }
						  else {putpixel(screenSurface, x*mul+c, d+y*mul, RGB(0, 0, 0)); }
				}}
		}}SDL_UpdateWindowSurface(window); return 0;
}