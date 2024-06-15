#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>
#include <windows.h>

#define IDATA0     0
#define IDATA1     1
#define IDATA2     2
#define IDATA3     3
#define ODATA0     4
#define ODATA1     5
#define ODATA2     6
#define ODATA3     7
#define ODATA4     8
#define ODATA5     9
#define ODATA6     10
#define ODATA7     11
#define THREAD_FLG 12
#define THREAD_CHK 13
#define MAIN_CHK   12
#define MAIN_FLG   13
#define GPIO_CNT   14

unsigned char gpio[GPIO_CNT];
bool terminate;
bool threadError;
bool setupFlag;
CRITICAL_SECTION cs;
//	EnterCriticalSection(&cs);
//	LeaveCriticalSection(&cs);

// ********** GPIO
// GPIO書き込み
void digitalWrite(int pin, int data)
{
	gpio[pin] = data & 1;
}

// GPIO読み込み
int digitalRead(int pin)
{
	int data = gpio[pin];
	return data;
}

// ********** Thread
// スレッドからメインに1バイト送信
void sendToMain(unsigned char data)
{
	OutputDebugStringA("A");
	digitalWrite(ODATA0, data & 1);
	digitalWrite(ODATA1, (data >> 1) & 1);
	digitalWrite(ODATA2, (data >> 2) & 1);
	digitalWrite(ODATA3, (data >> 3) & 1);
	digitalWrite(ODATA4, (data >> 4) & 1);
	digitalWrite(ODATA5, (data >> 5) & 1);
	digitalWrite(ODATA6, (data >> 6) & 1);
	digitalWrite(ODATA7, (data >> 7) & 1);
	digitalWrite(THREAD_FLG, 1);
	OutputDebugStringA("B");
	while(digitalRead(THREAD_CHK) != 1)
	{
		OutputDebugStringA("*1");
		Sleep(0);
	}
	digitalWrite(THREAD_FLG, 0);
	OutputDebugStringA("C");
	while(digitalRead(THREAD_CHK) == 1)
	{
		OutputDebugStringA("*2");
		Sleep(0);
	}
	OutputDebugStringA("D");
}

// スレッドがメインから4ビット受信
unsigned char rev4bitFromMain(void)
{
	OutputDebugStringA("E");
	while(digitalRead(THREAD_CHK) != 1)
	{
		OutputDebugStringA("*3");
		Sleep(0);
	}
	OutputDebugStringA("F");
	unsigned char receive4BitData = digitalRead(IDATA0) +
		digitalRead(IDATA1) * 2 +
		digitalRead(IDATA2) * 4 +
		digitalRead(IDATA3) * 8;
	digitalWrite(THREAD_FLG, 1);
	OutputDebugStringA("G");
	while(digitalRead(THREAD_CHK) == 1)
	{
		OutputDebugStringA("*4");
		Sleep(0);
	}
	digitalWrite(THREAD_FLG, 0);
	OutputDebugStringA("H");
	return receive4BitData;
}

// スレッドがメインから1バイト受信
unsigned char revFromMain(void)
{
	OutputDebugStringA("I");
	unsigned char receiveData = rev4bitFromMain() * 16;
	receiveData += rev4bitFromMain();
	OutputDebugStringA("J");
	return receiveData;
}

// SAVEテスト
// rev: cmd:  0x80
// snd: stat: 0x00
// --------
// rev: size: nn
// rev: size: mm
//      size = nn + mm * 256
// rev: data (size bytes)
void saveTest(void)
{
	OutputDebugStringA("K");
	int size = revFromMain() + revFromMain() * 256;
	OutputDebugStringA("L");
	for(int i = 0; i < size; ++ i)
	{
		unsigned char revData = revFromMain();
		if(revData != i % 256)
		{
			threadError = true;
			OutputDebugStringA("@");
			printf("thread error %d %u\n", i, revData);
			exit(0);
		}
	}
	OutputDebugStringA("M");
}

// LOADテスト
// rev: cmd:  0x81
// snd: stat: 0x00
// --------
// rev: size: nn
// rev: size: mm
//      size = nn + mm * 256
// snd: data (size bytes)
void loadTest()
{
	OutputDebugStringA("N");
	int size = revFromMain() + revFromMain() * 256;
	OutputDebugStringA("O");
	for(int i = 0; i < size; ++ i)
	{
		sendToMain((unsigned char)(i % 256));
	}
	OutputDebugStringA("P");
}

// Arduino setup
void setup()
{
	OutputDebugStringA("Q");
	for(int i = 0; i < GPIO_CNT; ++ i)
	{
		gpio[i] = 0;
	}
	setupFlag = true;
	OutputDebugStringA("R");
}

// Arduino loop
void loop(void)
{
	OutputDebugStringA("S");
	unsigned char cmd = revFromMain();
	switch(cmd)
	{
	case 0x80:
		OutputDebugStringA("T");
		sendToMain(0x00);
		OutputDebugStringA("U");
		saveTest();
		OutputDebugStringA("V");
		break;
	case 0x81:
		OutputDebugStringA("W");
		sendToMain(0x00);
		OutputDebugStringA("X");
		loadTest();
		OutputDebugStringA("Y");
		break;
	default:
		OutputDebugStringA("Z");
		sendToMain(0xF4); // Error
	}
	OutputDebugStringA("!");
}

