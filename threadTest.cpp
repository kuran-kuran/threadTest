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
	digitalWrite(ODATA0, data & 1);
	digitalWrite(ODATA1, (data >> 1) & 1);
	digitalWrite(ODATA2, (data >> 2) & 1);
	digitalWrite(ODATA3, (data >> 3) & 1);
	digitalWrite(ODATA4, (data >> 4) & 1);
	digitalWrite(ODATA5, (data >> 5) & 1);
	digitalWrite(ODATA6, (data >> 6) & 1);
	digitalWrite(ODATA7, (data >> 7) & 1);
	digitalWrite(THREAD_FLG, 1);
	while(digitalRead(THREAD_CHK) != 1)
	{
	}
	digitalWrite(THREAD_FLG, 0);
	while(digitalRead(THREAD_CHK) == 1)
	{
	}
}

// スレッドがメインから4ビット受信
unsigned char rev4bitFromMain(void)
{
	while(digitalRead(THREAD_CHK) != 1){
	}
	unsigned char receive4BitData = digitalRead(IDATA0) +
		digitalRead(IDATA1) * 2 +
		digitalRead(IDATA2) * 4 +
		digitalRead(IDATA3) * 8;
	digitalWrite(THREAD_FLG, 1);
	while(digitalRead(THREAD_CHK) == 1){
	}
	digitalWrite(THREAD_FLG, 0);
	return receive4BitData;
}

// スレッドがメインから1バイト受信
unsigned char revFromMain(void)
{
	unsigned char receiveData = rev4bitFromMain() * 16;
	receiveData += rev4bitFromMain();
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
	int size = revFromMain() + revFromMain() * 256;
	for(int i = 0; i < size; ++ i)
	{
		unsigned char revData = revFromMain();
		if(revData != i % 256)
		{
			threadError = true;
			printf("thread error\n");
		}
	}
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
	int size = revFromMain() + revFromMain() * 256;
	for(int i = 0; i < size; ++ i)
	{
		sendToMain((unsigned int)(i % 256));
	}
}

// Arduino setup
void setup()
{
	for(int i = 0; i < GPIO_CNT; ++ i)
	{
		gpio[i] = 0;
	}
}

// Arduino loop
void loop(void)
{
	unsigned char cmd = revFromMain();
	switch(cmd)
	{
	case 0x80:
		sendToMain(0x00);
		saveTest();
		break;
	case 0x81:
		sendToMain(0x00);
		loadTest();
		break;
	default:
		sendToMain(0xF4); // Error
	}
}

// loopを呼ぶスレッド
unsigned __stdcall loop_thread(void* param)
{
	while(!terminate)
	{
		loop();
	}
	_endthreadex(0);
	return 0;
}

// ********** Main
// メインからスレッドに4ビット送信
void send4bitToThread(unsigned char data)
{
	digitalWrite(IDATA0, data & 1);
	digitalWrite(IDATA1, (data >> 1) & 1);
	digitalWrite(IDATA2, (data >> 2) & 1);
	digitalWrite(IDATA3, (data >> 3) & 1);
	digitalWrite(MAIN_FLG, 1);
	while(digitalRead(MAIN_CHK) == 0) // F1CHK
	{
	}
	digitalWrite(MAIN_FLG, 0);
	while(digitalRead(MAIN_CHK) == 1) // F2CHK
	{
	}
}

// メインからスレッドに1バイト送信
void sendToThread(unsigned char data)
{
	send4bitToThread(data >> 4);
	send4bitToThread(data & 0xF);
}

// メインがスレッドから1バイト受信
unsigned char revFromThread(void)
{
	while(digitalRead(MAIN_CHK) == 0) // F1CHK
	{
	}
	unsigned char receive4BitData = digitalRead(ODATA0) +
		digitalRead(ODATA1) * 2 +
		digitalRead(ODATA2) * 4 +
		digitalRead(ODATA3) * 8 +
		digitalRead(ODATA4) * 16 +
		digitalRead(ODATA5) * 32 +
		digitalRead(ODATA6) * 64 +
		digitalRead(ODATA7) * 128;
	digitalWrite(MAIN_FLG, 1);
	while(digitalRead(MAIN_CHK) == 1) // F2CHK
	{
	}
	digitalWrite(MAIN_FLG, 0);
	return receive4BitData;
}

// メイン
int main()
{
	// thread
	terminate = false;
	threadError = false;
	InitializeCriticalSection(&cs);
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, loop_thread, NULL, 0, NULL);
	if(thread == NULL)
	{
		printf("cannot begin thread.\n");
	}
	// loop
	int toggle = 0;
	while(true)
	{
		if(_kbhit())
		{
			printf("user stop\n");
			break;
		}
		if(toggle == 0)
		{
			// SaveTest
			printf("SaveTest\n");
			sendToThread(0x80);
			unsigned char response = revFromThread();
			if(response != 0)
			{
				printf("save error.\n");
				break;
			}
			int size = 60000;
			sendToThread(size % 256);
			sendToThread(size / 256);
			for(int i = 0; i < size; ++ i)
			{
				sendToThread(i % 256);
			}
			// Save ok
			printf("SaveTest end\n");
		}
		else
		{
			// LoadTest
			printf("LoadTest\n");
			sendToThread(0x81);
			unsigned char response = revFromThread();
			if(response != 0)
			{
				printf("load error. (1)\n");
				break;
			}
			int size = 60000;
			sendToThread(size % 256);
			sendToThread(size / 256);
			for(int i = 0; i < size; ++ i)
			{
				unsigned char data = revFromThread();
				if(data != (i % 256))
				{
					printf("load error. (2)\n");
					break;
				}
			}
			// Load ok
			printf("LoadTest end\n");
		}
		toggle = 1 - toggle;
	}
	terminate = true;
	WaitForSingleObject(thread, INFINITE);
	DeleteCriticalSection(&cs);
	printf("terminate ok\n");
}
