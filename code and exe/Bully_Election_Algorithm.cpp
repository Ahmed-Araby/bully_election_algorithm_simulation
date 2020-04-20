#include "Header.h"	
int main()
{
	Node me;

	// initialize the winsock API 
	WSAData WinSockData;
	int start_ret = WSAStartup(MAKEWORD(2, 0), &WinSockData);
	if (start_ret != 0) {
		cout << "can't start the socket" << endl;
		return 0;
	}

	// build sockets , and addresses structure
	SOCKET listenSock, sendSock;
	struct sockaddr_in  listenSAddr;
	struct sockaddr_in maSAddr; // master address as it take the address , port of the master to connect to it 

	// listner address 
	listenSAddr.sin_family = AF_INET;
	listenSAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	u_short SERVER_PORT = me.ports[me.my_id]; // listen on my end 
	listenSAddr.sin_port = htons(SERVER_PORT);

	// listner sokcet  = master 
	listenSock = socket(AF_INET, SOCK_STREAM, 0); // tcp/ip
	int bind_ret = bind(listenSock, (struct sockaddr*) & listenSAddr, sizeof(listenSAddr));
	listen(listenSock, 10);


	// master address 
	maSAddr.sin_family = AF_INET;
	maSAddr.sin_port = htons(me.ports[me.master_id]);
	inet_pton(AF_INET, "127.0.0.1", &maSAddr.sin_addr);

	// client socekt 
	sendSock = socket(AF_INET, SOCK_STREAM, 0);
	int conval = -1;

	// set 
	fd_set master;
	FD_ZERO(&master);
	FD_SET(listenSock, &master); // non blocking listing and recieving 

	// bool wait_for_vectory = false;
	SYSTEMTIME time;
	chrono_time start = chrono::steady_clock::now();

	// initial ellecetion 
	election(sendSock, maSAddr, me.ports, me.my_id, me.master_id, me.wait_for_vectory, start);

	while (1)  // main loop
	{
		cout << "my id : " << me.my_id << " ,  master id : ";
		if (me.wait_for_vectory)
			cout << "UnKnown";
		else
			cout << me.master_id;
		if (me.master_id == me.my_id)
			cout << ",  curretn minimum value = " << me.min_value << endl;
		cout << endl;

		fd_set copy = master;
		struct timeval TimeOut;
		TimeOut.tv_sec = 2;
		TimeOut.tv_usec = 0;

		int sockcnt = select(0, &copy, nullptr, nullptr, &TimeOut);

		for (int i = 0; i < sockcnt; i++)
		{
			SOCKET actv_sock = copy.fd_array[i];
			if (actv_sock == listenSock)
			{// accept connection 

				SOCKET childSock = accept(listenSock, nullptr, nullptr);
				GetSystemTime(&time);
				cout << "#####asked for being ALIVE at time :  "<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<< endl;
				//cout << endl;
				FD_SET(childSock, &master);
			}

			else
			{// recv message

				//cout << "recv message" << endl;
				char buffer[8000];
				ZeroMemory(buffer, sizeof(buffer));

				// recv the the messages
				int recv_bytes = recv(actv_sock, buffer, sizeof(buffer), 0);
				int data_size = *((int*)buffer); // first 4 bytes 
				while (recv_bytes < data_size)
				{
					int bytesin = recv(actv_sock, buffer + recv_bytes, sizeof(buffer) - recv_bytes, 0);
					if (bytesin == SOCKET_ERROR || bytesin == 0) {
						//cout << "error at receving data in processes" << " " << me.my_id << endl;
						break;
					}
					recv_bytes += bytesin;
				}

				if (recv_bytes != data_size) {
					closesocket(actv_sock);
					FD_CLR(actv_sock, &master);
					continue;
				}
				char msg_type[] = { buffer[4] , buffer[5], buffer[6], buffer[7], '\0' };
				int sender_id = *((int*)& buffer[8]);


				// handle the message 
				GetSystemTime(&time);

				if (strcmp(msg_type, "elec\0") == 0)
				{// do election 
					cout << "#####recieveing election message from " << sender_id << "  at time :  " << time.wHour << ":" << time.wMinute << ":" << time.wSecond << endl;
					if(!me.wait_for_vectory)
						election(sendSock, maSAddr, me.ports, me.my_id, me.master_id, me.wait_for_vectory, start);
				}

				else if (strcmp(msg_type, "vect\0") == 0)
				{ // recieve vectory msg
					cout << "#####recieveing vectory message from bully " << sender_id << " at time : " << time.wHour << ":" << time.wMinute << ":" << time.wSecond << endl;
					assign_new_master(maSAddr, me.ports, sender_id, me.master_id, me.wait_for_vectory);
				}

				else if (strcmp(msg_type, "data\0") == 0)
				{// recv normal data to be processed
					cout << "#####recieveing data for processing from " << sender_id << " at time : " << time.wHour << ":" << time.wMinute << ":" << time.wSecond << endl;

					int recv_array_size = (data_size - 12) / sizeof(int); // int is 4 bytes here !!!
					int* int_ptr = (int*)& buffer[12];
					me.received_array = new int[recv_array_size];
					me.received_array_size = recv_array_size;

					for (int i = 0; i < recv_array_size; i++)
						me.received_array[i] = *(int_ptr + i);
					me.received[me.my_id] = true;
				}

				else if (strcmp(msg_type, "minv\0") == 0)
				{// store min value from a child 
					int min_value = *((int*)& buffer[12]);
					cout << "#####recieveing min value = " << min_value << " from " << sender_id << " at time : " << time.wHour << ":" << time.wMinute << ":" << time.wSecond << endl;
					me.min_value = min(me.min_value, min_value);
				}
				// close the socket 
				closesocket(actv_sock);
				FD_CLR(actv_sock, &master);
			}
		}

		/* ####################    master   #####################*/
		if (me.my_id == me.master_id)
		{
			if (me.received[me.process_index]) {
				me.process_index = max(1, (me.process_index + 1) % (me.number_of_processes + 1));
				continue;
			}
			me.fill_array(); // if filled nothing happen 
			int chunck_size = me.array_size / me.number_of_processes;

			// send chanks 
			bool success = false;
			if (me.process_index == me.master_id)
			{
				me.received_array_size = chunck_size;
				me.received_array = new int[chunck_size];
				int start_index = (me.process_index - 1) * chunck_size;
				for (int i = 0; i < chunck_size; i++)
					me.received_array[i] = me.my_array[start_index + i];
				me.calc_min_value();
				me.received[me.process_index] = 1;
			}
			else
				success = send_data(sendSock, maSAddr, me.ports, me.my_array, chunck_size, (me.process_index - 1) * chunck_size, me.process_index, me.master_id);
			/*if(success==false)
				cout << "error in sending data from master {" << me.master_id << "}" << " to process {" << me.process_index << "}" << endl;*/
			if (success)
				me.received[me.process_index] = 1;
			me.process_index = max((me.process_index + 1) % (me.number_of_processes+1), 1);
			continue;
		}

		/*############################# childs ##################################*/
		// do processing 
		if (me.received[me.my_id] && !me.finish) {
			me.calc_min_value();

			send_response(sendSock, maSAddr, me.ports,
				me.min_value, me.my_id, me.master_id);
		}

		// if U are a client try to connect to master.
		sendSock = socket(AF_INET, SOCK_STREAM, 0);
		maSAddr.sin_port = htons(me.ports[me.master_id]); // bug 
		conval = connect(sendSock, (struct sockaddr*) & maSAddr, sizeof(maSAddr));

		if (conval != 0 && !me.wait_for_vectory)
			election(sendSock, maSAddr, me.ports, me.my_id, me.master_id, me.wait_for_vectory, start);

		else if (me.wait_for_vectory)
		{
			chrono_time end = chrono::steady_clock::now();
			int elps_time = chrono::duration_cast<chrono::seconds>(end - start).count();
			if (elps_time >= 20) // do ellection again 
				me.wait_for_vectory = 0;
		}
	}
	return 0;
}