// loopを呼ぶスレッド
unsigned __stdcall loop_thread(void* param)
{
	setup();
	while(!terminate)
	{
		OutputDebugStringA("#");
		loop();
		OutputDebugStringA("$");
	}
	OutputDebugStringA("&");
	_endthreadex(0);
	return 0;
}

// ********** Main
// メインからスレッドに4ビット送信
void send4bitToThread(unsigned char data)
{
	OutputDebugStringA("a");
	digitalWrite(IDATA0, data & 1);
	digitalWrite(IDATA1, (data >> 1) & 1);
	digitalWrite(IDATA2, (data >> 2) & 1);
	digitalWrite(IDATA3, (data >> 3) & 1);
	digitalWrite(MAIN_FLG, 1);
	OutputDebugStringA("b");
	while(digitalRead(MAIN_CHK) == 0) // F1CHK
	{
		OutputDebugStringA("*5");
		Sleep(0);
	}
	digitalWrite(MAIN_FLG, 0);
	OutputDebugStringA("c");
	while(digitalRead(MAIN_CHK) == 1) // F2CHK
	{
		OutputDebugStringA("*6");
		Sleep(0);
	}
	OutputDebugStringA("d");
}

// メインからスレッドに1バイト送信
void sendToThread(unsigned char data)
{
	OutputDebugStringA("e");
	send4bitToThread(data >> 4);
	OutputDebugStringA("f");
	send4bitToThread(data & 0xF);
	OutputDebugStringA("g");
}

// メインがスレッドから1バイト受信
unsigned char revFromThread(void)
{
	OutputDebugStringA("h");
	while(digitalRead(MAIN_CHK) == 0) // F1CHK
	{
		OutputDebugStringA("*7");
		Sleep(0);
	}
	OutputDebugStringA("i");
	unsigned char receive4BitData = digitalRead(ODATA0) +
		digitalRead(ODATA1) * 2 +
		digitalRead(ODATA2) * 4 +
		digitalRead(ODATA3) * 8 +
		digitalRead(ODATA4) * 16 +
		digitalRead(ODATA5) * 32 +
		digitalRead(ODATA6) * 64 +
		digitalRead(ODATA7) * 128;
	digitalWrite(MAIN_FLG, 1);
	OutputDebugStringA("j");
	while(digitalRead(MAIN_CHK) == 1) // F2CHK
	{
		OutputDebugStringA("*8");
		Sleep(0);
	}
	digitalWrite(MAIN_FLG, 0);
	OutputDebugStringA("k");
	return receive4BitData;
}

// メイン
int main()
{
	OutputDebugStringA("l");
	// thread
	terminate = false;
	threadError = false;
	setupFlag = false;
	InitializeCriticalSection(&cs);
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, loop_thread, NULL, 0, NULL);
	if(thread == NULL)
	{
		printf("cannot begin thread.\n");
	}
	while(setupFlag == false)
	{
		OutputDebugStringA("?");
		Sleep(0);
	}
	// loop
	OutputDebugStringA("m");
	int toggle = 0;
	while(true)
	{
		OutputDebugStringA("n");
		if(_kbhit())
		{
			printf("user stop\n");
			break;
		}
		if(toggle == 0)
		{
			OutputDebugStringA("o");
			// SaveTest
			printf("SaveTest\n");
			sendToThread(0x80);
			OutputDebugStringA("p");
			unsigned char response = revFromThread();
			if(response != 0)
			{
				OutputDebugStringA("q");
				printf("save error. {%u}\n", response);
				break;
			}
			int size = 60000;
			OutputDebugStringA("r");
			sendToThread(size % 256);
			OutputDebugStringA("s");
			sendToThread(size / 256);
			OutputDebugStringA("t");
			for(int i = 0; i < size; ++ i)
			{
//				printf("[%d]", i);
				sendToThread(i % 256);
			}
			// Save ok
			printf("SaveTest end\n");
			OutputDebugStringA("u");
		}
		else
		{
			// LoadTest
			printf("LoadTest\n");
			OutputDebugStringA("v");
			sendToThread(0x81);
			unsigned char response = revFromThread();
			if(response != 0)
			{
				OutputDebugStringA("w");
				printf("load error. (1)\n");
				break;
			}
			int size = 60000;
			OutputDebugStringA("x");
			sendToThread(size % 256);
			OutputDebugStringA("y");
			sendToThread(size / 256);
			OutputDebugStringA("z");
			for(int i = 0; i < size; ++ i)
			{
				unsigned char data = revFromThread();
				if(data != (i % 256))
				{
					OutputDebugStringA("1");
					printf("load error. (2) %d, %u\n", i, data);
					break;
				}
			}
			// Load ok
			OutputDebugStringA("2");
			printf("LoadTest end\n");
		}
		toggle = 1 - toggle;
		OutputDebugStringA("3");
	}
	OutputDebugStringA("4");
	terminate = true;
	OutputDebugStringA("5");
	WaitForSingleObject(thread, INFINITE);
	OutputDebugStringA("6");
	DeleteCriticalSection(&cs);
	OutputDebugStringA("7");
	printf("terminate ok\n");
}
