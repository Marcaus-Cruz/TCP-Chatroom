# TCP-Chatroom

This project allowed me to practice the fundamentals of the TCP protocol. It's a command line chatroom that requires multiple windows to function properly. There are 3 main parts to this project: a server, participants, and observers. If you were to compare this program to a messaging app; the server acts as the connection point for everybody, participants act as the text box where you type and send your message, and observers act like the feed that you would see above the text box.

To run this program:
1) Download ZIP or clone repository
2) extract if needed and cd into project

I reccomend using TMUX as this program is best demonstrated with 5 terminals.
3) Open 5 terminal windows and run a 'make' command to compile all files using the Makefile

If you're using 5 windows: 1 window will act as the server, 2 window will act as observers, and 2 windows will act as two participants

4) Command for starting Server: $./server {participant port} {observer port}

5) Command for starting Participants: $./participant {server address} {server port}
- You will be prompted to enter a username

6) Command for starting Observers: $./observer {server address} {server port}
- You will be prompted to enter the username of a participant that's already connected to the server.

Once everybody is connected and assigned their roles, participants can start chatting with eachother. You're able to send private messages by starting the message with "@{participant-username} ". These private messages are indicated by a % sign on the recipients observer.

Note: server_address is going to be localhost by default. So you can either type "localhost" or "127.0.0.1" for that argument. 
