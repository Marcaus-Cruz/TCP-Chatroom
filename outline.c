Server Side 
	Command Line Parameters port for participants, port for observers
	While (1)
		Reset wrk-fds
		Select
		If (read)
			recv
			If (client is connecting for first time)
                        	Accept connection
                        	If (participant)
                                	Add to writefds & readfds
                        	Else if (observer)
                                	Add to writefds
                        	If (server is maxed)
                                	Send ‘N’
                                	Close socket
                        	Else
                                	Send ‘Y’
			else
				if received 0 && it's from an active participant // Active Participant shutdown
					//remove active participant from writefds and readfs
					Send usernamen length as uint16_t to all observers in network byte order
					Send User $username has left to all observers
					if disconnected participant has observer
						disconnect observer // remove from writefds

		else if (write)



Participant Client Side
	Command Line Parameters IP Address and port
	Connect
	Recv (Y or N)
	If (N)
		Print “Server is full”
		Close socket
		EXIT_SUCCESS
	else 
		Prompt for username (1-10 Characters, 60 sec timeout)
		if within time limit
			if (username > 10 and username <1 && valid) // uppercase, lowercase, numbers and underscores only. no whitespace, no symbols. Check in a for loop before this line
				remprompt
			else 
				send username length to server as uint8_t
				send username to server
		else
			print timeout expired
			close socket
			EXIT_SUCCESS
		recv // Y if valid, T if taken, I if invalid
		if (T)
			reset 60 sec timer
			reprompt with username already taken
		else if (I)
			reprompt with username invalid
		else if (Y)
			while (1)
				print prompt
				if message contains at least 1 non-whitespace // Server needs to do this too, kick participant and observer if they send whitespace message  
					send length of message as uint16_t in network byte order
					send message

Observer Client Side
	Command Line Parameters IP Address and Port
	Connect
	Recv (Y or N)
	if (N)
		Print server is full
		Close socket
		EXIT_SUCCESS
	else 
		Prompt for username (1-10 Characters, 60 sec timeout)
		if within time limit
			if username > 10 or username < 1
				remprompt
			else 
				send username length to server as uin8_t
				send username to server
				recv // N is no user found with that name, T if active participant is already taken, Y if valid
				if (N)
					print there is no active participant by that username
					close socket
					EXIT_SUCCESS
				else if (T)
					reset timeout
					reprompt with username already taken
				else if (Y)
					begin active observer code
		else
			print timeout expired
			close socket
			EXIT_SUCCESS
