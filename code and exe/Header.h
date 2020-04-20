#pragma once
#pragma comment(lib, "Ws2_32.lib") 
#include <WinSock2.h>
#include <Windows.h>
#include  <string>
#include <WS2tcpip.h> 
#include <chrono>
#include <stdlib.h>
#define chrono_time chrono::time_point<chrono::steady_clock>
#include <iostream>
using namespace std;

// data transfer 
bool send_data(SOCKET& sendSock, struct sockaddr_in& sendAddr, u_short* ports,
	int* my_array, int sent_data_size, int start_index, int process_id, int master_id);

bool send_response(SOCKET& sendSock, struct sockaddr_in& sendAddr, u_short* ports,
	int min_val, int process_id, int master_id);


// bully algorithm 
void election(SOCKET& sendSock, struct sockaddr_in& maSAddr,
	u_short* ports, int my_id, int& master_id,
	bool& wait_for_vectory, chrono_time& start);

void broad_cast_vectory_message(SOCKET& sendSock, struct sockaddr_in& maSAddr,
	u_short* ports, int new_master_id);

void assign_new_master(struct sockaddr_in& maSAddr, u_short* ports, int new_master_id, int& master_id, bool& wait_for_vectory);





class Node
{
public:

	u_short* ports;
	int my_id = -1, master_id = 5, number_of_processes;
	int* my_array, * received_array, array_size, received_array_size;
	int process_index = 1, min_value;
	bool wait_for_vectory;
	bool* received, finish, fill;
	Node()
	{
		number_of_processes = 5;
		ports = new u_short[number_of_processes + 2];
		received = new bool[number_of_processes + 2];
		finish = fill = false;
		wait_for_vectory = false;
		min_value = 999999;
		for (int i = 0; i <= number_of_processes; i++) {
			ports[i] = i;
			received[i] = 0;
		}
		cout << "enter processes id [1-5]" << endl;
		cin >> my_id;
		return;
	}
	void fill_array()
	{ /*will be called when I get ellected as the master*/
		if (fill)
			return;
		fill = true;
		array_size = 10 * number_of_processes;
		my_array = new int[array_size];
		for (int i = 0; i < array_size; i++)
			my_array[i] = rand();
		return;
	}
	void calc_min_value()
	{
		if (finish)
			return;
		min_value = received_array[0];
		for (int i = 0; i < received_array_size; i++)
			min_value = min(min_value, received_array[i]);
		finish = true;
		return;
	}
};