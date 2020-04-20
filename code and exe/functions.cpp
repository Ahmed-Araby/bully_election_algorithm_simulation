#include "Header.h"

/* #############################################  DATA Transfer #########################################################*/
bool send_data(SOCKET& sendSock, struct sockaddr_in& sendAddr, u_short* ports,
	int* my_array, int sent_data_size, int start_index, int process_id, int master_id)
{
	//cout << "send data to process :" << process_id << endl;
	bool success = true;


	sendSock = socket(AF_INET, SOCK_STREAM, 0);
	sendAddr.sin_port = htons(ports[process_id]);
	int conval = connect(sendSock, (struct sockaddr*) & sendAddr, sizeof(sendAddr));

	if (conval == 0) {
		int msg_size = (12 + sent_data_size * sizeof(int));
		char* msg = new char[msg_size];
		ZeroMemory(msg, sizeof(msg));

		// add msg size 
		int* int_ptr = (int*)msg;
		*int_ptr = msg_size;
		// add msg type
		msg[4] = 'd';
		msg[5] = 'a';
		msg[6] = 't';
		msg[7] = 'a';

		// add sender_id
		*((int*)& msg[8]) = master_id;

		// add the data 
		int_ptr = (int*)& msg[12];
		for (int i = 0; i < sent_data_size; i++)
			* (int_ptr + i) = my_array[start_index + i];

		// send the data 
		int sent_bytes = 0;
		while (sent_bytes < msg_size)
		{
			int bytesout = send(sendSock, msg + sent_bytes, msg_size - sent_bytes, 0);
			if (bytesout == SOCKET_ERROR || bytesout == 0)
			{
				//cout << "error in sending data from master {" << master_id << "}" << " to process {" << process_id << "}" << endl;
				success = false;
				break;
			}
			sent_bytes += bytesout;
		}

		delete[] msg;
	}
	else {
		//cout << "cant send data to processes "<<process_id << endl;
		success = false;
	}
	closesocket(sendSock);  // ** bug 
	return success;
}


bool send_response(SOCKET& sendSock, struct sockaddr_in& sendAddr, u_short* ports,
	int min_val, int process_id, int master_id)
{
	cout << "sending min val to master" << endl;
	bool success = true;

	sendSock = socket(AF_INET, SOCK_STREAM, 0);
	sendAddr.sin_port = htons(ports[master_id]);  // connect to master 
	int conval = connect(sendSock, (struct sockaddr*) & sendAddr, sizeof(sendAddr));

	if (conval == 0)
	{
		int msg_size = 16;
		char* msg = new char[msg_size];
		ZeroMemory(msg, sizeof(msg));

		// put size of msg
		*((int*)msg) = msg_size;

		// put the msg_type
		msg[4] = 'm';
		msg[5] = 'i';
		msg[6] = 'n';
		msg[7] = 'v';

		// put id of sender processes 
		*((int*)& msg[8]) = process_id;

		// put my answer 
		*((int*)& msg[12]) = min_val;

		// just  16 bytes supposed to get passed in 1 sent operation 
		int sent_bytes = 0;
		while (sent_bytes < msg_size)
		{
			int bytesout = send(sendSock, msg + sent_bytes, msg_size - sent_bytes, 0);
			if (0 == bytesout || bytesout == SOCKET_ERROR)
			{
				cout << "error in sending min value from process {" << process_id << "}" << " to master {" << master_id << "}" << endl;
				success = false;
				break;
			}
			sent_bytes += bytesout;
		}

		delete[]msg;
	}
	else {
		cout << "error in connection from process {" << process_id << "}" << " to master {" << master_id << "}" << endl;
		success = false;
	}

	closesocket(sendSock);
	return success;
}




/* ###################################################  bully algorithm ####################################################*/
void assign_new_master(struct sockaddr_in& maSAddr, u_short* ports, int new_master_id, int& master_id, bool& wait_for_vectory)
{
	cout << "assign new master" << endl;
	master_id = new_master_id;
	maSAddr.sin_port = htons(ports[new_master_id]);
	wait_for_vectory = false;
	return;
}


void broad_cast_vectory_message(SOCKET& sendSock, struct sockaddr_in& maSAddr,
	u_short* ports, int new_master_id)
{
	cout << "vectory broadcasting" << endl;
	for (int i = 1; i < new_master_id; i++)
	{
		sendSock = socket(AF_INET, SOCK_STREAM, 0);
		maSAddr.sin_port = htons(ports[i]);

		int conval = connect(sendSock, (struct sockaddr*) & maSAddr, sizeof(maSAddr));
		if (conval == 0)
		{
			char msg[] = { '0','0','0','0',  'v', 'e', 'c', 't',  '0','0','0','0' };
			int* int_ptr;
			int msg_bytes = sizeof(msg);

			// store the size of the data
			int_ptr = (int*)msg;
			*int_ptr = msg_bytes;
			// store the id of the processes
			int_ptr = (int*)& msg[8];
			*int_ptr = new_master_id;
			// send data 
			int sent_bytes = 0;
			while (sent_bytes < msg_bytes)
			{
				int bytesout = send(sendSock, msg + sent_bytes, sizeof(msg) - sent_bytes, 0);

				if (bytesout == SOCKET_ERROR || bytesout == 0) {
					cout << "error in broadcasting the new master id form " << new_master_id << " to " << i << endl;
					break;
				}
				sent_bytes += bytesout;
			}
		}
		/*else
			cout << "failed to send vectory message to process : "<<i<< endl;*/
	}
	return;
}

void election(SOCKET& sendSock, struct sockaddr_in& maSAddr,
	u_short* ports, int my_id, int& master_id,
	bool& wait_for_vectory, chrono_time& start)
{
	cout << "election process" << endl;
	for (int i = my_id + 1; i <= 5; i++)
	{
		sendSock = socket(AF_INET, SOCK_STREAM, 0);
		maSAddr.sin_port = htons(ports[i]);

		int conval = connect(sendSock, (struct sockaddr*) & maSAddr, sizeof(maSAddr));

		if (conval == 0)
		{
			// send election message 
			// store the header
			char msg[] = { '0', '0', '0', '0', 'e', 'l', 'e','c', '0', '0', '0', '0' };

			// store the data length 
			int data_len_bytes = sizeof(msg); // garented to be sent and retrieved at once 
			int* int_ptr = (int*)msg;
			*int_ptr = data_len_bytes;

			// store the sender processes id 
			int_ptr = (int*)& msg[8];
			*int_ptr = my_id;

			// send data
			int sent_bytes = 0;
			while (sent_bytes < sizeof(msg)) {
				int bytesout = send(sendSock, msg + sent_bytes, sizeof(msg) - sent_bytes, 0);
				if (bytesout == SOCKET_ERROR || bytesout == 0) {
					cout << "error in sending election message from : " << my_id << " to : " << i << endl;
					break;
				}
				sent_bytes += bytesout;
			}

			// stop election processes
			wait_for_vectory = true;
			start = chrono::steady_clock::now();
			return;
		}
	}

	// IAM The new master 
	broad_cast_vectory_message(sendSock, maSAddr, ports, my_id);
	assign_new_master(maSAddr, ports, my_id, master_id, wait_for_vectory);
	return;
}